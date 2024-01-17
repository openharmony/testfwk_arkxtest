/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <csignal>
#include <dlfcn.h>
#include <file_ex.h>
#include <hilog/log.h>
#include <mutex>
#include <securec.h>
#include <set>
#include <string_view>
#include "common_utilities_hpp.h"
#include "frontend_api_handler.h"
#include "ui_action.h"
#include "ui_driver.h"
#include "ui_record.h"
#include "screen_copy.h"
#include "extension_executor.h"

namespace OHOS::uitest {
    using namespace std;
    // uitest_extension port definitions =================================>>>>>>>>
    extern "C" {
        typedef int32_t RetCode;
#define RETCODE_SUCCESS 0
#define RETCODE_FAIL (-1)

        struct Text {
            const char *data;
            size_t size;
        };

        struct ReceiveBuffer {
            uint8_t *data;
            size_t capacity;
            size_t *size;
        };

        // enum LogLevel : int32_t { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

        typedef void (*DataCallback)(Text bytes);

        struct LowLevelFunctions {
            RetCode (*callThroughMessage)(Text in, ReceiveBuffer out, bool *fatalError);
            RetCode (*setCallbackMessageHandler)(DataCallback handler);
            RetCode (*atomicTouch)(int32_t stage, int32_t px, int32_t py);
            RetCode (*startCapture)(Text name, DataCallback callback, Text optJson);
            RetCode (*stopCapture)(Text name);
        };

        struct UiTestPort {
            RetCode (*getUiTestVersion)(ReceiveBuffer out);
            RetCode (*printLog)(int32_t level, Text tag, Text format, va_list ap);
            RetCode (*getAndClearLastError)(int32_t *codeOut, ReceiveBuffer msgOut);
            RetCode (*initLowLevelFunctions)(LowLevelFunctions *out);
        };

        // hook function names of UiTestExtension library
#define UITEST_EXTENSION_CALLBACK_ONINIT "UiTestExtension_OnInit"
#define UITEST_EXTENSION_CALLBACK_ONRUN "UiTestExtension_OnRun"
        // proto of UiTestExtensionOnInitCallback: void (UiTestPort port, size_t argc, const char **argv)
        typedef RetCode (*UiTestExtensionOnInitCallback)(UiTestPort, size_t, const char **);
        // proto of UiTestExtensionOnRun
        typedef RetCode (*UiTestExtensionOnRunCallback)();
    };
    // uitest_extension port definitions =================================<<<<<<<<

    static constexpr auto ERR_BAD_ARG = ErrCode::ERR_INVALID_INPUT;
    static constexpr size_t LOG_BUF_SIZE = 512;
    static constexpr LogType type = LogType::LOG_APP;
    static string_view g_version = "";
    static string g_lastErrorMessage = "";
    static int32_t g_lastErrorCode = 0;
    static mutex g_callThroughLock;
    static mutex g_captureLock;
    static mutex g_recordRunningLock;
    static set<string> g_runningCaptures;

#define EXTENSION_API_CHECK(cond, errorMessage, errorCode) \
do { \
    if (!(cond)) { \
        g_lastErrorMessage = (errorMessage); \
        g_lastErrorCode = static_cast<int32_t>(errorCode); \
        LOG_E("EXTENSION_API_CHECK failed: %{public}s", g_lastErrorMessage.c_str()); \
        return RETCODE_FAIL; \
    } \
} while (0)

    static RetCode WriteToBuffer(ReceiveBuffer &buffer, string_view data)
    {
        EXTENSION_API_CHECK(buffer.data != nullptr, "Illegal buffer pointer", ERR_BAD_ARG);
        EXTENSION_API_CHECK(buffer.size != nullptr, "Illegal outLen pointer", ERR_BAD_ARG);
        EXTENSION_API_CHECK(buffer.capacity > data.length(), "Char buffer capacity is not enough", ERR_BAD_ARG);
        memcpy_s(buffer.data, buffer.capacity, data.data(), data.length());
        *(buffer.size) = data.length();
        buffer.data[*(buffer.size)] = 0;
        return RETCODE_SUCCESS;
    }

