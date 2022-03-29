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

#include "ipc_transactors.h"

#include "common_defines.h"
#include "common_utilities_hpp.h"
#include "json.hpp"

namespace OHOS::uitest {
    using namespace std;
    using namespace chrono;
    using namespace nlohmann;

    static uint32_t NextMessageId()
    {
        static uint32_t increasingMessageId = 0;
        return increasingMessageId++;
    }

    void MessageTransceiver::EmitCall(string_view apiId, string_view caller, string_view params)
    {
        TransactionMessage msg = {
            .apiId_=string(apiId),
            .callerParcel_=string(caller),
            .paramsParcel_=string(params)
        };
        msg.id_ = NextMessageId();
        msg.type_ = TransactionType::CALL;
        EmitMessage(msg);
    }

    void MessageTransceiver::EmitReply(const TransactionMessage &request, string_view reply)
    {
        TransactionMessage msg = request; // keep the calling id
        msg.resultParcel_ = string(reply);
        msg.type_ = TransactionType::REPLY;
        EmitMessage(msg);
    }

    void MessageTransceiver::EmitHandshake()
    {
        TransactionMessage msg = {
            .id_ = NextMessageId(),
            .type_=TransactionType::HANDSHAKE
        };
        EmitMessage(msg);
    }

    void MessageTransceiver::EmitAck(const TransactionMessage &handshake)
    {
        TransactionMessage msg = handshake; // keep the calling id
        msg.type_ = TransactionType::ACK;
        EmitMessage(msg);
    }

    void MessageTransceiver::EmitExit()
    {
        TransactionMessage msg = {
            .id_ = NextMessageId(),
            .type_=TransactionType::EXIT
        };
        EmitMessage(msg);
    }

    void MessageTransceiver::EmitMessage(const TransactionMessage &message)
    {
        lastOutgoingMessageMillis_.store(GetCurrentMillisecond());
        DoEmitMessage(message);
    }

    void MessageTransceiver::SetMessageFilter(std::function<bool(TransactionType)> filter)
    {
        this->messageFilter_ = move(filter);
    }

    MessageTransceiver::PollStatus MessageTransceiver::PollCallReply(TransactionMessage &out, uint64_t timeoutMs)
    {
        const auto timeout = chrono::milliseconds(timeoutMs);
        static constexpr uint32_t flagSet = FLAG_REQUEST_EXIT | FLAG_CONNECT_DIED;
        const auto checker = [&]() {
            return (extraFlags_.load() & flagSet) != 0 || !messageQueue_.empty();
        };
        unique_lock<mutex> lock(queueLock_);
        if (busyCond_.wait_for(lock, timeout, checker)) {
            if ((extraFlags_.load() & flagSet) != 0) {
                if ((extraFlags_.load() & FLAG_REQUEST_EXIT) > 0) {
                    return ABORT_REQUEST_EXIT;
                } else {
                    return ABORT_CONNECTION_DIED;
                }
            } else {
                // copy and pop
                out = messageQueue_.front();
                messageQueue_.pop();
                return SUCCESS;
            }
        } else {
            return ABORT_WAIT_TIMEOUT;
        }
    }

    void MessageTransceiver::OnReceiveMessage(const TransactionMessage &message)
    {
        if (message.type_ == TransactionType::INVALID) {
            return;
        }
        if (messageFilter_ != nullptr && !messageFilter_(message.type_)) {
            return;
        }
        lastIncomingMessageMillis_.store(GetCurrentMillisecond());
        bool doNotification = true;
        if (message.type_ == CALL || message.type_ == REPLY) {
            lock_guard lock(queueLock_);
            messageQueue_.push(message);
        } else if (message.type_ == EXIT) {
            extraFlags_.store(extraFlags_.load() | FLAG_REQUEST_EXIT);
        } else if (message.type_ == HANDSHAKE) {
            // send ack automatically
            EmitAck(message);
            doNotification = false;
        } else {
            // handshake and ack are DFX events, won't be enqueued and notified
            doNotification = false;
        }
        if (doNotification) {
            busyCond_.notify_all();
        }
    }

    void MessageTransceiver::ScheduleCheckConnection(bool emitHandshake)
    {
        if (autoHandshaking_.load()) {
            return;
        }
        autoHandshaking_.store(true);
        lastOutgoingMessageMillis_.store(0);
        lastIncomingMessageMillis_.store(GetCurrentMillisecond()); // give a reasonable initial value
        static constexpr uint32_t slices = 100;
        static constexpr uint64_t secureDurationMs = WATCH_DOG_TIMEOUT_MS * 0.9;
        constexpr auto interval = chrono::milliseconds(secureDurationMs / slices);
        future<void> periodWork = async(launch::async, [transceiver = this, interval, emitHandshake]() {
            while (transceiver != nullptr && transceiver->autoHandshaking_.load()) {
                const auto millis = GetCurrentMillisecond();
                const auto outgoingIdleTime = millis - transceiver->lastOutgoingMessageMillis_.load();
                const auto incomingIdleTime = millis - transceiver->lastIncomingMessageMillis_.load();
                if (emitHandshake && outgoingIdleTime > secureDurationMs) {
                    // emit handshake in secure_duration
                    transceiver->EmitHandshake();
                }
                // check connection died in each slice
                if (incomingIdleTime > WATCH_DOG_TIMEOUT_MS) {
                    if (((transceiver->extraFlags_.load()) & FLAG_CONNECT_DIED) == 0) {
                        // first detected
                        transceiver->extraFlags_.store(transceiver->extraFlags_.load() | FLAG_CONNECT_DIED);
                        LOG_D("Connection dead detected");
                    }
                    transceiver->busyCond_.notify_all(); // notify the observer immediately
                }
                this_thread::sleep_for(interval);
            }
            LOG_D("Connection check exited");
        });
        handshakeFuture_ = move(periodWork);
        LOG_I("Connection-check scheduled, autoHandshake=%{public}d", emitHandshake);
    }

