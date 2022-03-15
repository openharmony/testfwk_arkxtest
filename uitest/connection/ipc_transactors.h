/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#ifndef IPC_TRANSACTORS_H
#define IPC_TRANSACTORS_H

#include <string_view>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <utility>

namespace OHOS::uitest {
    enum TransactionType : uint8_t {
        INVALID, CALL, REPLY, HANDSHAKE, ACK, EXIT
    };

    /**Represents the api invocation call/reply message.*/
    struct TransactionMessage {
        uint32_t id_;
        TransactionType type_;
        std::string apiId_;
        std::string callerParcel_;
        std::string paramsParcel_;
        std::string resultParcel_;
    };

    /**Api request/reply message transceiver.*/
    class MessageTransceiver {
    public:
        enum PollStatus : uint8_t {
            SUCCESS,
            ABORT_WAIT_TIMEOUT,
            ABORT_CONNECTION_DIED,
            ABORT_REQUEST_EXIT
        };

        virtual ~MessageTransceiver() = default;

        virtual bool Initialize() = 0;

        void OnReceiveMessage(const TransactionMessage &message);

        void EmitCall(std::string_view apiId, std::string_view caller, std::string_view params);

        void EmitReply(const TransactionMessage &request, std::string_view reply);

        void EmitHandshake();

        void EmitAck(const TransactionMessage &handshake);

        void EmitExit();

        void SetMessageFilter(std::function<bool(TransactionType)>);

        PollStatus PollCallReply(TransactionMessage &out, uint64_t timeoutMs);

        void ScheduleCheckConnection(bool emitHandshake);

        bool EnsureConnectionAlive(uint64_t timeoutMs);

        virtual void Finalize();

    protected:
        virtual void DoEmitMessage(const TransactionMessage &message) = 0;

    private:
        void EmitMessage(const TransactionMessage &message);

        static constexpr uint32_t FLAG_CONNECT_DIED = 1 << 0;
        static constexpr uint32_t FLAG_REQUEST_EXIT = 1 << 1;
        std::function<bool(TransactionType)> messageFilter_;
        std::atomic<bool> autoHandshaking_ = false;
        std::future<void> handshakeFuture_;
        std::atomic<uint32_t> extraFlags_ = 0;
        std::atomic<uint64_t> lastIncomingMessageMillis_ = 0;
        std::atomic<uint64_t> lastOutgoingMessageMillis_ = 0;
        std::condition_variable busyCond_;
        std::mutex queueLock_;
        std::queue<TransactionMessage> messageQueue_;
    };

    /**Represents the notify-alive timeout between client and server (2s).*/
    constexpr uint64_t WATCH_DOG_TIMEOUT_MS = 2000;

    /**Represents the api transaction participant(client/server).*/
    class Transactor {
    public:
        virtual bool Initialize();

        virtual void Finalize();

    protected:
        virtual std::unique_ptr<MessageTransceiver> CreateTransceiver() = 0;

        virtual std::function<bool(TransactionType)> GetMessageFilter()
        {
            return nullptr;
        }

        std::unique_ptr<MessageTransceiver> transceiver_;
        static constexpr uint32_t EXIT_CODE_SUCCESS = 0;
        static constexpr uint32_t EXIT_CODE_FAILURE = 1;
        static constexpr uint32_t WAIT_TRANSACTION_MS = WATCH_DOG_TIMEOUT_MS / 100;
    };

    /**Represents the api transaction server.*/
    class TransactionServer : public Transactor {
    public:
        uint32_t RunLoop();

        void SetCallFunction(std::function<std::string(std::string_view, std::string_view, std::string_view)> func);

    private:
        // use function pointer here to keep independent from 'core', which are not wanted by the client-side
        std::function<std::string(std::string_view, std::string_view, std::string_view)> callFunc_;
    };

    /**Represents the api transaction client.*/
    class TransactionClient : public Transactor {
    public:
        std::string InvokeApi(std::string_view apiId, std::string_view caller, std::string_view params);

        /**Finalize both self side and server side.*/
        void Finalize() override;

    private:
        std::mutex stateMtx_;
        std::string processingApi_;
        bool connectionDied_ = false;
    };
}

#endif