    static RetCode GetUiTestVersion(ReceiveBuffer buffer)
    {
        return WriteToBuffer(buffer, g_version);
    }

    static RetCode PrintLog(int32_t level, Text label, Text format, va_list ap)
    {
        constexpr uint32_t domain = 0xD003100;
        EXTENSION_API_CHECK(level >= LogRank::DEBUG && level <= LogRank::ERROR, "Illegal log level", ERR_BAD_ARG);
        EXTENSION_API_CHECK(label.data != nullptr && format.data != nullptr, "Illegal log tag/format", ERR_BAD_ARG);
        char buf[LOG_BUF_SIZE];
        EXTENSION_API_CHECK(vsprintf_s(buf, sizeof(buf), format.data, ap) >= 0, format.data, ERR_BAD_ARG);
        HiLogPrint(type, static_cast<LogLevel>(level), domain, label.data, "%{public}s", buf);
        return RETCODE_SUCCESS;
    }

    static RetCode GetAndClearLastError(int32_t *codeOut, ReceiveBuffer msgOut)
    {
        *codeOut = g_lastErrorCode;
        auto ret = WriteToBuffer(msgOut, g_lastErrorMessage);
        // clear error
        g_lastErrorCode = ErrCode::NO_ERROR;
        g_lastErrorMessage = "";
        return ret;
    }

// input-errors of call-through api should also be passed through
#define CALL_THROUGH_CHECK(cond, message, code, asFatalError, fatalPtr) \
do { \
    if (!(cond)) { \
        LOG_E("Check condition (%{public}s) failed: %{public}s", #cond, string(message).c_str()); \
        json errorJson; \
        errorJson["code"] = (code); \
        errorJson["message"] = (message); \
        json replyJson; \
        replyJson["exception"] = move(errorJson); \
        WriteToBuffer(out, replyJson.dump()); \
        if ((asFatalError) && (fatalPtr) != nullptr) { \
            *(fatalPtr) = true; \
        } \
        return RETCODE_FAIL; \
    } \
} while (0)

    static RetCode CallThroughMessage(Text in, ReceiveBuffer out, bool *fatalError)
    {
        auto ptr = fatalError;
        CALL_THROUGH_CHECK(g_callThroughLock.try_lock(), "Disallow concurrent use", ERR_API_USAGE, false, ptr);
        unique_lock<mutex> guard(g_callThroughLock, std::adopt_lock);
        using namespace nlohmann;
        using VT = nlohmann::detail::value_t;
        static auto server = FrontendApiServer::Get();
        *fatalError = false;
        CALL_THROUGH_CHECK(in.data != nullptr, "Null message", ERR_BAD_ARG, true, ptr);
        CALL_THROUGH_CHECK(out.data != nullptr, "Null output buffer", ERR_BAD_ARG, true, ptr);
        CALL_THROUGH_CHECK(out.size != nullptr, "Null output size pointer", ERR_BAD_ARG, true, ptr);
        CALL_THROUGH_CHECK(fatalError != nullptr, "Null fatalError output pointer", ERR_BAD_ARG, true, ptr);
        auto message = json::parse(in.data, nullptr, false);
        CALL_THROUGH_CHECK(!message.is_discarded(), "Illegal messsage, parse json failed", ERR_BAD_ARG, true, ptr);
        auto hasProps = message.contains("api") && message.contains("this") && message.contains("args");
        CALL_THROUGH_CHECK(hasProps, "Illegal messsage, api/this/args property missing", ERR_BAD_ARG, true, ptr);
        auto api = message["api"];
        auto caller = message["this"];
        auto params = message["args"];
        auto nullThis = caller.type() == VT::null;
        CALL_THROUGH_CHECK(api.type() == VT::string, "Illegal api value type", ERR_BAD_ARG, true, ptr);
        CALL_THROUGH_CHECK(caller.type() == VT::string || nullThis, "Illegal thisRef type", ERR_BAD_ARG, true, ptr);
        CALL_THROUGH_CHECK(params.type() == VT::array, "Illegal api args type", ERR_BAD_ARG, true, ptr);
        auto call = ApiCallInfo {
            .apiId_ = api.get<string>(),
            .callerObjRef_ = nullThis ? "" : caller.get<string>(),
            .paramList_ = move(params)
        };
        auto reply = ApiReplyInfo();
        server.Call(call, reply);
        const auto errCode = reply.exception_.code_;
        const auto isFatalErr = errCode == ErrCode::INTERNAL_ERROR || errCode == ErrCode::ERR_INTERNAL;
        CALL_THROUGH_CHECK(errCode == ErrCode::NO_ERROR, reply.exception_.message_.c_str(), errCode, isFatalErr, ptr);
        json result;
        result["result"] = move(reply.resultValue_);
        return WriteToBuffer(out, result.dump());
    }

    static RetCode SetCallbackMessageHandler(DataCallback handler)
    {
        EXTENSION_API_CHECK(handler != nullptr, "Null callback handler!", ERR_BAD_ARG);
        FrontendApiServer::Get().SetCallbackHandler([handler](const ApiCallInfo& in, ApiReplyInfo& out) {
            nlohmann::json msgJson;
            msgJson["api"] = in.apiId_;
            msgJson["this"] = in.callerObjRef_;
            msgJson["args"] = in.paramList_;
            auto msg = msgJson.dump();
            handler(Text {msg.data(), msg.length()});
        });
        return RETCODE_SUCCESS;
    }

    static RetCode AtomicTouch(int32_t stage, int32_t px, int32_t py)
    {
        static auto driver = UiDriver();
        EXTENSION_API_CHECK(stage >= ActionStage::DOWN && stage <= ActionStage::UP, "Illegal stage", ERR_BAD_ARG);
        auto touch = GenericAtomicAction(static_cast<ActionStage>(stage), Point(px, py));
        auto err = ApiCallErr(NO_ERROR);
        UiOpArgs uiOpArgs;
        driver.PerformTouch(touch, uiOpArgs, err);
        EXTENSION_API_CHECK(err.code_ == NO_ERROR, err.message_, err.code_);
        return RETCODE_SUCCESS;
    }

    static RetCode StopCapture(Text name)
    {
        EXTENSION_API_CHECK(name.data != nullptr, "Illegal name/callback", ERR_BAD_ARG);
        unique_lock<mutex> guard(g_captureLock);
        if (g_runningCaptures.find(name.data) == g_runningCaptures.end()) {
            return RETCODE_SUCCESS;
        }
        if (strcmp(name.data, "copyScreen") == 0) {
            StopScreenCopy();
        } else if (strcmp(name.data, "recordUiAction") == 0) {
            UiDriverRecordStop();
            g_recordRunningLock.lock(); // this cause waiting for recordThread exit
            g_recordRunningLock.unlock();
        } else if (strcmp(name.data, "dumpLayout") != 0) {
            EXTENSION_API_CHECK(false, string("Illegal capture type: ") + name.data, ERR_BAD_ARG);
        }
        g_runningCaptures.erase(name.data);
        return RETCODE_SUCCESS;
    }

    static RetCode StartCapture(Text name, DataCallback callback, Text optJson)
    {
        static auto driver = UiDriver();
        EXTENSION_API_CHECK(name.data != nullptr && callback != nullptr, "Illegal name/callback", ERR_BAD_ARG);
        nlohmann::json options = nullptr;
        if (optJson.data != nullptr) {
            options = nlohmann::json::parse(optJson.data, nullptr, false);
            EXTENSION_API_CHECK(!options.is_discarded(), "Illegal optJson format", ERR_BAD_ARG);
        }
        unique_lock<mutex> guard(g_captureLock);
        const auto running = g_runningCaptures.find(name.data) != g_runningCaptures.end();
        EXTENSION_API_CHECK(!running, string("Capture already running: ") + name.data, -1);
        g_runningCaptures.insert(name.data);
        guard.unlock();
        if (strcmp(name.data, "dumpLayout") == 0) {
            nlohmann::json tree;
            ApiCallErr err(NO_ERROR);
            driver.DumpUiHierarchy(tree, false, false, err);
            g_runningCaptures.erase("dumpLayout"); // dumpLayout is sync&once
            EXTENSION_API_CHECK(err.code_ == NO_ERROR, err.message_, err.code_);
            auto layout = tree.dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace);
            callback(Text{layout.c_str(), layout.length()});
        } else if (strcmp(name.data, "copyScreen") == 0) {
            float scale = 1.0f;
            if (options.type() == nlohmann::detail::value_t::object && options.contains("scale")) {
                nlohmann::json val = options["scale"];
                EXTENSION_API_CHECK(val.type() == detail::value_t::number_float, "Illegal scale value", ERR_BAD_ARG);
                scale = val.get<float>();
            }
            StartScreenCopy(scale, [callback](uint8_t *data, size_t len) {
                callback(Text{reinterpret_cast<const char *>(data), len});
                free(data);
            });
        } else if (strcmp(name.data, "recordUiAction") == 0) {
            StopCapture(name);
            auto recordThread = thread([callback]() {
                g_recordRunningLock.lock();
                UiDriverRecordStart([callback](nlohmann::json record) {
                    auto data = record.dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace);
                    callback(Text{data.c_str(), data.length()});
                    }, "");
                g_recordRunningLock.unlock();
            });
            recordThread.detach();
        } else {
            EXTENSION_API_CHECK(false, string("Illegal capture type: ") + name.data, ERR_BAD_ARG);
        }
        return RETCODE_SUCCESS;
    }

    static RetCode InitLowLevelFunctions(LowLevelFunctions *out)
    {
        EXTENSION_API_CHECK(out != nullptr, "Null LowLevelFunctions recveive pointer", ERR_BAD_ARG);
        out->callThroughMessage = CallThroughMessage;
        out->setCallbackMessageHandler = SetCallbackMessageHandler;
        out->atomicTouch = AtomicTouch;
        out->startCapture = StartCapture;
        out->stopCapture = StopCapture;
        return RETCODE_SUCCESS;
    }

    bool ExecuteExtension(string_view version)
    {
        g_version = version;
        constexpr string_view extensionPath = "/data/local/tmp/agent.so";
        if (!OHOS::FileExists(extensionPath.data())) {
            LOG_E("Client nativeCode not exist");
            return false;
        }
        auto handle = dlopen(extensionPath.data(), RTLD_LAZY);
        if (handle == nullptr) {
            LOG_E("Dlopen %{public}s failed: %{public}s", extensionPath.data(), strerror(errno));
            return false;
        }
        auto symInit = dlsym(handle, UITEST_EXTENSION_CALLBACK_ONINIT);
        if (symInit == nullptr) {
            LOG_E("Dlsym failed:" UITEST_EXTENSION_CALLBACK_ONINIT);
            dlclose(handle);
            return false;
        }
        auto symRun = dlsym(handle, UITEST_EXTENSION_CALLBACK_ONRUN);
        if (symRun == nullptr) {
            LOG_E("Dlsym failed: " UITEST_EXTENSION_CALLBACK_ONRUN);
            dlclose(handle);
            return false;
        }
        auto initFunction = reinterpret_cast<UiTestExtensionOnInitCallback>(symInit);
        auto runFunction = reinterpret_cast<UiTestExtensionOnRunCallback>(symRun);
        auto port = UiTestPort {
            .getUiTestVersion = GetUiTestVersion,
            .printLog = PrintLog,
            .getAndClearLastError = GetAndClearLastError,
            .initLowLevelFunctions = InitLowLevelFunctions,
        };
        if (initFunction(port, 0, nullptr) != RETCODE_SUCCESS) {
            LOG_I("Initialize UiTest extension failed");
            dlclose(handle);
            return false;
        }
        auto ret = runFunction() == RETCODE_SUCCESS;
        dlclose(handle);
        return ret;
    }
} // namespace OHOS::uitest
