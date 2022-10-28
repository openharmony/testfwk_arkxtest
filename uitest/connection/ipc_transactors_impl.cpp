/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ashmem.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <cinttypes>
#include "common_utilities_hpp.h"
#include "ipc_transactors_impl.h"

namespace OHOS::uitest {
    using namespace std;
    static constexpr uint32_t CHECK_SHARED_FILE_INTERVAL_MS = 50;
    static constexpr uint32_t CHECK_SHARED_FILE_ATTEMPTS = 100;
    static constexpr uint32_t POLL_MSG_INTERVAL_MS = 1;
    static constexpr size_t CHAR_BUF_SIZE = 4000;
    static constexpr size_t MMAP_SIZE = 8192;

    // shared data buffer, all members should be POD and not pointer/reference allowed
    struct SharedMsgDataBuf {
        // use separated call/reply data buffer to avoid overwriting each other
        bool ready_ = false;
        uint32_t id_;
        TransactionType type_;
        size_t dataLen_;
        char data_[CHAR_BUF_SIZE];
    };
    // shared memories
    static unique_ptr<OHOS::Ashmem> g_ashmem;
    static future<void> g_memRwWork;
    static SharedMsgDataBuf *g_callMsgBuf = nullptr;
    static SharedMsgDataBuf *g_replyMsgBuf = nullptr;
    static bool g_shmemReleased = false;

    TransactionTransceiverImpl::TransactionTransceiverImpl(string_view token, bool asServer)
        : asServer_(asServer), token_(token) {}

    bool TransactionTransceiverImpl::Initialize()
    {
        if (g_ashmem != nullptr) {
            return true;
        }
        uint32_t checkFileAttempts = 0;
        const auto openFlags = asServer_ ? O_RDWR : (O_RDWR | O_CREAT);
        constexpr mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        auto fd = open(token_.c_str(), openFlags, mode);
        // shared is created by client, server wait for available
        while (asServer_ && fd <=0 && checkFileAttempts < CHECK_SHARED_FILE_ATTEMPTS) {
            this_thread::sleep_for(chrono::milliseconds(CHECK_SHARED_FILE_INTERVAL_MS));
            fd = open(token_.c_str(), openFlags, mode);
            checkFileAttempts++;
        }
        if (fd <= 0) {
            LOG_W("Open shared file failed: %{public}s, %{public}s", token_.c_str(), strerror(errno));
            return false;
        }
        char zeroBuf[MMAP_SIZE] = {0};
        write(fd, zeroBuf, MMAP_SIZE);
        g_ashmem = make_unique<OHOS::Ashmem>(fd, MMAP_SIZE);
        if (!g_ashmem->MapReadAndWriteAshmem()) {
            LOG_E("MapReadAndWriteAshmem failed");
            return false;
        }
        // compute callMsg pointer and replyMsg pointer
        auto memStart = const_cast<void *>(g_ashmem->ReadFromAshmem(0, 0));
        g_callMsgBuf = reinterpret_cast<SharedMsgDataBuf *>(memStart);
        g_replyMsgBuf = g_callMsgBuf + 1;
        LOG_D("Mmap succeed, g_callMsgBuf=%{public}p, g_replyMsgBuf=%{public}p", g_callMsgBuf, g_replyMsgBuf);
        // start incoming message async polling loop
        auto pInMsg = asServer_ ? g_callMsgBuf : g_replyMsgBuf;
        g_memRwWork = async(launch::async, [pInMsg, this]() -> void {
            while (!g_shmemReleased && g_ashmem != nullptr) {
                if (g_shmemReleased) {
                    break;
                } else if (!pInMsg->ready_) { // wait for incoming message
                    this_thread::sleep_for(chrono::milliseconds(POLL_MSG_INTERVAL_MS));
                    continue;
                }
                TransactionMessage message;
                message.id_ = pInMsg->id_;
                message.type_ = pInMsg->type_;
                message.dataParcel_ = string(pInMsg->data_, pInMsg->dataLen_);
                pInMsg->ready_ = false; // clear flag so that producer can werite new messge
                this->OnReceiveMessage(message);
            }
            return;
        });
        return true;
    }

