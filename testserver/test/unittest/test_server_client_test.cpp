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
#include <thread>
#include <gtest/hwext/gtest-multithread.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "system_ability_definition.h"
#include "system_ability.h"
#include "iservice_registry.h"
#include "test_server_client.h"
#include "mock_permission.h"

using namespace std;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS;
using namespace OHOS::testserver;

const int32_t SYSTEM_ABILITY_ID = TEST_SERVER_SA_ID;
const int32_t UNLOAD_SYSTEMABILITY_WAITTIME = 2000;

HWTEST(ClientTest, testLoadTestServer, TestSize.Level1)
{
    TestServerMockPermission::MockProcess("testserver");
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<ITestServerInterface> iTestServerInterface = TestServerClient::GetInstance().LoadTestServer();
    EXPECT_NE(iTestServerInterface, nullptr);
    EXPECT_NE(samgr->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    samgr->UnloadSystemAbility(SYSTEM_ABILITY_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
}

void load_test_server_test(sptr<ISystemAbilityManager> samgr)
{
    TestServerMockPermission::MockProcess("testserver");
    sptr<ITestServerInterface> iTestServerInterface = TestServerClient::GetInstance().LoadTestServer();
    EXPECT_NE(iTestServerInterface, nullptr);
    EXPECT_NE(samgr->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
}

HWMTEST(ClientTest, testMutilLoadTestServer, TestSize.Level1, 3)
{
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_EQ(samgr->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    load_test_server_test(samgr);
    EXPECT_NE(samgr->CheckSystemAbility(SYSTEM_ABILITY_ID), nullptr);
    samgr->UnloadSystemAbility(SYSTEM_ABILITY_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(UNLOAD_SYSTEMABILITY_WAITTIME));
}