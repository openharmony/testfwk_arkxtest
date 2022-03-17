/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#include <utility>

#include "extern_api.h"
#include "widget_selector.h"
#include "gtest/gtest.h"

using namespace OHOS::uitest;
using namespace std;
using namespace nlohmann;

// test fixture
class ExternApiTest : public testing::Test {
public:
    ~ExternApiTest() override = default;
};

class DummyApi : public ExternApi<TypeId::BY> {
public:
    explicit DummyApi(string_view val) : value_(val) {};

    ~ DummyApi() = default;

    void WriteIntoParcel(json &data) const override
    {
        data["value"] = value_;
    }

    void ReadFromParcel(const json &data) override
    {
        value_ = data["value"];
    }

    string value_;
};

// generate a global-unique identifier
#define GENERATE_UNIQUE_ID(x) string(__PRETTY_FUNCTION__)+string("_")+to_string(__LINE__)

TEST_F(ExternApiTest, readWriteParcelable)
{
    json container;
    // write
    PushBackValueItemIntoJson<string>("wyz", container);
    PushBackValueItemIntoJson<int32_t>(666, container);
    PushBackValueItemIntoJson<float>(0.2f, container);
    PushBackValueItemIntoJson<bool>(true, container);
    auto obj = DummyApi("zl");
    PushBackValueItemIntoJson<DummyApi>(obj, container);
    // read
    auto val0 = GetItemValueFromJson<string>(container, 0);
    auto val1 = GetItemValueFromJson<int32_t>(container, 1);
    auto val2 = GetItemValueFromJson<float>(container, 2);
    auto val3 = GetItemValueFromJson<bool>(container, 3);
    auto val4 = GetItemValueFromJson<json>(container, 4);
    // verify
    ASSERT_EQ("wyz", val0);
    ASSERT_EQ(666, val1);
    ASSERT_EQ(0.2f, val2);
    ASSERT_EQ(true, val3);
    ASSERT_EQ("zl", val4["value"]);
}

TEST_F(ExternApiTest, noInvocationHandler)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto& server = ExternApiServer::Get();
    json caller;
    auto in = json::array();
    auto out = json::array();
    auto err = ApiCallErr(NO_ERROR);
    server.Call(apiId, caller, in, out, err);
    ASSERT_EQ(INTERNAL_ERROR, err.code_);
    ASSERT_TRUE(err.message_.find("No handler found") != string::npos);
}

TEST_F(ExternApiTest, addRemoveHandler)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto &server = ExternApiServer::Get();
    auto handler = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        if (fun == apiId) {
            return true;
        }
        return false;
    };
    server.AddHandler(handler);

    json caller;
    auto in = json::array();
    auto out = json::array();
    auto err = ApiCallErr(NO_ERROR);
    server.Call(apiId, caller, in, out, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    server.RemoveHandler(handler);
    server.Call(apiId, caller, in, out, err);
    ASSERT_EQ(INTERNAL_ERROR, err.code_) << "The handler should be unavailable after been removed";
}

TEST_F(ExternApiTest, inOutObjectsTransfer)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto& server = ExternApiServer::Get();
    auto handler = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        if (fun == apiId) {
            // write back the parameter value
            PushBackValueItemIntoJson<int32_t>(GetItemValueFromJson<int32_t>(in, 0), out);
            return true;
        }
        return false;
    };
    server.AddHandler(handler);

    json caller;
    auto in0 = json::array();
    auto in1 = json::array();
    auto out0 = json::array();
    auto out1 = json::array();
    auto err = ApiCallErr(NO_ERROR);
    PushBackValueItemIntoJson<int32_t>(0, in0);
    PushBackValueItemIntoJson<int32_t>(1, in1);
    server.Call(apiId, caller, in0, out0, err);
    // api request should be handled and get correct return value
    ASSERT_EQ(NO_ERROR, err.code_);std::cout<<out0.dump()<<std::endl;
    ASSERT_EQ(0, GetItemValueFromJson<int32_t>(out0, 0));
    server.Call(apiId, caller, in1, out1, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    ASSERT_EQ(1, GetItemValueFromJson<int32_t>(out1, 0));
}

TEST_F(ExternApiTest, jsonExceptionDefance)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto& server = ExternApiServer::Get();
    auto handler = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        if (fun == apiId) {
            // this statement will cause out-of-index exception
            int32_t val0 = in.at(100);
            return val0 > 0;
        }
        return false;
    };
    server.AddHandler(handler);

    json caller;
    auto in = json::array();
    auto out = json::array();
    auto err = ApiCallErr(NO_ERROR);
    server.Call(apiId, caller, in, out, err);
    // json exception should be caught and reported properly
    ASSERT_EQ(INTERNAL_ERROR, err.code_);
    ASSERT_TRUE(err.message_.find("Exception raised") != string::npos);
    ASSERT_TRUE(err.message_.find("json.exception.out_of_range") != string::npos);
}

