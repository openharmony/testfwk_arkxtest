/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "start_test_server.h"

#include "iremote_broker.h"
#include "hilog/log.h"
#include "iremote_broker.h"
#include "iservice_registry.h"
#include "test_server_interface_proxy.h"
#include "system_ability_definition.h"

namespace OHOS::testserver {
    static constexpr int32_t TESTSERVER_LOAD_TIMEOUT_MS = 1000; // ms

    StartTestServer &StartTestServer::GetInstance()
    {
        static StartTestServer startTestServer;
        return startTestServer;
    }

    class TestServerLoadCallback : public SystemAbilityLoadCallbackStub {
    public:
        explicit TestServerLoadCallback(int32_t systemAbilityId) : systemAbilityId_(systemAbilityId){};

        void InitLoadState()
        {
            std::unique_lock<std::mutex> lock(locatorMutex_);
            loadState_ = false;
        }

        bool WaitLoadStateChange(int32_t systemAbilityId)
        {
            std::unique_lock<std::mutex> lock(locatorMutex_);
            auto wait = locatorCond_.wait_for(lock, std::chrono::milliseconds(TESTSERVER_LOAD_TIMEOUT_MS), [this] {
                return loadState_ == true;
            });
            if (!wait) {
                return false;
            }
            return true;
        }

        sptr<IRemoteObject> GetTestServerObject()
        {
            return remoteObject_;
        }

    private:
        void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject) override
        {
            if (systemAbilityId == systemAbilityId_) {
                std::unique_lock<std::mutex> lock(locatorMutex_);
                loadState_ = true;
                remoteObject_ = remoteObject;
                locatorCond_.notify_one();
            }
        }

        void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
        {
            std::unique_lock<std::mutex> lock(locatorMutex_);
            loadState_ = false;
            locatorCond_.notify_one();
        }

        int32_t systemAbilityId_;
        std::condition_variable locatorCond_;
        std::mutex locatorMutex_;
        bool loadState_ = false;
        sptr<IRemoteObject> remoteObject_ = nullptr;
    };

    sptr<ITestServerInterface> StartTestServer::LoadTestServer()
    {
        const int32_t systemAbilityId = TEST_SERVER_SA_ID;
        sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            return nullptr;
        }
        auto object = samgr->CheckSystemAbility(systemAbilityId);
        if (object != nullptr) {
            remoteObject_ = object;
        } else {
            auto testServerLoadCallback = sptr<TestServerLoadCallback>(new TestServerLoadCallback(systemAbilityId));
            testServerLoadCallback->InitLoadState();
            int32_t ret = samgr->LoadSystemAbility(systemAbilityId, testServerLoadCallback);
            if (ret != ERR_NONE) {
                return nullptr;
            }
            if (testServerLoadCallback->WaitLoadStateChange(systemAbilityId)) {
                remoteObject_ = testServerLoadCallback->GetTestServerObject();
            }
        }
        if (remoteObject_ == nullptr) {
            return nullptr;
        }
        sptr<ITestServerInterface> iTestServerInterface = iface_cast<TestServerInterfaceProxy>(remoteObject_);
        if (iTestServerInterface == nullptr) {
            return nullptr;
        }
        return iTestServerInterface;
    }

} // namespace OHOS::testserver