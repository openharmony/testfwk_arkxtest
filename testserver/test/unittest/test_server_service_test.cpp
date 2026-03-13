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

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "system_ability_definition.h"
#include "system_ability.h"
#include "iservice_registry.h"
#include "test_server_service.h"
#include "mock_permission.h"
#include "start_test_server.h"
#include "test_server_error_code.h"
#ifdef ARKXTEST_PASTEBOARD_ENABLE
#include "pasteboard_client.h"
#endif
#include "common_event_manager.h"
#include "common_event_subscribe_info.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::testserver;

class TestServerServiceMock : public TestServerService {
public:
    TestServerServiceMock(int32_t saId, bool runOnCreate) : TestServerService(saId, runOnCreate) {}

    void OnStart()
    {
        TestServerService::OnStart();
    }

    void OnStop()
    {
        TestServerService::OnStop();
    }

    MOCK_METHOD0(IsRootVersion, bool());
    MOCK_METHOD0(IsDeveloperMode, bool());

    void DestorySession()
    {
        TestServerService::DestorySession();
    }

    bool RemoveTestServer()
    {
        return TestServerService::RemoveTestServer();
    }

    bool CreateSessionMock()
    {
        TestServerService::AddCaller();
        return true;
    }

    int GetCallerCount()
    {
        return TestServerService::GetCallerCount();
    }
};

class ServiceTest : public testing::Test {
public:
    ~ServiceTest() override = default;

protected:
    const int32_t SYSTEM_ABILITY_ID = TEST_SERVER_SA_ID;
    sptr<ISystemAbilityManager> samgr_ = nullptr;
    unique_ptr<TestServerServiceMock> testServerServiceMock_ = nullptr;
    const int32_t UNLOAD_SYSTEMABILITY_WAITTIME = 2000;

    void SetUp() override
    {
        TestServerMockPermission::MockProcess("testserver");
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        testServerServiceMock_ = make_unique<TestServerServiceMock>(SYSTEM_ABILITY_ID, false);
    }

    void TearDown() override
    {
        samgr_->UnloadSystemAbility(SYSTEM_ABILITY_ID);
        samgr_->RemoveSystemAbility(SYSTEM_ABILITY_ID);
        this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
        EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
        testServerServiceMock_.reset();
        EXPECT_EQ(testServerServiceMock_, nullptr);
    }
};

