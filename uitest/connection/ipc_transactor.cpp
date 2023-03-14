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

#include <common_event_manager.h>
#include <common_event_subscribe_info.h>
#include <functional>
#include <future>
#include <unistd.h>
#include "common_utilities_hpp.h"
#include "json.hpp"
#include "ipc_transactor.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace chrono;
    using namespace nlohmann;
    using namespace OHOS;
    using namespace OHOS::AAFwk;
    using namespace OHOS::EventFwk;
    using Message = MessageParcel &;

    using CommonEventHandler = function<void(const CommonEventData &)>;
    class CommonEventForwarder : public CommonEventSubscriber {
    public:
        explicit CommonEventForwarder(const CommonEventSubscribeInfo &info, CommonEventHandler handler)
            : CommonEventSubscriber(info), handler_(handler)
        {
        }

        virtual ~CommonEventForwarder() {}

        void UpdateHandler(CommonEventHandler handler)
        {
            handler_ = handler;
        }

        void OnReceiveEvent(const CommonEventData &data) override
        {
            if (handler_ != nullptr) {
                handler_(data);
            }
        }

    private:
        CommonEventHandler handler_ = nullptr;
    };

    using RemoteDiedHandler = function<void()>;
    class DeathRecipientForwarder : public IRemoteObject::DeathRecipient {
    public:
        explicit DeathRecipientForwarder(RemoteDiedHandler handler) : handler_(handler) {};
        ~DeathRecipientForwarder() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override
        {
            if (handler_ != nullptr) {
                handler_();
            }
        }

    private:
        const RemoteDiedHandler handler_;
    };

    int ApiCaller::OnRemoteRequest(uint32_t code, Message data, Message reply, MessageOption &option)
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
            call.fdParamIndex_ = data.ReadInt32();
            call.paramList_ = nlohmann::json::parse(paramListStr, nullptr, false);
            DCHECK(!call.paramList_.is_discarded());
            if (call.fdParamIndex_ >= 0) {
                call.paramList_.at(call.fdParamIndex_) = data.ReadFileDescriptor();
            }
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

    void ApiCaller::Call(const ApiCallInfo &call, ApiReplyInfo &result)
    {
        DCHECK(handler_ != nullptr);
        handler_(call, result);
    }

    bool ApiCaller::SetBackCaller(const sptr<IRemoteObject> &caller)
    {
        if (backcallerHandler_ == nullptr) {
            LOG_W("No backcallerHandler set!");
            return false;
        }
        backcallerHandler_(caller);
        return true;
    }

    void ApiCaller::SetCallHandler(ApiCallHandler handler)
    {
        handler_ = handler;
    }

    void ApiCaller::SetBackCallerHandler(function<void(sptr<IRemoteObject>)> handler)
    {
        backcallerHandler_ = handler;
    }

    ApiCallerProxy::ApiCallerProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IApiCaller>(impl) {}

    void ApiCallerProxy::Call(const ApiCallInfo &call, ApiReplyInfo &result)
    {
        MessageOption option;
        MessageParcel data, reply;
        // IPC io: verify on write
        auto ret = data.WriteInterfaceToken(GetDescriptor()) && data.WriteString(call.apiId_) &&
                   data.WriteString(call.callerObjRef_) && data.WriteString(call.paramList_.dump()) &&
                   data.WriteInt32(call.fdParamIndex_);
        auto fdIndex = call.fdParamIndex_;
        if (ret && fdIndex >= 0) {
            DCHECK(static_cast<size_t>(fdIndex) < call.paramList_.size());
            DCHECK(call.paramList_.at(fdIndex).type() == nlohmann::detail::value_t::number_unsigned);
            if (!data.WriteFileDescriptor(call.paramList_.at(fdIndex).get<uint32_t>())) {
                ret = false;
                LOG_E("Failed to write file descriptor param");
            }
        }
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
        MessageParcel data, reply;
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

    constexpr string_view PUBLISH_EVENT_PREFIX = "uitest.api.caller.publish#";
    constexpr uint32_t PUBLISH_MAX_RETIES = 10;
    constexpr uint32_t WAIT_CONN_TIMEOUT_MS = 5000;

    static sptr<IRemoteObject> PublishCallerAndWaitForBackcaller(const sptr<ApiCaller> &caller, string_view token)
    {
        CommonEventData event;
        Want want;
        want.SetAction(string(PUBLISH_EVENT_PREFIX) + token.data());
        want.SetParam(string(token), caller->AsObject());
        event.SetWant(want);
        // wait backcaller object registeration from client
        mutex mtx;
        unique_lock<mutex> lock(mtx);
        condition_variable condition;
        sptr<IRemoteObject> remoteCallerObject = nullptr;
        caller->SetBackCallerHandler([&remoteCallerObject, &condition](const sptr<IRemoteObject> &remote) {
            remoteCallerObject = remote;
            condition.notify_one();
        });
        constexpr auto period = chrono::milliseconds(WAIT_CONN_TIMEOUT_MS / PUBLISH_MAX_RETIES);
        uint32_t tries = 0;
        do {
            // publish caller with retries
            if (!CommonEventManager::PublishCommonEvent(event)) {
                LOG_E("Pulbish commonEvent failed");
            }
            tries++;
        } while (tries < PUBLISH_MAX_RETIES && condition.wait_for(lock, period) == cv_status::timeout);
        caller->SetBackCallerHandler(nullptr);
        return remoteCallerObject;
    }

    static sptr<IRemoteObject> WaitForPublishedCaller(string_view token)
    {
        MatchingSkills matchingSkills;
        matchingSkills.AddEvent(string(PUBLISH_EVENT_PREFIX) + token.data());
        CommonEventSubscribeInfo info(matchingSkills);
        mutex mtx;
        unique_lock<mutex> lock(mtx);
        condition_variable condition;
        sptr<IRemoteObject> remoteObject = nullptr;
        auto onEvent = [&condition, &remoteObject, &token](const CommonEventData &data) {
            LOG_D("Received commonEvent");
            const auto &want = data.GetWant();
            remoteObject = want.GetRemoteObject(string(token));
            if (remoteObject == nullptr || !remoteObject->IsProxyObject()) {
                LOG_W("Not a proxy object!");
                remoteObject = nullptr;
            } else {
                condition.notify_one();
            }
        };
        shared_ptr<CommonEventForwarder> subscriber = make_shared<CommonEventForwarder>(info, onEvent);
        if (!CommonEventManager::SubscribeCommonEvent(subscriber)) {
            LOG_E("Fail to subscribe commonEvent");
            return nullptr;
        }
        const auto timeout = chrono::milliseconds(WAIT_CONN_TIMEOUT_MS);
        if (condition.wait_for(lock, timeout) == cv_status::timeout) {
            LOG_E("Wait for ApiCaller publish by server timeout");
        } else if (remoteObject == nullptr) {
            LOG_E("Published ApiCaller object is null");
        }
        subscriber->UpdateHandler(nullptr); // unset handler
        CommonEventManager::UnSubscribeCommonEvent(subscriber);
        return remoteObject;
    }

    ApiTransactor::ApiTransactor(bool asServer) : asServer_(asServer) {};

    void ApiTransactor::SetDeathCallback(function<void()> callback)
    {
        onDeathCallback_ = callback;
    }

    void ApiTransactor::OnPeerDeath()
    {
        LOG_W("Connection with peer died!");
        connectState_ = DISCONNECTED;
        if (onDeathCallback_ != nullptr) {
            onDeathCallback_();
        }
    }

    ApiTransactor::~ApiTransactor()
    {
        if (connectState_ == UNINIT) {
            return;
        }
        if (remoteCaller_ != nullptr && peerDeathCallback_ != nullptr) {
            remoteCaller_->UnsetRemoteDeathCallback(peerDeathCallback_);
        }
        caller_ = nullptr;
        remoteCaller_ = nullptr;
        peerDeathCallback_ = nullptr;
    }

    bool ApiTransactor::InitAndConnectPeer(string_view token, ApiCallHandler handler)
    {
        LOG_I("Begin");
        DCHECK(connectState_ == UNINIT);
        connectState_ = DISCONNECTED;
        caller_ = new ApiCaller();
        caller_->SetCallHandler(handler);
        if (asServer_) {
            // public caller object, and wait for backcaller registration from client
            auto remoteObject = PublishCallerAndWaitForBackcaller(caller_, token);
            if (remoteObject != nullptr) {
                remoteCaller_ = new ApiCallerProxy(remoteObject);
            }
        } else {
            // wait for published caller object, then register backcaller to server
            auto remoteObject = WaitForPublishedCaller(token);
            if (remoteObject != nullptr) {
                remoteCaller_ = new ApiCallerProxy(remoteObject);
                if (!remoteCaller_->SetBackCaller(caller_)) {
                    LOG_E("Failed to set backcaller to server");
                    return false;
                }
            }
        }
        if (remoteCaller_ == nullptr) {
            LOG_E("Failed to get apiCaller object from peer");
            return false;
        }
        // link connectionState to it to remoteCaller
        peerDeathCallback_ = new DeathRecipientForwarder([this]() { this->OnPeerDeath(); });
        if (!remoteCaller_->SetRemoteDeathCallback(peerDeathCallback_)) {
            LOG_E("Failed to register remote caller DeathRecipient");
            return false;
        }
        // connect done
        connectState_ = CONNECTED;
        LOG_I("Done");
        return true;
    }

    ConnectionStat ApiTransactor::GetConnectionStat() const
    {
        return connectState_;
    }

    void ApiTransactor::Finalize() {}

    void ApiTransactor::Transact(const ApiCallInfo &call, ApiReplyInfo &reply)
    {
        // check connection state
        DCHECK(connectState_ != UNINIT);
        if (connectState_ == DISCONNECTED) {
            reply.exception_ = ApiCallErr(ERR_INTERNAL, "ipc connection is dead");
            return;
        }
        // check concurrent call
        if (!processingApi_.empty()) {
            constexpr auto msg = "uitest-api dose not allow calling concurrently, current processing:";
            reply.exception_.code_ = ERR_API_USAGE;
            reply.exception_.message_ = string(msg) + processingApi_ + ", incoming: " + call.apiId_;
            return;
        }
        processingApi_ = call.apiId_;
        // forward to peer
        DCHECK(remoteCaller_ != nullptr);
        remoteCaller_->Call(call, reply);
        processingApi_.clear();
    }
} // namespace OHOS::uitest