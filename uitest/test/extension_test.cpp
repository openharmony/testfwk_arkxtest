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
#include "extension_executor.h"
#include "json.hpp"

using namespace OHOS::uitest;
using namespace std;

// dummy uirecord implmentations, to get rid of depending on 'uitest_uirecord'
namespace OHOS::uitest {
static auto g_dummyRecordEvent = R"({})";
int32_t UiDriverRecordStart(function<void(nlohmann::json)> handler,  string modeOpt)
{
    if (handler != nullptr) {
        handler(nlohmann::json::parse(g_dummyRecordEvent));
    }
    return 0;
}
void UiDriverRecordStop() {}

// test fixture
class ExtensionTest : public testing::Test {
public:
    ~ExtensionTest() override = default;
protected:
    static void SetUpTestSuite()
    {
        // redirect hook functions to my implmentations
        auto addr0 = reinterpret_cast<uintptr_t>(ExtensionTest::ExtensionOnInitImpl);
        auto addr1 = reinterpret_cast<uintptr_t>(ExtensionTest::ExtensionOnRunImpl);
        setenv("OnInitImplAddr", to_string(addr0).c_str(), 1);
        setenv("OnRunImplAddr", to_string(addr1).c_str(), 1);
    }

    void SetUp() override
    {
        portCapture.getAndClearLastError = nullptr;
        portCapture.printLog = nullptr;
        portCapture.getUiTestVersion = nullptr;
        portCapture.initLowLevelFunctions = nullptr;
        argcCapture = 0;
        argvCapture = nullptr;
        onInitRet = RETCODE_SUCCESS;
        onRunRet = RETCODE_SUCCESS;
    }
    
    // overwrite these two values to set the return value of extension hook-functions
    static RetCode onInitRet;
    static RetCode onRunRet;
    static UiTestPort portCapture;
    static size_t argcCapture;
    static const char **argvCapture;
private:
    static RetCode ExtensionOnInitImpl(UiTestPort port, size_t argc, const char **argv)
    {
        portCapture = port;
        argcCapture = argc;
        argvCapture = argv;
        return onInitRet;
    }

    static RetCode ExtensionOnRunImpl()
    {
        return onRunRet;
    }
};

RetCode ExtensionTest::onInitRet = RETCODE_SUCCESS;
RetCode ExtensionTest::onRunRet = RETCODE_SUCCESS;
UiTestPort ExtensionTest::portCapture;
size_t ExtensionTest::argcCapture;
const char **ExtensionTest::argvCapture;

static bool CheckExtensionLibExist()
{
    if (!filesystem::exists("/data/local/tmp/agent.so")) {
        fprintf(stdout, "EXTENSION_LIBRARY_NOT_PRESENT!\n");
        fprintf(stderr, "EXTENSION_LIBRARY_NOT_PRESENT!\n");
        return false;
    }
    return true;
}

TEST_F(ExtensionTest, testHookFuncs)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    onInitRet = RETCODE_SUCCESS;
    onRunRet = RETCODE_SUCCESS;
    // should run both hooks and return true
    ASSERT_TRUE(ExecuteExtension("0", 0, nullptr));
    onInitRet = RETCODE_SUCCESS;
    onRunRet = RETCODE_FAIL;
    // should run both hooks and return false since onRunRet=FAIL
    ASSERT_FALSE(ExecuteExtension("0", 0, nullptr));
    onInitRet = RETCODE_FAIL;
    onRunRet = RETCODE_SUCCESS;
    // should run both hooks and return false since onInitRet=FAIL
    ASSERT_FALSE(ExecuteExtension("0", 0, nullptr));
}

TEST_F(ExtensionTest, testPassArgs)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    char arg0[1];
    char arg1[1];
    char *argv[] = {arg0, arg1};
    const int32_t argc = sizeof(argv) / sizeof(argv[0]);
    ExecuteExtension("0", argc, argv);
    ASSERT_EQ(argc, argcCapture);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(argv), reinterpret_cast<uintptr_t>(argvCapture));
}

