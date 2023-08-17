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

#include <js_runtime.h>
#include <event_runner.h>
#include <napi/native_api.h>
#include <napi/native_node_api.h>
#include <json.hpp>
#include <file_ex.h>
#include <future>
#include <set>
#include <unistd.h>
#include "ui_driver.h"
#include "common_utilities_hpp.h"
#include "screen_copy.h"
#include "ui_record.h"
#include "js_client_loader.h"

namespace OHOS::uitest {
    using namespace std;
    constexpr string_view JS_CODE_PATH = "data/local/tmp/app.abc";
    constexpr string_view CAPTURE_SCREEN = "copyScreen";
    constexpr string_view CAPTURE_LAYOUT = "dumpLayout";
    constexpr string_view CAPTURE_UIACTION = "recordUiAction";

    class CaptureContext {
    public:
        CaptureContext() = default;
        // capture options passed from js
        float scale = 1.0; // for screen cap
        // capture enviroment and data buffer
        string type;
        napi_env napiEnv = nullptr;
        napi_ref cbRef = nullptr;
        uint8_t *data = nullptr;
        size_t dataLen = 0;
        static constexpr size_t DATA_CAPACITY = 2 * 1024 * 1024;
    };

    static void NopNapiFinalizer(napi_env /**env*/, void* /**finalize_data*/, void* /**finalize_hint*/) {}

    static void CallbackCaptureResultToJs(const CaptureContext& context)
    {
        uv_loop_s *loop = nullptr;
        NAPI_CALL_RETURN_VOID(context.napiEnv, napi_get_uv_event_loop(context.napiEnv, &loop));
        auto work = new uv_work_t;
        auto contextCopy = new CaptureContext();
        *contextCopy = context; // copy-assign
        work->data = contextCopy;
        (void)uv_queue_work(loop, work, [](uv_work_t *) {}, [](uv_work_t* work, int status) {
            auto ctx = (CaptureContext*)work->data;
            auto env = ctx->napiEnv;
            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(env, &scope);
            if (scope == nullptr) {
                return;
            }
            napi_value callback = nullptr;
            napi_get_reference_value(env, ctx->cbRef, &callback);
            // use "napi_create_external_arraybuffer" to shared data with JS without new and copy
            napi_value arrBuf = nullptr;
            napi_create_external_arraybuffer(env, ctx->data, ctx->dataLen, NopNapiFinalizer, nullptr, &arrBuf);
            napi_value undefined = nullptr;
            napi_get_undefined(env, &undefined);
            LOG_I("Invoke js callback");
            napi_value argv[1] = { arrBuf };
            napi_call_function(env, undefined, callback, 1, argv, &arrBuf);
            auto errorPending = false;
            napi_is_exception_pending(env, &errorPending);
            if (errorPending) {
                LOG_W("Exception raised during invoke js callback");
                napi_get_and_clear_last_exception(env, &arrBuf);
            }
            if (ctx->type == CAPTURE_SCREEN) {
                free(ctx->data);
            }
            if (work != nullptr) {
                delete work;
            }
            napi_close_handle_scope(env, scope);
            delete ctx;
        });
    }

    static void RecordStart(CaptureContext& context, string type)
    {
        static uint8_t uiActionBuf[CaptureContext::DATA_CAPACITY] = {0};
        std::string modeOpt;
        auto handler = [type, context](nlohmann::json data) {
            const auto jsonStr = data.dump();
            jsonStr.copy((char *)uiActionBuf, jsonStr.length());
            uiActionBuf[jsonStr.length()] = 0;
            auto ctx = context;
            ctx.data = uiActionBuf;
            ctx.dataLen = jsonStr.length();
            CallbackCaptureResultToJs(ctx);
        };
        UiDriverRecordStop();
        LOG_I("Start record uiaction");
        UiDriverRecordStart(handler, modeOpt);
    }

    static void ScreenCopyStart(CaptureContext& context, string type)
    {
        auto handler = [type, context](uint8_t * data, size_t len) {
            auto ctx = context;
            ctx.data = data;
            ctx.dataLen = len;
            CallbackCaptureResultToJs(ctx);
        };
        StartScreenCopy(context.scale, handler);
    }

