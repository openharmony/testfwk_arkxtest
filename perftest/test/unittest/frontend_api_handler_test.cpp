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

#include "gtest/gtest.h"
#define private public
#define protected public
#include "frontend_api_handler.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::perftest;

class FrontedApiHandlerTest : public testing::Test {
public:
    ~FrontedApiHandlerTest() override = default;
protected:
    FrontendApiServer &server = FrontendApiServer::Get();
    string apiId = "testApi";
};

HWTEST_F(FrontedApiHandlerTest, testAddHandler, TestSize.Level1)
{
    ASSERT_FALSE(server.HasHandlerFor(apiId));
    auto handler = [](const ApiCallInfo &in, ApiReplyInfo &out) {};
    server.AddHandler(apiId, handler);
    ASSERT_TRUE(server.HasHandlerFor(apiId));
    server.RemoveHandler(apiId);
}

HWTEST_F(FrontedApiHandlerTest, testRemoveHandler, TestSize.Level1)
{
    ASSERT_FALSE(server.HasHandlerFor(apiId));
    auto handler = [](const ApiCallInfo &in, ApiReplyInfo &out) {};
    server.AddHandler(apiId, handler);
    ASSERT_TRUE(server.HasHandlerFor(apiId));
    server.RemoveHandler(apiId);
    ASSERT_FALSE(server.HasHandlerFor(apiId));
}

HWTEST_F(FrontedApiHandlerTest, testCall, TestSize.Level1)
{
    auto call1 = ApiCallInfo {.apiId_ = apiId};
    auto reply1 = ApiReplyInfo();
    server.Call(call1, reply1);
    ASSERT_EQ(reply1.exception_.code_, ERR_INTERNAL);
    ASSERT_TRUE(reply1.exception_.message_.find("No handler found") != string::npos);
    auto handler = [](const ApiCallInfo &in, ApiReplyInfo &out) {};
    server.AddHandler(apiId, handler);
    auto call2 = ApiCallInfo {.apiId_ = apiId};
    auto reply2 = ApiReplyInfo();
    server.Call(call2, reply2);
    ASSERT_EQ(reply2.exception_.code_, NO_ERROR);
    server.RemoveHandler(apiId);
}

HWTEST_F(FrontedApiHandlerTest, testPerfTestCreate, TestSize.Level1)
{
    auto call1 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    auto reply1 = ApiReplyInfo();
    server.Call(call1, reply1);
    ASSERT_EQ(reply1.exception_.code_, ERR_INVALID_INPUT);
    auto call2 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    nlohmann::json perfTestStrategyJson;
    call2.paramList_.emplace_back(perfTestStrategyJson);
    auto reply2 = ApiReplyInfo();
    server.Call(call2, reply2);
    ASSERT_EQ(reply2.exception_.code_, ERR_INVALID_INPUT);
    auto call3 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    perfTestStrategyJson["metrics"] = {0};
    call3.paramList_.emplace_back(perfTestStrategyJson);
    auto reply3 = ApiReplyInfo();
    server.Call(call3, reply3);
    ASSERT_EQ(reply3.exception_.code_, ERR_INVALID_INPUT);
    auto call4 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    perfTestStrategyJson["actionCode"] = "callback#1";
    call4.paramList_.emplace_back(perfTestStrategyJson);
    auto reply4 = ApiReplyInfo();
    server.Call(call4, reply4);
    ASSERT_EQ(reply4.exception_.code_, NO_ERROR);
}

HWTEST_F(FrontedApiHandlerTest, testPerfTestRun, TestSize.Level1)
{
    auto call0 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    nlohmann::json perfTestStrategyJson;
    perfTestStrategyJson["metrics"] = {0};
    perfTestStrategyJson["actionCode"] = "callback#1";
    call0.paramList_.emplace_back(perfTestStrategyJson);
    auto reply0 = ApiReplyInfo();
    server.Call(call0, reply0);
    ASSERT_EQ(reply0.exception_.code_, NO_ERROR);
    auto call1 = ApiCallInfo {.apiId_ = "PerfTest.run", .callerObjRef_ = reply0.resultValue_.get<string>()};
    auto reply1 = ApiReplyInfo();
    server.Call(call1, reply1);
    ASSERT_EQ(reply1.exception_.code_, ERR_INTERNAL);
}

HWTEST_F(FrontedApiHandlerTest, testGetMeasureResult, TestSize.Level1)
{
    auto call0 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    nlohmann::json perfTestStrategyJson;
    perfTestStrategyJson["metrics"] = {0};
    perfTestStrategyJson["actionCode"] = "callback#1";
    call0.paramList_.emplace_back(perfTestStrategyJson);
    auto reply0 = ApiReplyInfo();
    server.Call(call0, reply0);
    ASSERT_EQ(reply0.exception_.code_, NO_ERROR);
    auto call1 = ApiCallInfo {.apiId_ = "PerfTest.getMeasureResult",
                              .callerObjRef_ = reply0.resultValue_.get<string>()};
    auto reply1 = ApiReplyInfo();
    server.Call(call1, reply1);
    ASSERT_EQ(reply1.exception_.code_, ERR_INVALID_INPUT);
    auto call2 = ApiCallInfo {.apiId_ = "PerfTest.getMeasureResult",
                              .callerObjRef_ = reply0.resultValue_.get<string>()};
    call2.paramList_.emplace_back(0);
    auto reply2 = ApiReplyInfo();
    server.Call(call2, reply2);
    ASSERT_EQ(reply2.exception_.code_, ERR_GET_RESULT_FAILED);
    auto call3 = ApiCallInfo {.apiId_ = "PerfTest.getMeasureResult",
                              .callerObjRef_ = reply0.resultValue_.get<string>()};
    call3.paramList_.emplace_back(1000);
    auto reply3 = ApiReplyInfo();
    server.Call(call3, reply3);
    ASSERT_EQ(reply3.exception_.code_, ERR_INVALID_INPUT);
}

HWTEST_F(FrontedApiHandlerTest, testGetBundleNameByPid, TestSize.Level1)
{
    auto call1 = ApiCallInfo {.apiId_ = "PerfTest.destroy"};
    auto reply1 = ApiReplyInfo();
    server.Call(call1, reply1);
    ASSERT_EQ(reply1.exception_.code_, ERR_INTERNAL);
    auto call0 = ApiCallInfo {.apiId_ = "PerfTest.create"};
    nlohmann::json perfTestStrategyJson;
    perfTestStrategyJson["metrics"] = {0};
    perfTestStrategyJson["actionCode"] = "callback#1";
    call0.paramList_.emplace_back(perfTestStrategyJson);
    auto reply0 = ApiReplyInfo();
    server.Call(call0, reply0);
    ASSERT_EQ(reply0.exception_.code_, NO_ERROR);
    auto call2 = ApiCallInfo {.apiId_ = "PerfTest.destroy", .callerObjRef_ = reply0.resultValue_.get<string>()};
    auto reply2 = ApiReplyInfo();
    server.Call(call2, reply2);
    ASSERT_EQ(reply2.exception_.code_, NO_ERROR);
}