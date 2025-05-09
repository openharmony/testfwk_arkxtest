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

#include <unistd.h>
#include "nlohmann/json.hpp"
#include "ipc_transactor.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace nlohmann;
    using namespace OHOS;
    using Message = MessageParcel &;

    int ApiCallerStub::OnRemoteRequest(uint32_t code, Message data, Message reply, MessageOption &option)
    {
        if (data.ReadInterfaceToken() != GetDescriptor()) {
            return -1;
        }
        if (code == TRANS_ID_CALL) {
            // IPC io: verify on write
            ApiCallInfo call;
            string paramListStr;
            call.apiId_ = data.ReadString();
            call.callerObjRef_ = data.ReadString();
            paramListStr = data.ReadString();
            call.paramList_ = json::parse(paramListStr, nullptr, false);
            DCHECK(!call.paramList_.is_discarded());
            ApiReplyInfo result;
            Call(call, result);
            auto ret = reply.WriteString(result.resultValue_.dump()) && reply.WriteUint32(result.exception_.code_) &&
                       reply.WriteString(result.exception_.message_);
            return ret ? 0 : -1;
        } else if (code == TRANS_ID_SET_BACKCALLER) {
            reply.WriteBool(SetBackCaller(data.ReadRemoteObject()));
            return 0;
        } else {
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }

    void ApiCallerStub::Call(const ApiCallInfo &call, ApiReplyInfo &result)
    {
        DCHECK(handler_ != nullptr);
        handler_(call, result);
    }

    bool ApiCallerStub::SetBackCaller(const sptr<IRemoteObject> &caller)
    {
        if (backcallerHandler_ == nullptr) {
            LOG_W("No backcallerHandler set!");
            return false;
        }
        backcallerHandler_(caller);
        return true;
    }

    void ApiCallerStub::SetCallHandler(ApiCallHandler handler)
    {
        handler_ = handler;
    }

    void ApiCallerStub::SetBackCallerHandler(function<void(sptr<IRemoteObject>)> handler)
    {
        backcallerHandler_ = handler;
    }

    ApiCallerProxy::ApiCallerProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IApiCaller>(impl) {}

    void ApiCallerProxy::Call(const ApiCallInfo &call, ApiReplyInfo &result)
    {
        MessageOption option;
        MessageParcel data;
        MessageParcel reply;
        // IPC io: verify on write
        auto ret = data.WriteInterfaceToken(GetDescriptor()) && data.WriteString(call.apiId_) &&
                   data.WriteString(call.callerObjRef_) && data.WriteString(call.paramList_.dump());
        if (!ret || Remote()->SendRequest(TRANS_ID_CALL, data, reply, option) != 0) {
            result.exception_ = ApiCallErr(ERR_INTERNAL, "IPC SendRequest failed");
            result.resultValue_ = nullptr;
        } else {
            result.resultValue_ = json::parse(reply.ReadString(), nullptr, false);
            DCHECK(!result.resultValue_.is_discarded());
            result.exception_.code_ = static_cast<ErrCode>(reply.ReadUint32());
            result.exception_.message_ = reply.ReadString();
        }
    }

    bool ApiCallerProxy::SetBackCaller(const OHOS::sptr<IRemoteObject> &caller)
    {
        MessageOption option;
        MessageParcel data;
        MessageParcel reply;
        auto writeStat = data.WriteInterfaceToken(GetDescriptor()) && data.WriteRemoteObject(caller);
        if (!writeStat || (Remote()->SendRequest(TRANS_ID_SET_BACKCALLER, data, reply, option) != 0)) {
            LOG_E("IPC SendRequest failed");
            return false;
        }
        return reply.ReadBool();
    }

    bool ApiCallerProxy::SetRemoteDeathCallback(const sptr<IRemoteObject::DeathRecipient> &callback)
    {
        return Remote()->AddDeathRecipient(callback);
    }

    bool ApiCallerProxy::UnsetRemoteDeathCallback(const sptr<OHOS::IRemoteObject::DeathRecipient> &callback)
    {
        return Remote()->RemoveDeathRecipient(callback);
    }

    void DeathRecipientForwarder::OnRemoteDied(const wptr<IRemoteObject> &remote)
    {
        if (handler_ != nullptr) {
            handler_();
        }
    }
} // namespace OHOS::perftest