    void TransactionTransceiverImpl::Finalize()
    {
        MessageTransceiver::Finalize();
        g_shmemReleased = true;
        if (g_memRwWork.valid()) {
            // wait for memRwWord exit then release mem, to avoid SIGSEGV
            g_memRwWork.get();
        }
        if (g_ashmem != nullptr) {
            g_ashmem->UnmapAshmem();
            g_ashmem->CloseAshmem();
            g_ashmem = nullptr;
        }
        // delete shared file at client which did the creation
        if (!asServer_) {
            (void)remove(token_.c_str());
        }
    }

    void TransactionTransceiverImpl::DoEmitMessage(const TransactionMessage &message)
    {
        // server write into reply buf, client write into call buf
        auto pOutMsg = asServer_ ? g_replyMsgBuf : g_callMsgBuf;
        while (g_ashmem != nullptr && pOutMsg->ready_ == true) { // wait message in buffer consumed
            this_thread::sleep_for(chrono::milliseconds(POLL_MSG_INTERVAL_MS));
        }
        if (g_ashmem == nullptr) {
            LOG_W("Ashmem is not available");
            return;
        }
        pOutMsg->id_ = message.id_;
        pOutMsg->type_ = message.type_;
        pOutMsg->dataLen_ = message.dataParcel_.length();
        for (size_t idx = 0; idx < pOutMsg->dataLen_; idx++) {
            pOutMsg->data_[idx] = message.dataParcel_.at(idx);
        }
        pOutMsg->ready_ = true; // set flag so that consumer can read this message
    }

    TransactionServerImpl::TransactionServerImpl(string_view token) : token_(token) {};

    TransactionServerImpl::~TransactionServerImpl() {};

    bool TransactionServerImpl::Initialize()
    {
        if (!Transactor::Initialize()) {
            return false;
        }
        // schedule connection-checking on initialization
        transceiver_->ScheduleCheckConnection(false);
        return true;
    }

    unique_ptr<MessageTransceiver> TransactionServerImpl::CreateTransceiver()
    {
        return make_unique<TransactionTransceiverImpl>(token_, true);
    }

    TransactionClientImpl::TransactionClientImpl(string_view token) : token_(token) {};

    TransactionClientImpl::~TransactionClientImpl() {};

    static constexpr uint64_t WAIT_CONNECTION_TIMEOUT_MS = 5000;

    bool TransactionClientImpl::Initialize()
    {
        if (!Transactor::Initialize()) {
            return false;
        }
        // emit handshake and wait-for first interaction established
        LOG_I("Start checking CS-interaction");
        if (!transceiver_->DiscoverPeer(WAIT_CONNECTION_TIMEOUT_MS)) {
            LOG_E("Wait CS-interaction timed out in %{public}" PRIu64 "  ms", WAIT_CONNECTION_TIMEOUT_MS);
            return false;
        }
        // schedule connection-checking with auto-handshaking
        transceiver_->ScheduleCheckConnection(true);
        LOG_I("Check CS-interaction succeed");
        return true;
    }

    unique_ptr<MessageTransceiver> TransactionClientImpl::CreateTransceiver()
    {
        return make_unique<TransactionTransceiverImpl>(token_, false);
    }

    static unique_ptr<TransactionClientImpl> sClient = nullptr;
    static atomic<bool> g_sSetupCalled = false;

    /**Exported transaction-client initialization callback function.*/
    bool SetupTransactionEnv(string_view token)
    {
        if (!g_sSetupCalled.load()) {
            if (sClient == nullptr) {
                sClient = make_unique<TransactionClientImpl>(token);
            }
            if (!sClient->Initialize()) {
                LOG_E("SetupTransactionEnv failed");
            }
            g_sSetupCalled.store(true);
        }
        return true;
    }

    /**Exported transaction client api-calling function.*/
    void TransactionClientFunc(const ApiCallInfo& call, ApiReplyInfo& reply)
    {
        DCHECK(sClient != nullptr && g_sSetupCalled.load());
        sClient->InvokeApi(call, reply);
    }

    /**Exported transaction-client dispose callback function.*/
    void DisposeTransactionEnv()
    {
        if (g_sSetupCalled.load() && sClient != nullptr) {
            sClient->Finalize();
            g_sSetupCalled.store(false);
        }
    }
}
