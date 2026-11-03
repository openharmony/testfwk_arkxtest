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

#include <set>
#include <string>
#include <unistd.h>
#include <uv.h>
#include <atomic>
#include <mutex>
#include "nlohmann/json.hpp"
#include "common_utilities_hpp.h"
#include "ui_event_observer_napi.h"

namespace OHOS::uitest {
    using namespace nlohmann;
    using namespace std;
    
    /** Cached reference of UIEventObserver and JsCallbackFunction, key is the unique id.*/
    static map<string, napi_ref> g_jsRefs;
    static mutex g_jsRefsMtx;
    static atomic<size_t> g_incJsCbId { 0 };

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
        size_t callbackIndex = paramList.size() - 1;
        if (paramList.size() > 1) {
            NAPI_CALL_RETURN_VOID(env, napi_typeof(env, argv[callbackIndex], &type));
        }
        if (type != napi_function) {
            LOG_E("Invalid callback function argument");
            err = ApiCallErr(ERR_INVALID_INPUT, "Invalid callback function argument");
            return;
        }
        auto jsCallback = argv[callbackIndex];
        const auto jsCbId = string("js_callback#") + to_string(++g_incJsCbId);
        napi_ref ref = nullptr;
        {
            lock_guard<mutex> guard(g_jsRefsMtx);
            if (g_jsRefs.find(call.callerObjRef_) == g_jsRefs.end()) {
                // hold the jsObserver to avoid it be recycled, it's needed in performing callback
                napi_ref observerRef = nullptr;
                NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, jsThis, 1, &observerRef));
                LOG_D("Hold reference of %{public}s", call.callerObjRef_.c_str());
                g_jsRefs.insert({ call.callerObjRef_, observerRef });
            }
            // hold the const  to avoid it be recycled, it's needed in performing callback
            NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, jsCallback, 1, &ref));
            LOG_I("CbId = %{public}s, CbRef = %{public}p", jsCbId.c_str(), ref);
            g_jsRefs.insert({ jsCbId, ref });
        }
        // pass jsCbId instread of the function body
        paramList.at(callbackIndex) = jsCbId; // observer.once(type, cllbackId)
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
        if (in.apiId_ != "UIEventObserver.once") {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Api dose not support callback: " + in.apiId_);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
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
        napi_ref observerRef = nullptr;
        napi_ref callbackRef = nullptr;
        {
            lock_guard<mutex> guard(g_jsRefsMtx);
            auto findObserver = g_jsRefs.find(observerId);
            auto findCallback = g_jsRefs.find(callbackId);
            if (findObserver != g_jsRefs.end()) {
                observerRef = findObserver->second;
            }
            if (findCallback != g_jsRefs.end()) {
                callbackRef = findCallback->second;
            }
        }
        if (observerRef == nullptr) {
            out.exception_ = ApiCallErr(INTERNAL_ERROR, "UIEventObserver is not referenced: " + observerId);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
        if (callbackRef == nullptr) {
            out.exception_ = ApiCallErr(INTERNAL_ERROR, "JsCallbackFunction is not referenced: " + callbackId);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
        ctx.env = env;
        ctx.observerId = observerId;
        ctx.callbackId = callbackId;
        ctx.observerRef = observerRef;
        ctx.callbackRef = callbackRef;
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
        auto task = [context]() {
            napi_value global = nullptr;
            napi_value jsonProp = nullptr;
            napi_value parseFunc = nullptr;
            napi_get_global(context->env, &global);
            napi_get_named_property(context->env, global, "JSON", &jsonProp);
            napi_get_named_property(context->env, jsonProp, "parse", &parseFunc);
            napi_value argv[1] = { nullptr };
            napi_create_string_utf8(context->env, context->elmentInfo.dump().c_str(), NAPI_AUTO_LENGTH, argv);
            napi_call_function(context->env, jsonProp, parseFunc, 1, argv, argv);
            napi_value jsCallback = nullptr;
            napi_get_reference_value(context->env, context->callbackRef, &jsCallback);
            napi_call_function(context->env, nullptr, jsCallback, 1, argv, argv);
            // accessing callback from js-caller, do check and warn
            auto hasError = false;
            napi_is_exception_pending(context->env, &hasError);
            if (hasError) {
                LOG_W("Exception raised during call js_cb_function!");
            }
            if (context->releaseObserver) {
                LOG_D("Unref jsObserver: %{public}s", context->observerId.c_str());
                lock_guard<mutex> guard(g_jsRefsMtx);
                napi_delete_reference(context->env, context->observerRef);
                g_jsRefs.erase(context->observerId);
            }
            if (context->releaseCallback) {
                LOG_D("Unref jsCallback: %{public}s", context->callbackId.c_str());
                lock_guard<mutex> guard(g_jsRefsMtx);
                napi_delete_reference(context->env, context->callbackRef);
                g_jsRefs.erase(context->callbackId);
            }
            delete context;
        };
        if (napi_send_event(env, task, napi_eprio_immediate) != napi_status::napi_ok) {
            LOG_E("Failed to post event callback, callback=%{public}s", context->callbackId.c_str());
            delete context;
        }
    }
}
