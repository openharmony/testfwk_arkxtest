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

#include "test_server_service.h"

#include "iremote_object.h"
#include "system_ability_definition.h"
#include "hilog/log.h"
#include "parameters.h"
#include "iservice_registry.h"
#include "test_server_error_code.h"

namespace OHOS::testserver {
    // TEST_SERVER_SA_ID
    REGISTER_SYSTEM_ABILITY_BY_ID(TestServerService, TEST_SERVER_SA_ID, false); // SA run on demand

    using namespace std;
    using namespace OHOS::HiviewDFX;
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL_SERVICE = {LOG_CORE, 0xD003110, "TestServerService"};
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL_TIMER = {LOG_CORE, 0xD003110, "CallerDetectTimer"};
    static const int CALLER_DETECT_DURING = 10000;

    TestServerService::TestServerService(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called. saId=%{public}d, runOnCreate=%{public}d",
            __func__, saId, runOnCreate);
        StartCallerDetectTimer();
    }

    TestServerService::~TestServerService()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        callerDetectTimer_->Cancel();
    }

    void TestServerService::OnStart()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (!IsRootVersion() && !IsDeveloperMode()) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. System mode is unsatisfied.", __func__);
            return;
        }
        bool res = Publish(this);
        if (!res) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. Publish failed", __func__);
        }
    }

    void TestServerService::OnStop()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        IsDeveloperMode();
    }

    bool TestServerService::IsRootVersion()
    {
        bool debugmode = OHOS::system::GetBoolParameter("const.debuggable", false);
        HiLog::Info(LABEL_SERVICE, "%{public}s. debugmode=%{public}d", __func__, debugmode);
        return debugmode;
    }

    bool TestServerService::IsDeveloperMode()
    {
        bool developerMode = OHOS::system::GetBoolParameter("const.security.developermode.state", false);
        HiLog::Info(LABEL_SERVICE, "%{public}s. developerMode=%{public}d", __func__, developerMode);
        return developerMode;
    }

    void TestServerService::StartCallerDetectTimer()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        callerDetectTimer_ = new CallerDetectTimer(this);
        callerDetectTimer_->Start();
    }

    ErrCode TestServerService::CreateSession(const SessionToken &sessionToken)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        bool result = true;
        try {
            result = sessionToken.AddDeathRecipient(new TestServerProxyDeathRecipient(this));
        } catch(...) {
            result = false;
        }
        if (!result) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. AddDeathRecipient FAILD.", __func__);
            DestorySession();
            return TEST_SERVER_ADD_DEATH_RECIPIENT_FAILED;
        }
        AddCaller();
        HiLog::Info(LABEL_SERVICE, "%{public}s. Create session SUCCESS. callerCount=%{public}d",
            __func__, GetCallerCount());
        return TEST_SERVER_OK;
    }

    void TestServerService::AddCaller()
    {
        callerCount_++;
    }

    void TestServerService::RemoveCaller()
    {
        callerCount_--;
    }

    int TestServerService::GetCallerCount()
    {
        return callerCount_.load();
    }

    void TestServerService::DestorySession()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (callerCount_ == 0) {
            HiLog::Info(LABEL_SERVICE, "%{public}s. No proxy exists. Remove the TestServer", __func__);
            RemoveTestServer();
        } else {
            HiLog::Info(LABEL_SERVICE, "%{public}s. Other proxys exist. Can not remove the TestServer", __func__);
        }
    }

    bool TestServerService::RemoveTestServer()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called. ", __func__);
        sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. Get SystemAbility Manager failed!", __func__);
            return false;
        }
        auto res = samgr->UnloadSystemAbility(TEST_SERVER_SA_ID);
        return res == ERR_OK;
    }

    void TestServerService::TestServerProxyDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (object == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. IRemoteObject is NULL.", __func__);
            return;
        }
        testServerService_->RemoveCaller();
        testServerService_->DestorySession();
    }

    void TestServerService::CallerDetectTimer::Start()
    {
        HiLog::Info(LABEL_TIMER, "%{public}s called.", __func__);
        thread_ = thread([this] {
            this_thread::sleep_for(chrono::milliseconds(CALLER_DETECT_DURING));
            HiLog::Info(LABEL_TIMER, "%{public}s. Timer is done.", __func__);
            if (!testServerExit_) {
                testServerService_->DestorySession();
            }
        });
        thread_.detach();
    }

    void TestServerService::CallerDetectTimer::Cancel()
    {
        HiLog::Info(LABEL_TIMER, "%{public}s called.", __func__);
        testServerExit_ = true;
    }

} // namespace OHOS::testserver