TEST_F(ExtensionTest, testPassArgsExtName)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    static constexpr uint32_t ARG_BUF_SIZE = 32;
    char arg0[ARG_BUF_SIZE];
    char arg1[ARG_BUF_SIZE];
    char arg2[ARG_BUF_SIZE];
    char arg3[ARG_BUF_SIZE];
    memset_s(arg0, ARG_BUF_SIZE, 0, ARG_BUF_SIZE);
    memset_s(arg1, ARG_BUF_SIZE, 0, ARG_BUF_SIZE);
    memset_s(arg2, ARG_BUF_SIZE, 0, ARG_BUF_SIZE);
    memset_s(arg3, ARG_BUF_SIZE, 0, ARG_BUF_SIZE);
    memcpy_s(arg0, ARG_BUF_SIZE - 1, "--extension-name", strlen("--extension-name"));
    memcpy_s(arg1, ARG_BUF_SIZE - 1, "agent.so", strlen("agent.so"));
    memcpy_s(arg2, ARG_BUF_SIZE - 1, "--arg1", strlen("--arg1"));
    memcpy_s(arg3, ARG_BUF_SIZE - 1, "--val1", strlen("--val1"));
    char *argv[] = {arg0, arg1, arg2, arg3};
    const int32_t argc = sizeof(argv) / sizeof(argv[0]);
    ExecuteExtension("0", argc, argv);
    // argv0,1 gives the extension name, should not be pass down
    ASSERT_EQ(argc - TWO, argcCapture);
    ASSERT_STREQ(argv[TWO], argvCapture[0]);
    ASSERT_STREQ(argv[THREE], argvCapture[1]);
    // illegal extension-name, should cause run failure
    memcpy_s(arg1, ARG_BUF_SIZE - 1, "notexist.so", strlen("notexist.so"));
    ASSERT_FALSE(ExecuteExtension("0", argc, argv));
}

TEST_F(ExtensionTest, testExtensionPorts)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    ExecuteExtension("0", 0, nullptr);
    ASSERT_NE(nullptr, portCapture.getUiTestVersion);
    ASSERT_NE(nullptr, portCapture.printLog);
    ASSERT_NE(nullptr, portCapture.getAndClearLastError);
    ASSERT_NE(nullptr, portCapture.initLowLevelFunctions);
    // load and check low-level-functions
    // if recv arg is null, should fail
    ASSERT_EQ(RETCODE_FAIL, portCapture.initLowLevelFunctions(nullptr));
    LowLevelFunctions llfs;
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.initLowLevelFunctions(&llfs));
    ASSERT_NE(nullptr, llfs.callThroughMessage);
    ASSERT_NE(nullptr, llfs.setCallbackMessageHandler);
    ASSERT_NE(nullptr, llfs.startCapture);
    ASSERT_NE(nullptr, llfs.stopCapture);
    ASSERT_NE(nullptr, llfs.atomicTouch);
}

TEST_F(ExtensionTest, testGetUiTestVersion)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    static constexpr string_view VERSION = "1.2.3.4";
    ASSERT_TRUE(ExecuteExtension(VERSION, 0, nullptr));
    static constexpr size_t bufSize = 16;
    uint8_t buf[bufSize] = { 0 };
    size_t outSize = 0;
    auto recv = ReceiveBuffer {.data = buf, .capacity = bufSize, .size = &outSize};
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.getUiTestVersion(recv));
    ASSERT_EQ(VERSION.length(), outSize);
    ASSERT_EQ(0, memcmp(VERSION.data(), buf, VERSION.length()));
    // receive with not-enough buf, should cause failure
    auto recvShort = ReceiveBuffer {.data = buf, .capacity = VERSION.length() - 1, .size = &outSize};
    ASSERT_EQ(RETCODE_FAIL, portCapture.getUiTestVersion(recvShort));
}

TEST_F(ExtensionTest, testGetAndClearLastError)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    ASSERT_TRUE(ExecuteExtension("0", 0, nullptr));
    static constexpr size_t bufSize = 64;
    uint8_t buf[bufSize] = { 0 };
    size_t outSize = 0;
    auto recv = ReceiveBuffer {.data = buf, .capacity = bufSize, .size = &outSize};
    int32_t errCode = 0;
    // code receiver is nullptr, cannot get
    ASSERT_EQ(RETCODE_FAIL, portCapture.getAndClearLastError(nullptr, recv));
    // trigger an error (pass an illegal receiver), get and check it
    ASSERT_EQ(RETCODE_FAIL, portCapture.getUiTestVersion(ReceiveBuffer {.data = nullptr}));
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.getAndClearLastError(&errCode, recv));
    ASSERT_NE(RETCODE_SUCCESS, errCode);
    constexpr string_view expMsg = "Illegal buffer pointer";
    ASSERT_EQ(0, memcmp(expMsg.data(), buf, expMsg.length()));
    // error should be cleared after get
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.getAndClearLastError(&errCode, recv));
    ASSERT_EQ(RETCODE_SUCCESS, errCode);
    ASSERT_EQ(0, outSize);
}

