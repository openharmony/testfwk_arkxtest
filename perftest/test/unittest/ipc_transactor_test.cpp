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

#include <fcntl.h>
#include <sys/mman.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#define private public
#define protected public
#include "api_caller_client.h"
#include "api_caller_server.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::perftest;

class IpcTransactorTest : public testing::Test {
public:
    ~IpcTransactorTest() override = default;
protected:
    shared_ptr<ApiCallerClient> apiCallerClient_ = make_shared<ApiCallerClient>();
    shared_ptr<ApiCallerServer> apiCallerServer_ = make_shared<ApiCallerServer>();
    void TearDown() override
    {
        apiCallerClient_->connectState_ = UNINIT;
        apiCallerClient_->remoteCaller_ = nullptr;
        apiCallerClient_->processingApi_ = "";
        apiCallerServer_->connectState_ = UNINIT;
    }
};

HWTEST_F(IpcTransactorTest, testClientInitAndConnectPeer, TestSize.Level1)
{
    ASSERT_EQ(apiCallerClient_->GetConnectionStat(), UNINIT);
    bool result = apiCallerClient_->InitAndConnectPeer("TestToken", nullptr);
    ASSERT_EQ(result, false);
    ASSERT_EQ(apiCallerClient_->GetConnectionStat(), DISCONNECTED);
    apiCallerClient_->connectState_ = CONNECTED;
    ASSERT_EQ(apiCallerClient_->GetConnectionStat(), CONNECTED);
    result = apiCallerClient_->InitAndConnectPeer("TestToken", nullptr);
    ASSERT_EQ(result, true);
}

HWTEST_F(IpcTransactorTest, testClientTransact, TestSize.Level1)
{
    apiCallerClient_->connectState_ = DISCONNECTED;
    auto call = ApiCallInfo();
    auto reply = ApiReplyInfo();
    apiCallerClient_->Transact(call, reply);
    ASSERT_EQ(reply.exception_.code_, ERR_INTERNAL);
    apiCallerClient_->connectState_ = CONNECTED;
    apiCallerClient_->remoteCaller_ = new ApiCallerProxy(new ApiCallerStub());
    apiCallerClient_->processingApi_ = "TestID";
    apiCallerClient_->Transact(call, reply);
    ASSERT_EQ(reply.exception_.code_, ERR_API_USAGE);
}

HWTEST_F(IpcTransactorTest, testClientOnPeerDeath, TestSize.Level1)
{
    ASSERT_EQ(apiCallerClient_->GetConnectionStat(), UNINIT);
    apiCallerClient_->OnPeerDeath();
    ASSERT_EQ(apiCallerClient_->GetConnectionStat(), DISCONNECTED);
}

HWTEST_F(IpcTransactorTest, testServerInitAndConnectPeer, TestSize.Level1)
{
    ASSERT_EQ(apiCallerServer_->GetConnectionStat(), UNINIT);
    bool result = apiCallerServer_->InitAndConnectPeer("TestToken", nullptr);
    ASSERT_EQ(result, false);
}

HWTEST_F(IpcTransactorTest, testServerTransact, TestSize.Level1)
{
    apiCallerServer_->connectState_ = DISCONNECTED;
    auto call = ApiCallInfo();
    auto reply = ApiReplyInfo();
    apiCallerServer_->Transact(call, reply);
    ASSERT_EQ(reply.exception_.code_, ERR_INTERNAL);
}

HWTEST_F(IpcTransactorTest, testServerOnPeerDeath, TestSize.Level1)
{
    ASSERT_EQ(apiCallerServer_->GetConnectionStat(), UNINIT);
    apiCallerServer_->OnPeerDeath();
    ASSERT_EQ(apiCallerServer_->GetConnectionStat(), DISCONNECTED);
}