HWTEST_F(ServiceTest, testOnStartWhenRoot, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    EXPECT_CALL(*testServerServiceMock_, IsRootVersion()).WillOnce(testing::Return(true));
    testServerServiceMock_->OnStart();
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWTEST_F(ServiceTest, testOnStartWhenUserDeveloper, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    EXPECT_CALL(*testServerServiceMock_, IsRootVersion()).WillOnce(testing::Return(false));
    EXPECT_CALL(*testServerServiceMock_, IsDeveloperMode()).WillOnce(testing::Return(true));
    testServerServiceMock_->OnStart();
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWTEST_F(ServiceTest, testOnStartWhenUserNonDeveloper, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    EXPECT_CALL(*testServerServiceMock_, IsRootVersion()).WillOnce(testing::Return(false));
    EXPECT_CALL(*testServerServiceMock_, IsDeveloperMode()).WillOnce(testing::Return(false));
    testServerServiceMock_->OnStart();
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWTEST_F(ServiceTest, testRemoveTestServer, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    EXPECT_TRUE(testServerServiceMock_->RemoveTestServer());
    this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWTEST_F(ServiceTest, testDestorySessionWithCaller, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    testServerServiceMock_->CreateSessionMock();
    testServerServiceMock_->DestorySession();
    this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    EXPECT_EQ(testServerServiceMock_->GetCallerCount(), 1);
}

HWTEST_F(ServiceTest, testDestorySessionWithoutCaller, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    testServerServiceMock_->DestorySession();
    this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWTEST_F(ServiceTest, testSetPasteData, TestSize.Level1)
{
#ifdef ARKXTEST_PASTEBOARD_ENABLE
    auto pasteBoardMgr = MiscServices::PasteboardClient::GetInstance();
    pasteBoardMgr->Clear();
    EXPECT_FALSE(pasteBoardMgr->HasPasteData());
    string text = "中文文本";
    int32_t ret;
    int32_t resCode1 = testServerServiceMock_->SetPasteData(text, ret);
    EXPECT_EQ(resCode1, 0);
    EXPECT_TRUE(pasteBoardMgr->HasPasteData());
    OHOS::MiscServices::PasteData pasteData;
    int32_t resCode2 = pasteBoardMgr->GetPasteData(pasteData);
    uint32_t successErrCode = 27787264;
    EXPECT_EQ(resCode2, successErrCode);
    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == text);
#else
    int32_t resCode1 = testServerServiceMock_->SetPasteData("text");
    EXPECT_EQ(resCode1, 0);
#endif
}

HWTEST_F(ServiceTest, testPublishEvent, TestSize.Level1)
{
    OHOS::EventFwk::CommonEventData event;
    auto want = OHOS::AAFwk::Want();
    want.SetAction("uitest.broadcast.command");
    event.SetWant(want);
    bool re = false;
    int32_t resCode1 = testServerServiceMock_->PublishCommonEvent(event, re);
    EXPECT_EQ(resCode1, 0);
}

class CallerDetectTimerTest : public testing::Test {
public:
    ~CallerDetectTimerTest() override = default;

protected:
    const int32_t SYSTEM_ABILITY_ID = TEST_SERVER_SA_ID;
    sptr<ISystemAbilityManager> samgr_ = nullptr;
    const int32_t UNLOAD_SYSTEMABILITY_WAITTIME = 2000;
    const int32_t CALLER_DECTECT_TIMER_WAITTIME = 12000;

    void SetUp() override
    {
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    }

    void TearDown() override
    {
        samgr_->UnloadSystemAbility(SYSTEM_ABILITY_ID);
        this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    }
};

HWTEST_F(CallerDetectTimerTest, testCallerDetectTimerWithoutCaller, TestSize.Level1)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    this_thread::sleep_for(chrono::milliseconds(CALLER_DECTECT_TIMER_WAITTIME));
    EXPECT_EQ(samgr_->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWTEST_F(ServiceTest, testSetTime, TestSize.Level1)
{
    int64_t timeMs = 1739085000000;
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetTime(timeMs, ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(ServiceTest, testSetTimeInvalid, TestSize.Level1)
{
    int64_t timeMs = -1;
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetTime(timeMs, ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, OHOS::testserver::TEST_SERVER_TIME_SERVICE_FAILED);
}

HWTEST_F(ServiceTest, testSetTimezone, TestSize.Level1)
{
    string timezoneId = "Asia/Shanghai";
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetTimezone(timezoneId, ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(ServiceTest, testSetTimezoneInvalid, TestSize.Level1)
{
    string timezoneId = "Invalid/Timezone";
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetTimezone(timezoneId, ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, OHOS::testserver::TEST_SERVER_INVALID_TIMEZONE_ID);
}

HWTEST_F(ServiceTest, testGetPasteData, TestSize.Level1)
{
#ifdef ARKXTEST_PASTEBOARD_ENABLE
    string text = "Test Content";
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetPasteData(text, ret);
    EXPECT_EQ(resCode, 0);
    string pasteText;
    int32_t getResult;
    int32_t resCode2 = testServerServiceMock_->GetPasteData(pasteText, getResult);
    EXPECT_EQ(resCode2, 0);
    EXPECT_EQ(getResult, 0);
    EXPECT_EQ(pasteText, text);
#else
    string pasteText;
    int32_t getResult;
    int32_t resCode = testServerServiceMock_->GetPasteData(pasteText, getResult);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(getResult, OHOS::testserver::TEST_SERVER_NOT_SUPPORTED);
#endif
}

HWTEST_F(ServiceTest, testClearPasteData, TestSize.Level1)
{
#ifdef ARKXTEST_PASTEBOARD_ENABLE
    string text = "Test Content";
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetPasteData(text, ret);
    EXPECT_EQ(resCode, 0);
    auto pasteBoardMgr = MiscServices::PasteboardClient::GetInstance();
    EXPECT_TRUE(pasteBoardMgr->HasPasteData());
    int32_t clearRet;
    int32_t resCode2 = testServerServiceMock_->ClearPasteData(clearRet);
    EXPECT_EQ(resCode2, 0);
    EXPECT_EQ(clearRet, 0);
    EXPECT_FALSE(pasteBoardMgr->HasPasteData());
#else
    int32_t clearRet;
    int32_t resCode = testServerServiceMock_->ClearPasteData(clearRet);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(clearRet, OHOS::testserver::TEST_SERVER_NOT_SUPPORTED);
#endif
}

HWTEST_F(ServiceTest, testHideKeyboard, TestSize.Level1)
{
    int32_t ret;
    int32_t resCode = testServerServiceMock_->HideKeyboard(ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, OHOS::testserver::TEST_SERVER_NO_ACTIVE_IME);
}

HWTEST_F(ServiceTest, testSetTimezoneCommon, TestSize.Level1)
{
    vector<string> timezones = {
        "America/New_York",
        "Europe/London",
        "Asia/Tokyo",
        "Australia/Sydney"
    };
    for (const auto& tz : timezones) {
        int32_t ret;
        int32_t resCode = testServerServiceMock_->SetTimezone(tz, ret);
        EXPECT_EQ(resCode, 0) << "Failed for timezone: " << tz;
        EXPECT_EQ(ret, 0) << "Failed for timezone: " << tz;
    }
}

HWTEST_F(ServiceTest, testSetPasteDataEmpty, TestSize.Level1)
{
#ifdef ARKXTEST_PASTEBOARD_ENABLE
    string text = "";
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetPasteData(text, ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, 0);
#else
    string text = "";
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetPasteData(text, ret);
    EXPECT_EQ(resCode, 0);
#endif
}

HWTEST_F(ServiceTest, testSetTimeBoundary, TestSize.Level1)
{
    int64_t timeMs = 0;
    int32_t ret;
    int32_t resCode = testServerServiceMock_->SetTime(timeMs, ret);
    EXPECT_EQ(resCode, 0);
    EXPECT_EQ(ret, OHOS::testserver::TEST_SERVER_TIME_SERVICE_FAILED);
}