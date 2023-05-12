/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <set>
#include <string>
#include <unistd.h>
#include <uv.h>
#include "json.hpp"
#include "common_utilities_hpp.h"
#include "ui_event_observer_napi.h"

namespace OHOS::uitest {
    using namespace nlohmann;
    using namespace std;
    
    /** Cached reference of UiEventObserver and JsCallbackFunction, key is the unique id.*/
    static map<string, napi_ref> g_jsRefs;
    static size_t g_incJsCbId = 0;

    UiEventObserverNapi &UiEventObserverNapi::Get()
    {
        static UiEventObserverNapi instance;
        return instance;
    }

    void UiEventObserverNapi::PreprocessCallOnce(napi_env env, ApiCallInfo &call, napi_value jsThis,
                                                 napi_value* argv, ApiCallErr &err)
    {
        DCHECK(env != nullptr);
        DCHECK(jsThis != nullptr);
        auto &paramList = call.paramList_;
        // pop js_cb_function and save at local side
        if (paramList.size() < 1 || paramList.at(0).type() != detail::value_t::string) {
            LOG_E("Missing event type argument");
            err = ApiCallErr(ERR_INVALID_INPUT, "Missing event type argument");
            return;
        }
        auto event = paramList.at(0).get<string>();
        napi_valuetype type = napi_undefined;
        if (paramList.size() > 1) {
            NAPI_CALL_RETURN_VOID(env, napi_typeof(env, argv[1], &type));
        }
        if (type != napi_function) {
            LOG_E("Invalid callback function argument");
            err = ApiCallErr(ERR_INVALID_INPUT, "Invalid callback function argument");
            return;
        }
        if (g_jsRefs.find(call.callerObjRef_) == g_jsRefs.end()) {
            // hold the jsObserver to avoid it be recycled, it's needed in performing callback
            napi_ref ref = nullptr;
            NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, jsThis, 1, &ref));
            LOG_D("Hold reference of %{public}s", call.callerObjRef_.c_str());
            g_jsRefs.insert( {call.callerObjRef_, ref} );
        }
        auto jsCallback = argv[1];
        const auto jsCbId = string("js_callback#") + to_string(++g_incJsCbId);
        // hold the const  to avoid it be recycled, it's needed in performing callback
        napi_ref ref = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, jsCallback, 1, &ref));
        LOG_I("CbId = %{public}s, CbRef = %{public}p", jsCbId.c_str(), ref);
        LOG_D("Hold reference of %{public}s", jsCbId.c_str());
        g_jsRefs.insert( {jsCbId, ref} );
        // pass jsCbId instread of the function body
        paramList.at(1) = jsCbId; // observer.once(type, cllbackId)
    }

    struct EventCallbackContext {
        napi_env env;
        string observerId;
        string callbackId;
        napi_ref observerRef;
        napi_ref callbackRef;
        bool releaseObserver; // if or not release observer object after performing this callback
        bool releaseCallback; // if or not release callback function after performing this callback
        nlohmann::json elmentInfo;
    };

    static void InitCallbackContext(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out, EventCallbackContext &ctx)
    {
        LOG_I("Handler api callback: %{public}s", in.apiId_.c_str());
        if (in.apiId_ != "UiEventObserver.once") {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Api dose not support callback: " + in.apiId_);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
        // observer.once(UiElementInfo, cllbackId, releaseObserver, releaseCallback)
        DCHECK(env != nullptr);
        DCHECK(in.paramList_.size() > INDEX_ZERO && in.paramList_.at(INDEX_ZERO).type() == detail::value_t::object);
        DCHECK(in.paramList_.size() > INDEX_ONE && in.paramList_.at(INDEX_ONE).type() == detail::value_t::string);
        DCHECK(in.paramList_.size() > INDEX_TWO && in.paramList_.at(INDEX_TWO).type() == detail::value_t::boolean);
        DCHECK(in.paramList_.size() > INDEX_THREE && in.paramList_.at(INDEX_THREE).type() == detail::value_t::boolean);
        auto &observerId = in.callerObjRef_;
        auto &elementInfo = in.paramList_.at(INDEX_ZERO);
        auto callbackId = in.paramList_.at(INDEX_ONE).get<string>();
        LOG_I("Begin to callback UiEvent: observer=%{public}s, callback=%{public}s",
              observerId.c_str(), callbackId.c_str());
        auto findObserver = g_jsRefs.find(observerId);
        auto findCallback = g_jsRefs.find(callbackId);
        if (findObserver == g_jsRefs.end()) {
            out.exception_ = ApiCallErr(INTERNAL_ERROR, "UiEventObserver is not referenced: " + observerId);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
        if (findCallback == g_jsRefs.end()) {
            out.exception_ = ApiCallErr(INTERNAL_ERROR, "JsCallbackFunction is not referenced: " + callbackId);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
        ctx.env = env;
        ctx.observerId = observerId;
        ctx.callbackId = callbackId;
        ctx.observerRef = findObserver->second;
        ctx.callbackRef = findCallback->second;
        ctx.elmentInfo = move(elementInfo);
        ctx.releaseObserver = in.paramList_.at(INDEX_TWO).get<bool>();
        ctx.releaseCallback = in.paramList_.at(INDEX_THREE).get<bool>();
    }

    /**Handle api callback from server end.*/
    void UiEventObserverNapi::HandleEventCallback(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        auto context = new EventCallbackContext();
        InitCallbackContext(env, in, out, *context);
        if (out.exception_.code_ != NO_ERROR) {
            delete context;
            LOG_W("InitCallbackContext failed, cannot perform callback");
            return;
        }
        uv_loop_s *loop = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_get_uv_event_loop(env, &loop));
        auto work = new uv_work_t;
        work->data = context;
        (void)uv_queue_work(loop, work, [](uv_work_t *) {}, [](uv_work_t *work, int status) {
            auto ctx = reinterpret_cast<EventCallbackContext *>(work->data);
            napi_value global = nullptr;
            napi_value jsonProp = nullptr;
            napi_value parseFunc = nullptr;
            napi_get_global(ctx->env, &global);
            napi_get_named_property(ctx->env, global, "JSON", &jsonProp);
            napi_get_named_property(ctx->env, jsonProp, "parse", &parseFunc);
            napi_value argv[1] = { nullptr };
            napi_create_string_utf8(ctx->env, ctx->elmentInfo.dump().c_str(), NAPI_AUTO_LENGTH, argv);
            napi_call_function(ctx->env, jsonProp, parseFunc, 1, argv, argv);
            napi_value jsCallback = nullptr;
            napi_get_reference_value(ctx->env, ctx->callbackRef, &jsCallback);
            napi_call_function(ctx->env, nullptr, jsCallback, 1, argv, argv);
            // accessing callback from js-caller, do check and warn
            auto hasError = false;
            napi_is_exception_pending(ctx->env, &hasError);
            if (hasError) {
                LOG_W("Exception raised during call js_cb_function!");
            }
            if (ctx->releaseObserver) {
                LOG_D("Unref jsObserver: %{public}s", ctx->observerId.c_str());
                napi_delete_reference(ctx->env, ctx->observerRef);
                g_jsRefs.erase(ctx->observerId);
            }
            if (ctx->releaseCallback) {
                LOG_D("Unref jsCallback: %{public}s", ctx->callbackId.c_str());
                napi_delete_reference(ctx->env, ctx->callbackRef);
                g_jsRefs.erase(ctx->callbackId);
            }
            delete work;
            delete ctx;
        });
    }
}