static RetCode PrintLog(UiTestPort &port, int32_t level, const char *format, ...)
{
    auto tag = Text { .data = "TAG", .size = strlen("TAG") };
    auto fmt = Text { .data = format, .size = format == nullptr ? 0: strlen(format) };
    va_list ap;
    va_start(ap, format);
    auto ret = port.printLog(level, tag, fmt, ap);
    va_end(ap);
    return ret;
}

TEST_F(ExtensionTest, testPrintLog)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    static constexpr int32_t levelDebug = 3;
    ASSERT_TRUE(ExecuteExtension("0", 0, nullptr));
    // print normal log
    ASSERT_EQ(RETCODE_SUCCESS, PrintLog(portCapture, levelDebug, "Name is %s, age is %d", "wyz", 1));
    // print with illegal level should cause failure
    ASSERT_EQ(RETCODE_FAIL, PrintLog(portCapture, levelDebug - 1, "Name is %s, age is %d", "wyz", 1));
    // print with illegal format should cause failure
    ASSERT_EQ(RETCODE_FAIL, PrintLog(portCapture, levelDebug - 1, "Name is %s, age is %d", "wyz", "1"));
    // print with null format should cause failure
    ASSERT_EQ(RETCODE_FAIL, PrintLog(portCapture, levelDebug, nullptr));
    // print with too-long string should cause failure
    auto chars = ".....................................................................";
    auto ret = PrintLog(portCapture, levelDebug, "%s%s%s%s%s%s%s%s", chars, chars, chars,
                        chars, chars, chars, chars, chars, chars, chars);
    ASSERT_EQ(RETCODE_FAIL, ret);
}

struct CallThroughMessageTestParams {
    const char *inMsg;
    ReceiveBuffer recv;
    bool *isFatalRecv;
    const char *expOutMsg;
    bool expSuccess;
};

static constexpr auto CALL_NORMAL = R"({"api":"BackendObjectsCleaner","this":null,"args":[]})";
static constexpr auto CALL_ILLJSON = R"({xxxx})";
static constexpr auto CALL_NO_API = R"({"this":null,"args":[]})";
static constexpr auto CALL_NO_THIS = R"({"api":"BackendObjectsCleaner","args":[]})";
static constexpr auto CALL_NO_ARGS = R"({"api":"BackendObjectsCleaner","this":null})";
static constexpr auto CALL_ILL_API = R"({"api":0,"this":null,"args":[]})";
static constexpr auto CALL_ILL_THIS = R"({"api":"BackendObjectsCleaner","this":false,"args":[]})";
static constexpr auto CALL_ILL_ARGS = R"({"api":"BackendObjectsCleaner","this":null,"args":""})";
static constexpr size_t BUF_SIZE = 128;
static uint8_t g_rBuf[BUF_SIZE] = { 0 };
static size_t g_rSize = 0;
static bool g_rFatal = false;

static constexpr CallThroughMessageTestParams PARAM_SUITE[] = {
    {CALL_NORMAL, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, R"({"result":null})", true},
    {nullptr, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "Null message", false},
    {CALL_NORMAL, ReceiveBuffer {nullptr, 0, &g_rSize}, &g_rFatal, "Null output buffer", false},
    {CALL_NORMAL, ReceiveBuffer {g_rBuf, BUF_SIZE, nullptr}, &g_rFatal, "Null output size pointer", false},
    {CALL_NORMAL, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, nullptr, "Null fatalError output pointer", false},
    {CALL_ILLJSON, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "parse json failed", false},
    {CALL_NO_API, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "api/this/args property missing", false},
    {CALL_NO_THIS, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "api/this/args property missing", false},
    {CALL_NO_ARGS, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "api/this/args property missing", false},
    {CALL_ILL_API, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "Illegal api value type", false},
    {CALL_ILL_THIS, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "Illegal thisRef type", false},
    {CALL_ILL_ARGS, ReceiveBuffer {g_rBuf, BUF_SIZE, &g_rSize}, &g_rFatal, "Illegal api args type", false},
};


TEST_F(ExtensionTest, testCallThroughMessage)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    ASSERT_TRUE(ExecuteExtension("0", 0, nullptr));
    LowLevelFunctions llfs;
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.initLowLevelFunctions(&llfs));
    for (size_t index = 0; index < sizeof(PARAM_SUITE) / sizeof(PARAM_SUITE[0]); index++) {
        auto &suite = PARAM_SUITE[index];
        auto callIn = Text { .data = suite.inMsg, .size = suite.inMsg == nullptr? 0 : strlen(suite.inMsg) };
        auto ret = llfs.callThroughMessage(callIn, suite.recv, suite.isFatalRecv);
        ASSERT_EQ(suite.expSuccess, ret == RETCODE_SUCCESS) << "Unexpected ret code at index " << index;
        if (suite.recv.data != nullptr && suite.recv.size != nullptr) {
            auto outMsg = reinterpret_cast<const char *>(suite.recv.data);
            auto checkOutMsg = strstr(outMsg, suite.expOutMsg) != nullptr;
            ASSERT_TRUE(checkOutMsg) << "Unexpected output message at index " << index << ": " << outMsg;
        }
    }
}

