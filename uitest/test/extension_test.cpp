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

#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <future>
#include <securec.h>
#include <sys/mman.h>
#include "gtest/gtest.h"
#include "common_utilities_hpp.h"
#include "extension_c_api.h"
#include "mock_extension_executor.h"
#include "screen_copy.h"
#include "nlohmann/json.hpp"

using namespace OHOS::uitest;
using namespace std;

namespace OHOS::uitest {
class ExtensionTest : public testing::Test {
public:
    ~ExtensionTest() override = default;
};

TEST_F(ExtensionTest, testAtomicMouseAction)
{
    int32_t stage1 = 1;
    int32_t stage2 = -1;
    int32_t px = 100;
    int32_t py = 100;
    int32_t btn1 = 0;
    int32_t btn2 = 5;
    RetCode ret1 = AtomicMouseAction(stage1, px, py, btn1);
    ASSERT_EQ(RETCODE_SUCCESS, ret1);
    RetCode ret2 = AtomicMouseAction(stage2, px, py, btn1);
    ASSERT_EQ(RETCODE_FAIL, ret2);
    RetCode ret3 = AtomicMouseAction(stage1, px, py, btn2);
    ASSERT_EQ(RETCODE_FAIL, ret3);
}

TEST_F(ExtensionTest, testAtomicTouch)
{
    int32_t stage1 = 1;
    int32_t stage2 = -1;
    int32_t px = 100;
    int32_t py = 100;
    RetCode ret1 = AtomicTouch(stage1, px, py);
    ASSERT_EQ(RETCODE_SUCCESS, ret1);
    RetCode ret2 = AtomicTouch(stage2, px, py);
    ASSERT_EQ(RETCODE_FAIL, ret2);
}

TEST_F(ExtensionTest, testGetDumpInfo)
{
    nlohmann::json options;
    options["bundleName"] = "bundleName";
    options["windowId"] = "1";
    options["mergeWindow"] = "true";
    options["displayId"] = 0;
    nlohmann::json tree;
    ApiCallErr err(NO_ERROR);
    RetCode ret1 = GetDumpInfo(options, tree, err);
    ASSERT_EQ(RETCODE_SUCCESS, ret1);
    options["bundleName"] = 0;
    RetCode ret2 = GetDumpInfo(options, tree, err);
    ASSERT_EQ(RETCODE_FAIL, ret2);
    options["windowId"] = 1;
    RetCode ret3 = GetDumpInfo(options, tree, err);
    ASSERT_EQ(RETCODE_FAIL, ret3);
    options["windowId"] = "windowId";
    RetCode ret4 = GetDumpInfo(options, tree, err);
    ASSERT_EQ(RETCODE_FAIL, ret4);
    options["mergeWindow"] = true;
    RetCode ret5 = GetDumpInfo(options, tree, err);
    ASSERT_EQ(RETCODE_FAIL, ret5);
    options["displayId"] = "0";
    RetCode ret6 = GetDumpInfo(options, tree, err);
    ASSERT_EQ(RETCODE_FAIL, ret6);
}

static string g_recordCapture;
static void Callback(Text bytes)
{
    g_recordCapture = string(reinterpret_cast<const char *>(bytes.data), bytes.size);
}

TEST_F(ExtensionTest, testStartCapture)
{
    auto name1 = Text{ .data = "recordUiAction", .size = strlen("recordUiAction") };
    auto opt1 = Text{ .data = "{}", .size = strlen("{}") };
    RetCode ret1 = StartCapture(name1, Callback, opt1);
    ASSERT_EQ(RETCODE_SUCCESS, ret1);
    auto name2 = Text{ .data = "haha", .size = strlen("haha") };
    RetCode ret2 = StartCapture(name2, Callback, opt1);
    ASSERT_EQ(RETCODE_FAIL, ret2);
    auto opt2 = Text{ .data = "{xxxx}", .size = strlen("{xxxx}") };
    RetCode ret3 = StartCapture(name1, Callback, opt2);
    ASSERT_EQ(RETCODE_FAIL, ret3);
}

TEST_F(ExtensionTest, testStopCapture)
{
    auto name1 = Text{ .data = "copyScreen", .size = strlen("copyScreen") };
    RetCode ret1 = StopCapture(name1);
    ASSERT_EQ(RETCODE_SUCCESS, ret1);
    auto name2 = Text{ .data = "haha", .size = strlen("haha") };
    RetCode ret2 = StopCapture(name2);
    ASSERT_EQ(RETCODE_FAIL, ret2);
}
}