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

#ifndef IPC_TRANSACTORS_IMPL_H
#define IPC_TRANSACTORS_IMPL_H

#include <string_view>
#include "ipc_transactors.h"
#include "common_event_manager.h"

namespace OHOS::uitest {
    /**Transceiver implemented basing on ohos CommonEvent.*/
    class TransactionTransceiverImpl : public MessageTransceiver {
    public:
        TransactionTransceiverImpl() = delete;

        explicit TransactionTransceiverImpl(std::string_view token, bool asServer);

        bool Initialize() override;

        void Finalize() override;

    protected:
        void DoEmitMessage(const TransactionMessage &message) override;

    private:
        const bool asServer_;
        const std::string token_;
        std::shared_ptr<OHOS::EventFwk::CommonEventSubscriber> subscriber_;
    };

    /**Implementation TransactionServer with token.*/
    class TransactionServerImpl : public TransactionServer {
    public:
        explicit TransactionServerImpl(std::string_view token);

        virtual ~TransactionServerImpl();

        bool Initialize() override;

    protected:
        std::unique_ptr<MessageTransceiver> CreateTransceiver() override;

    private:
        const std::string token_;
    };

    /**Implementation TransactionClient with token.*/
    class TransactionClientImpl final : public TransactionClient {
    public:
        explicit TransactionClientImpl(std::string_view token);

        virtual ~TransactionClientImpl();

        bool Initialize() override;

    protected:
        std::unique_ptr<MessageTransceiver> CreateTransceiver() override;

    private:
        const std::string token_;
    };
}

#endif