    bool MessageTransceiver::EnsureConnectionAlive(uint64_t timeoutMs)
    {
        constexpr uint64_t intervalMs = 20;
        constexpr auto duration = chrono::milliseconds(intervalMs);
        const auto prevIncoming = lastIncomingMessageMillis_.load();
        for (size_t count = 0; count < (timeoutMs / intervalMs); count++) {
            if (lastIncomingMessageMillis_.load() > prevIncoming) { // newer message came
                return true;
            }
            EmitHandshake();
            this_thread::sleep_for(duration);
        }
        return false;
    }

    void MessageTransceiver::Finalize()
    {
        if (autoHandshaking_.load() && handshakeFuture_.valid()) {
            autoHandshaking_.store(false);
            handshakeFuture_.get();
        }
    }

    bool Transactor::Initialize()
    {
        auto pTransceiver = CreateTransceiver();
        DCHECK(pTransceiver != nullptr);
        transceiver_ = move(pTransceiver);
        transceiver_->SetMessageFilter(GetMessageFilter());
        return transceiver_->Initialize();
    }

    void Transactor::Finalize()
    {
        if (transceiver_ != nullptr) {
            // inject exit message
            auto terminate = TransactionMessage {.type_ = TransactionType::EXIT};
            transceiver_->OnReceiveMessage(terminate);
            transceiver_->Finalize();
        }
    }

    uint32_t TransactionServer::RunLoop()
    {
        DCHECK(transceiver_ != nullptr && callFunc_ != nullptr);
        while (true) {
            TransactionMessage message;
            auto status = transceiver_->PollCallReply(message, WAIT_TRANSACTION_MS);
            string reply;
            switch (status) {
                case MessageTransceiver::PollStatus::SUCCESS:
                    DCHECK(message.type_ == TransactionType::CALL);
                    reply = callFunc_(message.apiId_, message.callerParcel_, message.paramsParcel_);
                    transceiver_->EmitReply(message, reply);
                    break;
                case MessageTransceiver::PollStatus::ABORT_CONNECTION_DIED:
                    return EXIT_CODE_FAILURE;
                case MessageTransceiver::PollStatus::ABORT_REQUEST_EXIT:
                    return EXIT_CODE_SUCCESS;
                default: // continue wait-and-fetch
                    continue;
            }
        }
    }

    void TransactionServer::SetCallFunction(function<string(string_view, string_view, string_view)> func)
    {
        callFunc_ = std::move(func);
    }

    static string CreateResultForDiedConnection()
    {
        json data;
        json exceptionInfo;
        exceptionInfo[KEY_CODE] = "INTERNAL_ERROR";
        exceptionInfo[KEY_MESSAGE] = "connection with uitest_daemon is dead";
        data[KEY_EXCEPTION] = exceptionInfo;
        return data.dump();
    }

    static string CreateResultForConcurrentInvoke(string_view processingApi, string_view incomingApi)
    {
        static constexpr string_view msg = "uitest-api dose not allow calling concurrently, current processing:";
        json data;
        json exceptionInfo;
        exceptionInfo[KEY_CODE] = "USAGE_ERROR";
        exceptionInfo[KEY_MESSAGE] = string(msg) + string(processingApi) + ", incoming: " + string(incomingApi);
        data[KEY_EXCEPTION] = exceptionInfo;
        return data.dump();
    }

    string TransactionClient::InvokeApi(string_view apiId, string_view caller, string_view params)
    {
        unique_lock<mutex> stateLock(stateMtx_);
        // return immediately if the cs-connection has died or concurrent invoking occurred
        if (transceiver_ == nullptr || connectionDied_) {
            return CreateResultForDiedConnection();
        }
        if (!processingApi_.empty()) {
            return CreateResultForConcurrentInvoke(processingApi_, apiId);
        }
        processingApi_ = apiId;
        stateLock.unlock(); // unlock, allow reentry, make it possible to check and reject concurrent usage
        transceiver_->EmitCall(apiId, caller, params);
        while (true) {
            TransactionMessage message;
            auto status = transceiver_->PollCallReply(message, WAIT_TRANSACTION_MS);
            string reply;
            switch (status) {
                case MessageTransceiver::PollStatus::SUCCESS:
                    DCHECK(message.type_ == TransactionType::REPLY);
                    stateLock.lock();
                    processingApi_.clear();
                    stateLock.unlock();
                    return message.resultParcel_;
                case MessageTransceiver::PollStatus::ABORT_CONNECTION_DIED:
                case MessageTransceiver::PollStatus::ABORT_REQUEST_EXIT:
                    stateLock.lock();
                    connectionDied_ = true;
                    stateLock.unlock();
                    return CreateResultForDiedConnection();
                default: // continue wait-and-fetch
                    break;
            }
        }
    }

    void TransactionClient::Finalize()
    {
        if (transceiver_ != nullptr) {
            // destroy server side
            transceiver_->EmitExit();
            // destroy self side
            Transactor::Finalize();
            connectionDied_ = true;
            LOG_I("CsConnection disposed");
            transceiver_ = nullptr;
        }
    }
}