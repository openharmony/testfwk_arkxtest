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

#include <common_event_manager.h>
#include <functional>
#include <future>
#include <unistd.h>
#include <want.h>
#include "common_utils.h"
#include "test_server_client.h"
#include "api_caller_server.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace chrono;
    using namespace OHOS;
    using namespace OHOS::AAFwk;
    using namespace OHOS::EventFwk;

    ApiCallerServer::~ApiCallerServer()
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

    bool ApiCallerServer::InitAndConnectPeer(string_view token, ApiCallHandler handler)
    {
        LOG_I("ApiCallerServer InitAndConnectPeer Begin");
        DCHECK(connectState_ == UNINIT);
        connectState_ = DISCONNECTED;
        caller_ = new ApiCallerStub();
        caller_->SetCallHandler(handler);
        sptr<IRemoteObject> remoteObject = nullptr;
        // public caller object, and wait for backcaller registration from client
        remoteObject = PublishCallerAndWaitForBackcaller(caller_, token);
        if (remoteObject != nullptr) {
            remoteCaller_ = new ApiCallerProxy(remoteObject);
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
        LOG_I("ApiCallerServer InitAndConnectPeer Done");
        return true;
    }

    void ApiCallerServer::Transact(const ApiCallInfo &call, ApiReplyInfo &reply)
    {
        // check connection state
        DCHECK(connectState_ != UNINIT);
        if (connectState_ == DISCONNECTED) {
            reply.exception_ = ApiCallErr(ERR_INTERNAL, "ipc connection is dead");
            return;
        }
        // forward to peer
        DCHECK(remoteCaller_ != nullptr);
        remoteCaller_->Call(call, reply);
    }

    void ApiCallerServer::SetDeathCallback(function<void()> callback)
    {
        onDeathCallback_ = callback;
    }

    ConnectionStat ApiCallerServer::GetConnectionStat() const
    {
        return connectState_;
    }

    void ApiCallerServer::Finalize() {}
    
    sptr<IRemoteObject> ApiCallerServer::PublishCallerAndWaitForBackcaller(const sptr<ApiCallerStub> &caller,
                                                                           string_view token)
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
            if (!OHOS::testserver::TestServerClient::GetInstance().PublishCommonEvent(event)) {
                LOG_E("Pulbish commonEvent failed");
            }
            tries++;
        } while (tries < PUBLISH_MAX_RETIES && condition.wait_for(lock, period) == cv_status::timeout);
        caller->SetBackCallerHandler(nullptr);
        return remoteCallerObject;
    }

    void ApiCallerServer::OnPeerDeath()
    {
        LOG_W("Connection with peer died!");
        connectState_ = DISCONNECTED;
        if (onDeathCallback_ != nullptr) {
            onDeathCallback_();
        }
    }
} // namespace OHOS::perftest