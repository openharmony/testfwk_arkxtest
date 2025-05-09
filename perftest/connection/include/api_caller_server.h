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

#ifndef API_CALLER_SERVER_H
#define API_CALLER_SERVER_H

#include <string>
#include <string_view>
#include <functional>
#include <future>
#include "frontend_api_defines.h"
#include "ipc_transactor.h"

namespace OHOS::perftest {
    /**Represents the api transaction server.*/
    class ApiCallerServer {
    public:
        ApiCallerServer() {};
        ~ApiCallerServer();
        bool InitAndConnectPeer(std::string_view token, ApiCallHandler handler);
        void Transact(const ApiCallInfo &call, ApiReplyInfo &reply);
        void SetDeathCallback(std::function<void()> callback);
        ConnectionStat GetConnectionStat() const;
        void Finalize();
    private:
        sptr<IRemoteObject> PublishCallerAndWaitForBackcaller(const sptr<ApiCallerStub> &caller, string_view token);
        void OnPeerDeath();

    private:
        ConnectionStat connectState_ = UNINIT;
        // ipc objects
        sptr<ApiCallerStub> caller_ = nullptr;
        sptr<ApiCallerProxy> remoteCaller_ = nullptr;
        sptr<OHOS::IRemoteObject::DeathRecipient> peerDeathCallback_ = nullptr;
        std::function<void()> onDeathCallback_ = nullptr;
    };
} // namespace OHOS::perftest

#endif