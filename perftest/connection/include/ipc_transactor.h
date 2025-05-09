/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <functional>
#include "common_utils.h"
#include "frontend_api_defines.h"

namespace OHOS::perftest {
    using namespace std;

    class IApiCaller : public OHOS::IRemoteBroker {
    public:
        DECLARE_INTERFACE_DESCRIPTOR(u"ohos.perftest.IApiCaller");
        const uint32_t TRANS_ID_CALL = 1;
        const uint32_t TRANS_ID_SET_BACKCALLER = 2;
        // call api specified in call and receive result in reply
        virtual void Call(const ApiCallInfo &call, ApiReplyInfo &result) = 0;
        // set backcaller for callback usage
        virtual bool SetBackCaller(const OHOS::sptr<OHOS::IRemoteObject> &caller) = 0;
    };

    using ApiCallHandler = function<void(const ApiCallInfo &, ApiReplyInfo &)>;
    class ApiCallerStub : public OHOS::IRemoteStub<IApiCaller> {
    public:
        explicit ApiCallerStub() = default;
        virtual ~ApiCallerStub() = default;
        virtual int OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data,
                    OHOS::MessageParcel &reply, OHOS::MessageOption &option) override;
        void Call(const ApiCallInfo &call, ApiReplyInfo &result) override;
        bool SetBackCaller(const OHOS::sptr<IRemoteObject> &caller) override;
        // set functions which do api-invocation and backcaller handling
        void SetCallHandler(ApiCallHandler handler);
        void SetBackCallerHandler(function<void(OHOS::sptr<OHOS::IRemoteObject>)> handler);

    private:
        ApiCallHandler handler_ = nullptr;
        function<void(OHOS::sptr<OHOS::IRemoteObject>)> backcallerHandler_ = nullptr;
    };

    class ApiCallerProxy : public OHOS::IRemoteProxy<IApiCaller> {
    public:
        explicit ApiCallerProxy(const OHOS::sptr<OHOS::IRemoteObject> &impl);
        virtual ~ApiCallerProxy()  = default;
        void Call(const ApiCallInfo &call, ApiReplyInfo &result) override;
        bool SetBackCaller(const OHOS::sptr<IRemoteObject> &caller) override;
        bool SetRemoteDeathCallback(const sptr<OHOS::IRemoteObject::DeathRecipient> &callback);
        bool UnsetRemoteDeathCallback(const sptr<OHOS::IRemoteObject::DeathRecipient> &callback);
    };

    using RemoteDiedHandler = function<void()>;
    class DeathRecipientForwarder : public IRemoteObject::DeathRecipient {
    public:
        explicit DeathRecipientForwarder(RemoteDiedHandler handler) : handler_(handler) {};
        ~DeathRecipientForwarder() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

    private:
        const RemoteDiedHandler handler_;
    };
} // namespace OHOS::perftest

#endif