static string g_recordCapture;
static void Callback(Text bytes)
{
    g_recordCapture = string(reinterpret_cast<const char *>(bytes.data), bytes.size);
}

TEST_F(ExtensionTest, testSetCallbackMessageHandler)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    ASSERT_TRUE(ExecuteExtension("0", 0, nullptr));
    LowLevelFunctions llfs;
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.initLowLevelFunctions(&llfs));
    ASSERT_NE(RETCODE_SUCCESS, llfs.setCallbackMessageHandler(nullptr));
    ASSERT_EQ(RETCODE_SUCCESS, llfs.setCallbackMessageHandler(Callback));
}

TEST_F(ExtensionTest, testCaptures)
{
    if (!CheckExtensionLibExist()) {
        return;
    }
    ASSERT_TRUE(ExecuteExtension("0", 0, nullptr));
    LowLevelFunctions llfs;
    ASSERT_EQ(RETCODE_SUCCESS, portCapture.initLowLevelFunctions(&llfs));
    auto goodName = Text{ .data = "recordUiAction", .size = strlen("recordUiAction") };
    auto goodOpt = Text{ .data = "{}", .size = strlen("{}") };
    auto illName = Text{ .data = "haha", .size = strlen("haha") };
    auto illOpt = Text{ .data = "{xxxx}", .size = strlen("{xxxx}") };
    uint8_t errBuf[BUF_SIZE];
    size_t msgSizeRecv = 0;
    int32_t codeRecv = 0;
    ReceiveBuffer msgRecv = { .data = errBuf, .capacity = BUF_SIZE, .size = &msgSizeRecv };
    // test illegal inputs
    ASSERT_NE(RETCODE_SUCCESS, llfs.startCapture(illName, Callback, goodOpt));
    portCapture.getAndClearLastError(&codeRecv, msgRecv);
    ASSERT_EQ(0, memcmp(errBuf, "Illegal capture type: haha", msgSizeRecv));
    ASSERT_NE(RETCODE_SUCCESS, llfs.startCapture(goodName, nullptr, goodOpt));
    portCapture.getAndClearLastError(&codeRecv, msgRecv);
    ASSERT_EQ(0, memcmp(errBuf, "Illegal name/callback", msgSizeRecv));
    ASSERT_NE(RETCODE_SUCCESS, llfs.startCapture(goodName, Callback, illOpt));
    portCapture.getAndClearLastError(&codeRecv, msgRecv);
    ASSERT_EQ(0, memcmp(errBuf, "Illegal optJson format", msgSizeRecv));
    ASSERT_NE(RETCODE_SUCCESS, llfs.stopCapture(illName));
    portCapture.getAndClearLastError(&codeRecv, msgRecv);
    ASSERT_EQ(0, memcmp(errBuf, "Illegal capture type: haha", msgSizeRecv));
    // test normal inputs
    auto firstStartRet = llfs.startCapture(goodName, Callback, goodOpt);
    auto secondStartRet = llfs.startCapture(goodName, Callback, goodOpt);
    ASSERT_EQ(RETCODE_SUCCESS, firstStartRet);
    ASSERT_NE(RETCODE_SUCCESS, secondStartRet);
    portCapture.getAndClearLastError(&codeRecv, msgRecv);
    ASSERT_EQ(0, memcmp(errBuf, "Capture already running: recordUiAction", msgSizeRecv));
    // stop always returns true event if not running
    ASSERT_EQ(RETCODE_SUCCESS, llfs.stopCapture(goodName));
    ASSERT_EQ(RETCODE_SUCCESS, llfs.stopCapture(goodName));
    // check the capture data can be delivered to the callback
    // the ui_record implementation is mocke @line:31, set a dummy event here
    g_dummyRecordEvent = R"({"data":"dummy_record"})";
    ASSERT_EQ(RETCODE_SUCCESS, llfs.startCapture(goodName, Callback, goodOpt));
    // callback should receive the event
    sleep(1); // let record fire
    ASSERT_STREQ(g_dummyRecordEvent, g_recordCapture.c_str()) << "Capture not callbacked!";
    ASSERT_EQ(RETCODE_SUCCESS, llfs.stopCapture(goodName));
}
}