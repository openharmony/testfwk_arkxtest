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

#ifndef IPC_TRANSACTOR_H
#define IPC_TRANSACTOR_H

#include <iremote_broker.h>
#include <iremote_stub.h>
#include <iremote_proxy.h>
#include <string>
#include <string_view>
#include <functional>
#include <future>
#include "frontend_api_defines.h"

namespace OHOS::uitest {
    class IApiCaller : public OHOS::IRemoteBroker {
    public:
        DECLARE_INTERFACE_DESCRIPTOR(u"ohos.uitest.IApiCaller");
        const uint32_t TRANS_ID_CALL = 1;
        const uint32_t TRANS_ID_SET_BACKCALLER = 2;
        // call api specified in call and receive result in reply
        virtual void Call(const ApiCallInfo &call, ApiReplyInfo &result) = 0;
        // set backcaller for callback usage
        virtual bool SetBackCaller(const OHOS::sptr<OHOS::IRemoteObject> &caller) = 0;
    };

    using ApiCallHandler = std::function<void(const ApiCallInfo &, ApiReplyInfo &)>;
    class ApiCaller : public OHOS::IRemoteStub<IApiCaller> {
    public:
        explicit ApiCaller() = default;
        virtual ~ApiCaller() = default;
        virtual int OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data,
                    OHOS::MessageParcel &reply, OHOS::MessageOption &option) override;
        void Call(const ApiCallInfo &call, ApiReplyInfo &result) override;
        bool SetBackCaller(const OHOS::sptr<IRemoteObject> &caller) override;
        // set functions which do api-invocation and backcaller handling
        void SetCallHandler(ApiCallHandler handler);
        void SetBackCallerHandler(std::function<void(OHOS::sptr<OHOS::IRemoteObject>)> handler);

    private:
        ApiCallHandler handler_ = nullptr;
        std::function<void(OHOS::sptr<OHOS::IRemoteObject>)> backcallerHandler_ = nullptr;
    };

    class ApiCallerProxy : public OHOS::IRemoteProxy<IApiCaller> {
    public:
        explicit ApiCallerProxy(const OHOS::sptr<OHOS::IRemoteObject> &impl);
        virtual ~ApiCallerProxy()  = default;
        void Call(const ApiCallInfo &call, ApiReplyInfo &result) override;
        bool SetBackCaller(const OHOS::sptr<IRemoteObject> &caller) override;
        bool SetRemoteDeathCallback(const sptr<OHOS::IRemoteObject::DeathRecipient> &callback);
        bool UnsetRemoteDeathCallback(const sptr<OHOS::IRemoteObject::DeathRecipient> &callback);

    private:
        static inline OHOS::BrokerDelegator<ApiCallerProxy> delegator_;
    };

    /**Represents the api transaction participant(client/server).*/
    enum ConnectionStat : uint8_t { UNINIT, CONNECTED, DISCONNECTED };
    class ApiTransactor {
    public:
        ApiTransactor() = delete;
        explicit ApiTransactor(bool asServer);
        ~ApiTransactor();
        bool InitAndConnectPeer(std::string_view token, ApiCallHandler handler);
        void Finalize();
        void Transact(const ApiCallInfo &call, ApiReplyInfo &reply);
        void SetDeathCallback(std::function<void()> callback);
        ConnectionStat GetConnectionStat() const;

    private:
        const bool asServer_ = false;
        ConnectionStat connectState_ = UNINIT;
        // for concurrent invocation detect
        std::string processingApi_ = "";
        sptr<ApiCaller> caller_ = nullptr;
        // ipc objects
        sptr<ApiCallerProxy> remoteCaller_ = nullptr;
        sptr<OHOS::IRemoteObject::DeathRecipient> peerDeathCallback_ = nullptr;
        std::function<void()> onDeathCallback_ = nullptr;
        void OnPeerDeath();
    };
} // namespace OHOS::uitest

#endif