    static void UpdateCaptureState(CaptureContext&& context, bool active)
    {
        static auto driver = UiDriver();
        static set<string> runningCaptures;
        static mutex stateLock;
        static uint8_t dumpLayoutBuf[CaptureContext::DATA_CAPACITY] = {0};
        auto &type = context.type;
        stateLock.lock();
        const auto running = runningCaptures.find(type) != runningCaptures.end();
        if (running && active) {
            LOG_W("Capture %{public}s already running, call stop first!", context.type.c_str());
            stateLock.unlock();
            return;
        }
        if (active) {
            runningCaptures.insert(type);
        } else {
            runningCaptures.erase(type);
        }
        stateLock.unlock();
        if (type == CAPTURE_SCREEN && !active) {
            StopScreenCopy();
        } else if (type == CAPTURE_SCREEN && active) {
            ScreenCopyStart(context, type);
        } else if (type == CAPTURE_LAYOUT && active) {
            auto dom = nlohmann::json();
            auto err = ApiCallErr(NO_ERROR);
            driver.DumpUiHierarchy(dom, false, err);
            if (err.code_ == NO_ERROR) {
                const auto jsonStr = dom.dump();
                jsonStr.copy((char *)dumpLayoutBuf, jsonStr.length());
                dumpLayoutBuf[jsonStr.length()] = 0;
                context.data = dumpLayoutBuf;
                context.dataLen = jsonStr.length();
                CallbackCaptureResultToJs(context);
            } else {
                LOG_W("DumpLayout failed: %{public}s", err.message_.c_str());
            }
            stateLock.lock();
            runningCaptures.erase(type);
            stateLock.unlock();
        } else if (type == CAPTURE_UIACTION && active) {
            RecordStart(context, type);
        } else if (type == CAPTURE_UIACTION && !active) {
            UiDriverRecordStop();
        }
    }

