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

#include "test_server_client.h"

#include "hilog/log.h"
#include "iremote_broker.h"
#include "iservice_registry.h"
#include "test_server_interface_proxy.h"
#include "system_ability_definition.h"
#include "session_token.h"
#include "test_server_error_code.h"
#include "system_ability_load_callback_stub.h"

namespace OHOS::testserver {
    using namespace OHOS::HiviewDFX;
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD003110, "TestServerClient"};
    static constexpr int32_t TESTSERVER_LOAD_TIMEOUT_MS = 4000; // ms

    TestServerClient &TestServerClient::GetInstance()
    {
        HiLog::Info(LABEL, "%{public}s called. ", __func__);
        static TestServerClient testServerClient;
        return testServerClient;
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
                HiLog::Error(LABEL, "%{public}s. Locator SystemAbility [%{public}d] time out.",
                             __func__, systemAbilityId);
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
            HiLog::Info(LABEL, "%{public}s. Load SystemAbility success, systemAbilityId = [%{public}d]",
                        __func__, systemAbilityId);
            if (systemAbilityId == systemAbilityId_) {
                std::unique_lock<std::mutex> lock(locatorMutex_);
                loadState_ = true;
                remoteObject_ = remoteObject;
                locatorCond_.notify_one();
            }
        }

        void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
        {
            HiLog::Info(LABEL, "%{public}s. Load SystemAbility failed, systemAbilityId = [%{public}d]",
                        __func__, systemAbilityId);
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

    TestServerClient::TestServerClient()
    {
        iTestServerInterface_ = LoadTestServer();
    }

    sptr<ITestServerInterface> TestServerClient::LoadTestServer()
    {
        const int32_t systemAbilityId = TEST_SERVER_SA_ID;
        HiLog::Info(LABEL, "%{public}s called. SystemAbility [%{public}d] loading", __func__, systemAbilityId);
        sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get SystemAbility Manager failed!", __func__);
            return nullptr;
        }
        auto object = samgr->CheckSystemAbility(systemAbilityId);
        if (object != nullptr) {
            HiLog::Info(LABEL, "%{public}s. CheckSystemAbility [%{public}d] SUCCESS", __func__, systemAbilityId);
            remoteObject_ = object;
        } else {
            auto testServerLoadCallback = sptr<TestServerLoadCallback>(new TestServerLoadCallback(systemAbilityId));
            testServerLoadCallback->InitLoadState();
            int32_t ret = samgr->LoadSystemAbility(systemAbilityId, testServerLoadCallback);
            if (ret != ERR_NONE) {
                HiLog::Error(LABEL, "%{public}s. LoadSystemAbility [%{public}d] FAILED, ret %{public}d",
                             __func__, systemAbilityId, ret);
                return nullptr;
            }
            if (testServerLoadCallback->WaitLoadStateChange(systemAbilityId)) {
                HiLog::Info(LABEL, "%{public}s. LoadSystemAbility [%{public}d] SUCCESS", __func__, systemAbilityId);
                remoteObject_ = testServerLoadCallback->GetTestServerObject();
            }
        }
        if (remoteObject_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get SystemAbility [%{public}d] remoteObject FAILED",
                         __func__, systemAbilityId);
            return nullptr;
        }
        sptr<ITestServerInterface> iTestServerInterface = iface_cast<TestServerInterfaceProxy>(remoteObject_);
        if (iTestServerInterface == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get SystemAbility [%{public}d] proxy FAILED", __func__, systemAbilityId);
            return nullptr;
        }
        sptr<SessionToken> sessionToken = new (std::nothrow) SessionToken();
        if (sessionToken == nullptr || iTestServerInterface->CreateSession(*sessionToken) != TEST_SERVER_OK) {
            HiLog::Error(LABEL, "%{public}s. Create session FAILED", __func__);
            return nullptr;
        }
        return iTestServerInterface;
    }

    int32_t TestServerClient::SetPasteData(std::string text)
    {
        HiLog::Info(LABEL, "%{public}s called.", __func__);
        if (iTestServerInterface_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
            return TEST_SERVER_GET_INTERFACE_FAILED;
        }
        return iTestServerInterface_->SetPasteData(text);
    }

    int32_t TestServerClient::ChangeWindowMode(int windowId, uint32_t mode)
    {
      HiLog::Info(LABEL, "%{public}s called.", __func__);
      if (iTestServerInterface_ == nullptr)
      {
        HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
        return TEST_SERVER_GET_INTERFACE_FAILED;
      }
      return iTestServerInterface_->ChangeWindowMode(windowId, mode);
    }

    int32_t TestServerClient::TerminateWindow(int windowId)
    {
      HiLog::Info(LABEL, "%{public}s called.", __func__);
      if (iTestServerInterface_ == nullptr)
      {
        HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
        return TEST_SERVER_GET_INTERFACE_FAILED;
      }
      return iTestServerInterface_->TerminateWindow(windowId);
    }

    int32_t TestServerClient::MinimizeWindow(int windowId)
    {
      HiLog::Info(LABEL, "%{public}s called.", __func__);
      if (iTestServerInterface_ == nullptr)
      {
        HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
        return TEST_SERVER_GET_INTERFACE_FAILED;
      }
      return iTestServerInterface_->MinimizeWindow(windowId);
    }

    bool TestServerClient::PublishCommonEvent(const EventFwk::CommonEventData &event)
    {
        HiLog::Info(LABEL, "%{public}s called.", __func__);
        if (iTestServerInterface_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
            return TEST_SERVER_GET_INTERFACE_FAILED;
        }
        bool result = false;
        auto ret = iTestServerInterface_->PublishCommonEvent(event, result);
        HiLog::Info(LABEL, "%{public}s ipc ret = %{public}d.", __func__, ret);
        return result;
    }

    void TestServerClient::FrequencyLock()
    {
        if (iTestServerInterface_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
            return;
        }
        iTestServerInterface_->FrequencyLock();
        return;
    }

    int32_t TestServerClient::SpDaemonProcess(int daemonCommand, std::string extraInfo)
    {
        if (iTestServerInterface_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
            return TEST_SERVER_GET_INTERFACE_FAILED;
        }

        return iTestServerInterface_->SpDaemonProcess(daemonCommand, extraInfo);
    }

    int32_t TestServerClient::CollectProcessMemory(int32_t &pid, ProcessMemoryInfo &processMemoryInfo)
    {
        HiLog::Info(LABEL, "%{public}s called.", __func__);
        if (iTestServerInterface_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
            return TEST_SERVER_GET_INTERFACE_FAILED;
        }
        return iTestServerInterface_->CollectProcessMemory(pid, processMemoryInfo);
    }

    int32_t TestServerClient::CollectProcessCpu(int32_t &pid, bool isNeedUpdate, ProcessCpuInfo &processCpuInfo)
    {
        HiLog::Info(LABEL, "%{public}s called.", __func__);
        if (iTestServerInterface_ == nullptr) {
            HiLog::Error(LABEL, "%{public}s. Get iTestServerInterface FAILED", __func__);
            return TEST_SERVER_GET_INTERFACE_FAILED;
        }
        return iTestServerInterface_->CollectProcessCpu(pid, isNeedUpdate, processCpuInfo);
    }
} // namespace OHOS::testserver

void FrequencyLockPlugin()
{
    OHOS::testserver::TestServerClient::GetInstance().FrequencyLock();
}