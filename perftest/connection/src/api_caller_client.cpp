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
#include "common_utils.h"
#include "api_caller_client.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace chrono;
    using namespace OHOS;
    using namespace OHOS::AAFwk;
    using namespace OHOS::EventFwk;

    ApiCallerClient::~ApiCallerClient()
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

    bool ApiCallerClient::InitAndConnectPeer(string_view token, ApiCallHandler handler)
    {
        LOG_I("InitAndConnectPeer Begin");
        if (connectState_ == CONNECTED) {
            LOG_I("InitAndConnectPeer, client has connected with server");
            return true;
        }
        DCHECK(connectState_ == UNINIT);
        connectState_ = DISCONNECTED;
        caller_ = new ApiCallerStub();
        caller_->SetCallHandler(handler);
        // wait for published caller object, then register backcaller to server
        sptr<IRemoteObject> remoteObject = WaitForPublishedCaller(token);
        if (remoteObject != nullptr) {
            remoteCaller_ = new ApiCallerProxy(remoteObject);
            if (!remoteCaller_->SetBackCaller(caller_)) {
                LOG_E("Failed to set backcaller to server");
                return false;
            }
        }
        if (remoteObject == nullptr || remoteCaller_ == nullptr) {
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
        LOG_I("ApiCallerClient InitAndConnectPeer Done");
        return true;
    }

    void ApiCallerClient::Transact(const ApiCallInfo &call, ApiReplyInfo &reply)
    {
        // check connection state
        DCHECK(connectState_ != UNINIT);
        if (connectState_ == DISCONNECTED) {
            reply.exception_ = ApiCallErr(ERR_INTERNAL, "ipc connection is dead");
            return;
        }
        DCHECK(remoteCaller_ != nullptr);
        // check concurrent call
        if (!processingApi_.empty()) {
            constexpr auto msg = "perftest-api dose not allow calling concurrently, current processing:";
            reply.exception_.code_ = ERR_API_USAGE;
            reply.exception_.message_ = string(msg) + processingApi_ + ", incoming: " + call.apiId_;
            return;
        }
        processingApi_ = call.apiId_;
        // forward to peer
        remoteCaller_->Call(call, reply);
        processingApi_.clear();
    }

    void ApiCallerClient::SetDeathCallback(function<void()> callback)
    {
        onDeathCallback_ = callback;
    }

    ConnectionStat ApiCallerClient::GetConnectionStat() const
    {
        return connectState_;
    }

    sptr<IRemoteObject> ApiCallerClient::WaitForPublishedCaller(string_view token)
    {
        MatchingSkills matchingSkills;
        matchingSkills.AddEvent(string(PUBLISH_EVENT_PREFIX) + token.data());
        CommonEventSubscribeInfo info(matchingSkills);
        mutex mtx;
        unique_lock<mutex> lock(mtx);
        condition_variable condition;
        sptr<IRemoteObject> remoteObject = nullptr;
        auto onEvent = [&condition, &remoteObject, &token](const CommonEventData &data) {
            LOG_I("Received commonEvent");
            const auto &want = data.GetWant();
            remoteObject = want.GetRemoteObject(string(token));
            if (remoteObject == nullptr) {
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
        auto ret = condition.wait_for(lock, timeout);
        CommonEventManager::UnSubscribeCommonEvent(subscriber);
        subscriber->UpdateHandler(nullptr); // unset handler
        if (ret == cv_status::timeout) {
            LOG_E("Wait for ApiCaller publish by server timeout");
        } else if (remoteObject == nullptr) {
            LOG_E("Published ApiCaller object is null");
        }
        return remoteObject;
    }

    void ApiCallerClient::OnPeerDeath()
    {
        LOG_W("Connection with peer died!");
        connectState_ = DISCONNECTED;
        if (onDeathCallback_ != nullptr) {
            onDeathCallback_();
        }
    }

    void CommonEventForwarder::UpdateHandler(CommonEventHandler handler)
    {
        handler_ = handler;
    }

    void CommonEventForwarder::OnReceiveEvent(const CommonEventData &data)
    {
        if (handler_ != nullptr) {
            handler_(data);
        }
    }
} // namespace OHOS::perftest