    static void ParseCaptureOptions(napi_env env, string_view type, napi_value opt, CaptureContext& out)
    {
        constexpr size_t OPTION_MAX_LEN = 256;
        napi_value global = nullptr;
        napi_value jsonProp = nullptr;
        napi_value jsonFunc = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));
        NAPI_CALL_RETURN_VOID(env, napi_get_named_property(env, global, "JSON", &jsonProp));
        NAPI_CALL_RETURN_VOID(env, napi_get_named_property(env, jsonProp, "stringify", &jsonFunc));
        napi_value jsStr = nullptr;
        napi_value argv[1] = {opt};
        NAPI_CALL_RETURN_VOID(env, napi_call_function(env, jsonProp, jsonFunc, 1, argv, &jsStr));
        size_t len = 0;
        char buf[OPTION_MAX_LEN] = {0};
        NAPI_CALL_RETURN_VOID(env, napi_get_value_string_utf8(env, jsStr, buf, OPTION_MAX_LEN, &len));
        auto cppStr = string(buf, len);
        auto optJson = nlohmann::json::parse(cppStr, nullptr, false);
        NAPI_ASSERT_RETURN_VOID(env, !optJson.is_discarded(), "Bad option json string");
        LOG_I("CaptureOption=%{public}s", cppStr.c_str());
        if (optJson.contains("scale") && optJson["scale"].type() == nlohmann::detail::value_t::number_float) {
            out.scale = optJson["scale"].get<float>();
        }
    }

    // JS-function-proto: startCapture(type: string, resultCb: (data:ArrayBuffer)=>void, options?:any):void
    // JS-function-proto: stopCapture(type: string): void
    template<bool kOn = true>
    static napi_value SetCaptureEventJsCallback(napi_env env, napi_callback_info info)
    {
        static const set<string> TYPES {string(CAPTURE_SCREEN), string(CAPTURE_LAYOUT), string(CAPTURE_UIACTION)};
        constexpr size_t MIN_ARGC = 1;
        constexpr size_t MAX_ARGC = 3;
        constexpr size_t BUF_SIZE = 32;
        static napi_value argv[MAX_ARGC] = {nullptr};
        static char buf[BUF_SIZE] = { 0 };
        size_t argc = MAX_ARGC;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_ASSERT(env, argc >= MIN_ARGC, "Illegal argument count");
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        NAPI_ASSERT(env, type == napi_string, "Illegal arg[0], string required");
        size_t typeLen = 0;
        NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], buf, BUF_SIZE - 1, &typeLen));
        auto capType = string(buf, typeLen);
        auto support = TYPES.find(capType) != TYPES.end();
        NAPI_ASSERT(env, support == true, "Invalid event name");
        CaptureContext context;
        if constexpr(kOn) {
            NAPI_ASSERT(env, argc > MIN_ARGC, "Need callback function argument");
            NAPI_CALL(env, napi_typeof(env, argv[1], &type));
            NAPI_ASSERT(env, type == napi_function, "Illegal arg[1], function required");
            napi_ref callbackRef = nullptr;
            NAPI_CALL(env, napi_create_reference(env, argv[1], 1, &callbackRef));
            context.cbRef = callbackRef;
            if (argc > MIN_ARGC + 1) {
                NAPI_CALL(env, napi_typeof(env, argv[MIN_ARGC + 1], &type));
                NAPI_ASSERT(env, type == napi_object, "Illegal options argument");
                ParseCaptureOptions(env, capType, argv[MIN_ARGC + 1], context);
            }
        }
        LOG_I("Update context for capture: %{public}s, active=%{public}d", capType.c_str(), kOn);
        context.type = move(capType);
        context.napiEnv = env;
        auto asyncJob = thread(UpdateCaptureState, move(context), kOn);
        asyncJob.detach();
        LOG_I("Return");
        return nullptr;
    }

    static bool BindAddonProperties(napi_env env, string_view version)
    {
        if (env == nullptr) {
            LOG_E("Internal error, napi_env is nullptr");
            return false;
        }
        napi_value kit = nullptr;
        NAPI_CALL_BASE(env, napi_create_object(env, &kit), false);
        napi_value global = nullptr;
        NAPI_CALL_BASE(env, napi_get_global(env, &global), false);
        napi_value serverVersion = nullptr;
        NAPI_CALL_BASE(env, napi_create_string_utf8(env, version.data(), version.length(), &serverVersion), false);
        napi_property_descriptor kitProps[] = {
            DECLARE_NAPI_STATIC_PROPERTY("serverVersion", serverVersion),
            DECLARE_NAPI_STATIC_FUNCTION("startCapture", SetCaptureEventJsCallback<true>),
            DECLARE_NAPI_STATIC_FUNCTION("stopCapture", SetCaptureEventJsCallback<false>),
        };
        NAPI_CALL_BASE(env, napi_define_properties(env, kit, sizeof(kitProps)/sizeof(kitProps[0]), kitProps), false);

        napi_property_descriptor globalProps[] = {
            DECLARE_NAPI_STATIC_PROPERTY("uitestAddonKit", kit),
        };
        NAPI_CALL_BASE(env, napi_define_properties(env, global, 1, globalProps), false);
        return true;
    }

    bool RunJsClient(string_view serverVersion)
    {
        LOG_I("Enter RunJsClient, serverVersion=%{public}s", serverVersion.data());
        if (!OHOS::FileExists(JS_CODE_PATH.data())) {
            LOG_E("Client jsCode not exist");
            return false;
        }
        OHOS::AbilityRuntime::Runtime::Options opt;
        opt.lang = OHOS::AbilityRuntime::Runtime::Language::JS;
        opt.loadAce = false;
        opt.preload = false;
        opt.isStageModel = true;  // stage model with jsbundle packing
        opt.isBundle = true;
        opt.eventRunner = OHOS::AppExecFwk::EventRunner::Create(false);
        if (opt.eventRunner == nullptr) {
            LOG_E("Create event runner failed");
            return false;
        }
        auto runtime = OHOS::AbilityRuntime::JsRuntime::Create(opt);
        if (runtime == nullptr) {
            LOG_E("Create JsRuntime failed");
            return false;
        }
        if (!BindAddonProperties(reinterpret_cast<napi_env>(&(runtime->GetNativeEngine())), serverVersion)) {
            LOG_E("Bind addon functions failed");
            return false;
        }
        map<string, vector<string>> napiLibPath;
        napiLibPath.insert({"default", {"data/local/tmp"}});
        runtime->SetAppLibPath(napiLibPath);
        // execute single .abc file
        auto execAbcRet = runtime->RunScript(JS_CODE_PATH.data(), "", false);
        LOG_I("Run client jsCode, ret=%{public}d", execAbcRet);
        pthread_setname_np(pthread_self(), "event_runner");
        auto runnerRet = opt.eventRunner->Run();
        LOG_I("Event runner exit, code=%{public}d", runnerRet);
        return EXIT_SUCCESS;
    }
} // namespace OHOS::uitest