TEST_F(ExternApiTest, apiErrorDeliver)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto& server = ExternApiServer::Get();
    auto handler = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        if (fun == apiId) {
            // this statement will cause out-of-index exception
            int32_t val0 = GetItemValueFromJson<int32_t>(in, 0);
            if (val0 == 0) {
                err = ApiCallErr(NO_ERROR);
            } else {
                err = ApiCallErr(INTERNAL_ERROR, "dummy_error");
            }
            return true;
        }
        return false;
    };
    server.AddHandler(handler);

    json caller;
    auto in0 = json::array();
    PushBackValueItemIntoJson<int32_t>(0, in0);
    auto in1 = json::array();
    PushBackValueItemIntoJson<int32_t>(1, in1);
    auto out = json::array();
    auto err = ApiCallErr(NO_ERROR);

    server.Call(apiId, caller, in0, out, err);
    ASSERT_EQ(NO_ERROR, err.code_) << err.message_;
    server.Call(apiId, caller, in1, out, err);
    // api error should be delivered out to caller
    ASSERT_EQ(INTERNAL_ERROR, err.code_);
    ASSERT_TRUE(err.message_.find("dummy_error") != string::npos);
}

TEST_F(ExternApiTest, stopDispatchingToRestHandlersAfterHandled)
{
    auto& server = ExternApiServer::Get();
    auto handler0 = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        PushBackValueItemIntoJson<int32_t>(0, out);
        if (fun != "api0") {
            return false;
        }
        return true;
    };
    auto handler1 = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        PushBackValueItemIntoJson<int32_t>(1, out);
        if (fun != "api1") {
            return false;
        }
        auto param0 = GetItemValueFromJson<string>(in, 0);
        if (param0 == "good") {
            err = ApiCallErr(NO_ERROR);
        } else {
            err = ApiCallErr(INTERNAL_ERROR, "not good");
        }
        return true;
    };
    auto handler2 = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        PushBackValueItemIntoJson<int32_t>(2, out);
        return fun == "api2";
    };
    server.AddHandler(handler0);
    server.AddHandler(handler1);
    server.AddHandler(handler2);

    json caller;
    auto in = json::array();
    auto out = json::array();
    auto err = ApiCallErr(NO_ERROR);
    PushBackValueItemIntoJson<string>("good", in);
    server.Call("api1", caller, in, out, err);
    // Request should not be dispatched to handler2, since its served with by handler1
    ASSERT_EQ(2, out.size());
    ASSERT_EQ(NO_ERROR, err.code_);
    ASSERT_EQ(0, GetItemValueFromJson<int32_t>(out, 0));
    ASSERT_EQ(1, GetItemValueFromJson<int32_t>(out, 1));
    in.clear();
    out.clear();
    PushBackValueItemIntoJson<string>("bad", in);
    server.Call("api1", caller, in, out, err);
    // Request should not be dispatched to handler2, since its served with by handler1, even though error happened
    ASSERT_EQ(2, out.size());
    ASSERT_EQ(INTERNAL_ERROR, err.code_);
    ASSERT_EQ(0, GetItemValueFromJson<int32_t>(out, 0));
    ASSERT_EQ(1, GetItemValueFromJson<int32_t>(out, 1));
}

TEST_F(ExternApiTest, apiTransactE2E)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto& server = ExternApiServer::Get();
    auto handler = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        if (fun != apiId) {
            return false;
        }
        PushBackValueItemIntoJson<int32_t>(0, out);
        return true;
    };
    server.AddHandler(handler);

    string resultStr = ApiTransact(apiId, "{}", "[]");
    auto result = json::parse(resultStr);
    ASSERT_FALSE(result.contains("exception"));
    ASSERT_TRUE(result.contains("result_values"));
    ASSERT_NE(0, result["result_values"].size()) << resultStr; // should have result values
}

TEST_F(ExternApiTest, apiTransactE2EFailure)
{
    static auto apiId = GENERATE_UNIQUE_ID();
    auto &server = ExternApiServer::Get();
    auto handler = [](string_view fun, json &caller, const json &in, json &out, ApiCallErr &err) {
        if (fun != apiId) {
            return false;
        }
        PushBackValueItemIntoJson<int32_t>(0, out);
        return true;
    };
    server.AddHandler(handler);

    string resultStr = ApiTransact("foo", "{}", "[]"); // api not-found
    auto result = json::parse(resultStr);
    ASSERT_TRUE(result.contains("exception")) << "Transact failure should be delivered in the results";
}