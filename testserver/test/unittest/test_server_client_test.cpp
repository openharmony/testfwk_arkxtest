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
#include "gtest/gtest.h"
#include "system_ability_definition.h"
#include "system_ability.h"
#include "iservice_registry.h"
#include "test_server_client.h"
#include "mock_permission.h"

using namespace OHOS;
using namespace OHOS::testserver;

const int32_t systemAbilityId = TEST_SERVER_SA_ID;

TEST(ClientTest, testLoadTestServer)
{
    TestServerMockPermission::MockProcess("testserver");
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<ITestServerInterface> iTestServerInterface = TestServerClient::GetInstance().LoadTestServer();
    EXPECT_NE(iTestServerInterface, nullptr);
    EXPECT_NE(samgr->CheckSystemAbility(systemAbilityId), nullptr);
    samgr->UnloadSystemAbility(systemAbilityId);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}