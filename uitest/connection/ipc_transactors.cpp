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

    void MessageTransceiver::EmitCall(string_view dataParcel)
    {
        TransactionMessage msg = {
            .id_ = NextMessageId(),
            .type_ = TransactionType::CALL,
            .dataParcel_ = string(dataParcel)
        };
        EmitMessage(msg);
    }

    void MessageTransceiver::EmitReply(const TransactionMessage &request, string_view dataParcel)
    {
        TransactionMessage msg = {
            .id_ = request.id_, // keep the calling id
            .type_ = TransactionType::REPLY,
            .dataParcel_ = string(dataParcel)
        };
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
                const auto incomingMillis = transceiver->lastIncomingMessageMillis_.load();
                const auto outgoingMillis = transceiver->lastOutgoingMessageMillis_.load();
                const auto millis = GetCurrentMillisecond();
                const auto incomingIdleTime = millis - incomingMillis;
                const auto outgoingIdleTime = millis - outgoingMillis;
                if (emitHandshake && (outgoingIdleTime > secureDurationMs || incomingIdleTime > secureDurationMs)) {
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

    bool MessageTransceiver::DiscoverPeer(uint64_t timeoutMs)
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

    /** Serialization function: Marshal ApiCallInfo into string.*/
    static string MarshalApiCall(const ApiCallInfo& call)
    {
        auto data = nlohmann::json::array();
        data.emplace_back(call.apiId_);
        data.emplace_back(call.callerObjRef_);
        data.emplace_back(call.paramList_);
        return data.dump();
    }

    /** Serialization function: Unmarshal ApiCallInfo from string.*/
    static void UnmarshalApiCall(ApiCallInfo& call, string_view from)
    {
        auto data = nlohmann::json::parse(from);
        call.apiId_ = data.at(INDEX_ZERO);
        call.callerObjRef_ = data.at(INDEX_ONE);
        call.paramList_ = data.at(INDEX_TWO);
    }

    /** Serialization function: Marshal ApiReplyInfo into string.*/
    static string MarshalApiReply(const ApiReplyInfo& reply)
    {
        auto data = nlohmann::json::array();
        data.emplace_back(reply.resultValue_);
        data.emplace_back(reply.exception_.code_);
        data.emplace_back(reply.exception_.message_);
        return data.dump();
    }

    /** Serialization function: Unmarshal ApiReplyInfo from string.*/
    static void UnmarshalApiReply(ApiReplyInfo& reply, string_view from)
    {
        auto data = nlohmann::json::parse(from);
        reply.resultValue_ = data.at(INDEX_ZERO);
        reply.exception_.code_ = data.at(INDEX_ONE);
        reply.exception_.message_ = data.at(INDEX_TWO);
    }

    uint32_t TransactionServer::RunLoop()
    {
        DCHECK(transceiver_ != nullptr && callFunc_ != nullptr);
        while (true) {
            TransactionMessage message;
            auto status = transceiver_->PollCallReply(message, WAIT_TRANSACTION_MS);
            auto call = ApiCallInfo();
            auto reply = ApiReplyInfo();
            switch (status) {
                case MessageTransceiver::PollStatus::SUCCESS:
                    DCHECK(message.type_ == TransactionType::CALL);
                    UnmarshalApiCall(call, message.dataParcel_);
                    callFunc_(call, reply);
                    transceiver_->EmitReply(message, MarshalApiReply(reply));
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

    void TransactionServer::SetCallFunction(function<void(const ApiCallInfo&, ApiReplyInfo&)> func)
    {
        callFunc_ = std::move(func);
    }

    static void CreateResultForDiedConnection(ApiReplyInfo& reply)
    {
        reply.exception_.code_ = ErrCode::INTERNAL_ERROR;
        reply.exception_.message_ = "connection with uitest_daemon is dead";
    }

    static void CreateResultForConcurrentInvoke(string_view processing, string_view incoming, ApiReplyInfo& reply)
    {
        static constexpr string_view msg = "uitest-api dose not allow calling concurrently, current processing:";
        reply.exception_.code_ = ErrCode::USAGE_ERROR;
        reply.exception_.message_ = string(msg) + string(processing) + ", incoming: " + string(incoming);
    }

    void TransactionClient::InvokeApi(const ApiCallInfo& call, ApiReplyInfo& reply)
    {
        unique_lock<mutex> stateLock(stateMtx_);
        // return immediately if the cs-connection has died or concurrent invoking occurred
        if (transceiver_ == nullptr || connectionDied_) {
            CreateResultForDiedConnection(reply);
            return;
        }
        if (!processingApi_.empty()) {
            CreateResultForConcurrentInvoke(processingApi_, call.apiId_, reply);
            return;
        }
        processingApi_ = call.apiId_;
        stateLock.unlock(); // unlock, allow reentry, make it possible to check and reject concurrent usage
        transceiver_->EmitCall(MarshalApiCall(call));
        while (true) {
            TransactionMessage message;
            auto status = transceiver_->PollCallReply(message, WAIT_TRANSACTION_MS);
            switch (status) {
                case MessageTransceiver::PollStatus::SUCCESS:
                    DCHECK(message.type_ == TransactionType::REPLY);
                    stateLock.lock();
                    processingApi_.clear();
                    stateLock.unlock();
                    UnmarshalApiReply(reply, message.dataParcel_);
                    return;
                case MessageTransceiver::PollStatus::ABORT_CONNECTION_DIED:
                case MessageTransceiver::PollStatus::ABORT_REQUEST_EXIT:
                    stateLock.lock();
                    connectionDied_ = true;
                    stateLock.unlock();
                    CreateResultForDiedConnection(reply);
                    return;
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