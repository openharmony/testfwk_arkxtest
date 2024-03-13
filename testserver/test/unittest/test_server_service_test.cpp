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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "system_ability_definition.h"
#include "system_ability.h"
#include "iservice_registry.h"
#include "test_server_service.h"
#include "mock_permission.h"
#include "start_test_server.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::testserver;

class TestServerServiceMock : public TestServerService {
public:
    TestServerServiceMock(int32_t saId, bool runOnCreate) : TestServerService(saId, runOnCreate) {}

    void OnStart()
    {
        TestServerService::OnStart();
    }

    // These test should convert function IsRootVersion(), IsDeveloperMode() to virtual
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
};

class ServiceTest : public testing::Test {
public:
    ~ServiceTest() override = default;

protected:
    const int32_t systemAbilityId = TEST_SERVER_SA_ID;
    sptr<ISystemAbilityManager> samgr_ = nullptr;
    unique_ptr<TestServerServiceMock> testServerServiceMock_ = nullptr;
    const int32_t UNLOAD_SYSTEMABILITY_WAITTIME = 2000;

    void SetUp() override
    {
        TestServerMockPermission::MockProcess("testserver");
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        testServerServiceMock_ = make_unique<TestServerServiceMock>(systemAbilityId, false);
    }

    void TearDown() override
    {
        samgr_->UnloadSystemAbility(systemAbilityId);
        samgr_->RemoveSystemAbility(systemAbilityId);
        this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
        EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
        testServerServiceMock_.reset();
        EXPECT_EQ(testServerServiceMock_, nullptr);
    }
};

TEST_F(ServiceTest, testOnStartWhenMock)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    EXPECT_CALL(*testServerServiceMock_, IsRootVersion()).WillOnce(testing::Return(true));
    testServerServiceMock_->OnStart();
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

TEST_F(ServiceTest, testOnStartWhenUserDeveloper)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    EXPECT_CALL(*testServerServiceMock_, IsRootVersion()).WillOnce(testing::Return(false));
    EXPECT_CALL(*testServerServiceMock_, IsDeveloperMode()).WillOnce(testing::Return(true));
    testServerServiceMock_->OnStart();
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

TEST_F(ServiceTest, testOnStartWhenUserNonDeveloper)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    EXPECT_CALL(*testServerServiceMock_, IsRootVersion()).WillOnce(testing::Return(false));
    EXPECT_CALL(*testServerServiceMock_, IsDeveloperMode()).WillOnce(testing::Return(false));
    testServerServiceMock_->OnStart();
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

TEST_F(ServiceTest, testRemoveTestServer)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    EXPECT_TRUE(testServerServiceMock_->RemoveTestServer());
    this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

TEST_F(ServiceTest, testDestorySessionWithCaller)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    testServerServiceMock_->CreateSessionMock();
    testServerServiceMock_->DestorySession();
    this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

TEST_F(ServiceTest, testDestorySessionWithoutCaller)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    testServerServiceMock_->DestorySession();
    this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

class CallerDetectTimerTest : public testing::Test {
public:
    ~CallerDetectTimerTest() override = default;

protected:
    const int32_t systemAbilityId = TEST_SERVER_SA_ID;
    sptr<ISystemAbilityManager> samgr_ = nullptr;
    const int32_t UNLOAD_SYSTEMABILITY_WAITTIME = 2000;
    const int32_t CALLER_DECTECT_TIMER_WAITTIME = 12000;

    void SetUp() override
    {
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    }

    void TearDown() override
    {
        samgr_->UnloadSystemAbility(systemAbilityId);
        this_thread::sleep_for(chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
    }
};

TEST_F(CallerDetectTimerTest, testCallerDetectTimerWithCaller)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    sptr<SessionToken> sessionToken = new (std::nothrow) SessionToken();
    EXPECT_EQ(iTestServerInterface->CreateSession(*sessionToken), ERR_OK);
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    this_thread::sleep_for(chrono::milliseconds(CALLER_DECTECT_TIMER_WAITTIME));
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}

TEST_F(CallerDetectTimerTest, testCallerDetectTimerWithoutCaller)
{
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    sptr<ITestServerInterface> iTestServerInterface = StartTestServer::GetInstance().LoadTestServer();
    EXPECT_NE(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
    this_thread::sleep_for(chrono::milliseconds(CALLER_DECTECT_TIMER_WAITTIME));
    EXPECT_EQ(samgr_->CheckSystemAbility(systemAbilityId), nullptr);
}