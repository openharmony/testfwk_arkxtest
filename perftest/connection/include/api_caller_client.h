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

#ifndef API_CALLER_CLIENT_H
#define API_CALLER_CLIENT_H

#include <string>
#include <string_view>
#include <functional>
#include <common_event_subscribe_info.h>
#include <common_event_manager.h>
#include "ipc_transactor.h"


namespace OHOS::perftest {
    using namespace std;
    using namespace OHOS::AAFwk;
    using namespace OHOS::EventFwk;

    /**Represents the api transaction participant client.*/
    class ApiCallerClient {
    public:
        ApiCallerClient() {};
        ~ApiCallerClient();
        bool InitAndConnectPeer(string_view token, ApiCallHandler handler);
        void Transact(const ApiCallInfo &call, ApiReplyInfo &reply);
        void SetDeathCallback(function<void()> callback);
        ConnectionStat GetConnectionStat() const;
    private:
        sptr<IRemoteObject> WaitForPublishedCaller(string_view token);
        void OnPeerDeath();

    private:
        ConnectionStat connectState_ = UNINIT;
        // for concurrent invocation detect
        string processingApi_ = "";
        // ipc objects
        sptr<ApiCallerStub> caller_ = nullptr;
        sptr<ApiCallerProxy> remoteCaller_ = nullptr;
        sptr<OHOS::IRemoteObject::DeathRecipient> peerDeathCallback_ = nullptr;
        function<void()> onDeathCallback_ = nullptr;
    };

    using CommonEventHandler = function<void(const CommonEventData &)>;
    class CommonEventForwarder : public CommonEventSubscriber {
    public:
        explicit CommonEventForwarder(const CommonEventSubscribeInfo &info, CommonEventHandler handler)
            : CommonEventSubscriber(info), handler_(handler) {}
        virtual ~CommonEventForwarder() {}
        void UpdateHandler(CommonEventHandler handler);
        void OnReceiveEvent(const CommonEventData &data) override;

    private:
        CommonEventHandler handler_ = nullptr;
    };
} // namespace OHOS::perftest

#endif