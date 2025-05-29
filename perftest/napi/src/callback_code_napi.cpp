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

#include <set>
#include <string>
#include <unistd.h>
#include <uv.h>
#include "nlohmann/json.hpp"
#include "common_utils.h"
#include "callback_code_napi.h"

namespace OHOS::perftest {
    using namespace nlohmann;
    using namespace std;
    
    static constexpr size_t NAPI_MAX_BUF_LEN = 1024;

    
    void ThreadLock::LockRelease(string errorMessage)
    {
        errMsg = errorMessage;
        ready = true;
        condition.notify_all();
    }

    /** Cached reference of callbackId and JsCallbackFunction, key is the unique id.*/
    static map<string, napi_ref> g_jsRefs;
    static size_t g_incJsCbId = 0;

    CallbackCodeNapi &CallbackCodeNapi::Get()
    {
        static CallbackCodeNapi instance;
        return instance;
    }

    static void GetFunctionFromParam(napi_env env, napi_value jsParam, json& param, string funcName)
    {
        if (param.contains(funcName)) {
            LOG_D("%{public}s has been defined", funcName.c_str());
            return;
        }
        bool funcExisted;
        NAPI_CALL_RETURN_VOID(env, napi_has_named_property(env, jsParam, funcName.c_str(), &funcExisted));
        if (funcExisted) {
            napi_value funcValue = nullptr;
            NAPI_CALL_RETURN_VOID(env, napi_get_named_property(env, jsParam, funcName.c_str(), &funcValue));
            napi_valuetype valueType = napi_undefined;
            NAPI_CALL_RETURN_VOID(env, napi_typeof(env, funcValue, &valueType));
            if (valueType != napi_function) {
                LOG_E("GetFunctionFromParam failed, invalid callback function argument");
                return;
            }
            const auto jsCbId = string("js_callback#") + to_string(++g_incJsCbId);
            // hold the const to avoid it be recycled, it's needed in performing callback
            napi_ref ref = nullptr;
            NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, funcValue, 1, &ref));
            LOG_I("CbId = %{public}s, CbRef = %{public}p", jsCbId.c_str(), ref);
            g_jsRefs.insert({ jsCbId, ref }); // store jsCbId instread of the function body
            param[funcName] = jsCbId;
        }
    }

    void CallbackCodeNapi::PreprocessCallback(napi_env env, ApiCallInfo &call, napi_value jsThis,
                                              napi_value jsParam, ApiCallErr &err)
    {
        DCHECK(env != nullptr);
        DCHECK(jsThis != nullptr);
        napi_valuetype paramType = napi_undefined;
        NAPI_CALL_RETURN_VOID(env, napi_typeof(env, jsParam, &paramType));
        if (paramType != napi_object) {
            err = ApiCallErr(ERR_INVALID_INPUT, "PreprocessCallback failed, Invalid type of param");
            return;
        }
        auto &paramList = call.paramList_;
        if (paramList.size() < 1 || paramList.at(0).type() != detail::value_t::object) {
            err = ApiCallErr(ERR_INVALID_INPUT, "Missing PerfTestStrategy argument");
            return;
        }
        GetFunctionFromParam(env, jsParam, paramList.at(0), ACTION_CODE_FUNC_NAME);
        GetFunctionFromParam(env, jsParam, paramList.at(0), RESET_CODE_FUNC_NAME);
    }

    void CallbackCodeNapi::InitCallbackContext(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out,
                                               shared_ptr<CodeCallbackContext> ctx)
    {
        LOG_I("Handler api callback: %{public}s", in.apiId_.c_str());
        DCHECK(env != nullptr);
        DCHECK(in.paramList_.size() == TWO);
        DCHECK(in.paramList_.at(INDEX_ZERO).type() == detail::value_t::string);
        DCHECK(in.paramList_.at(INDEX_ONE).type() == detail::value_t::number_unsigned);
        auto callbackId = in.paramList_.at(INDEX_ZERO).get<string>();
        auto timeout = in.paramList_.at(INDEX_ONE).get<uint32_t>();
        LOG_I("Begin to callback function: callback=%{public}s", callbackId.c_str());
        auto findCallback = g_jsRefs.find(callbackId);
        if (findCallback == g_jsRefs.end()) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "JsCallbackFunction is not referenced: " + callbackId);
            LOG_E("%{public}s", out.exception_.message_.c_str());
            return;
        }
        auto cb = [](napi_env env, napi_callback_info info) -> napi_value {
            LOG_D("ExecuteCallback jscallback return start");
            size_t argc_in = 1;
            napi_value argv_in[1] = {nullptr};
            void* param_in = nullptr;
            NAPI_CALL_BASE(env, napi_get_cb_info(env, info, &argc_in, argv_in, nullptr, &param_in), nullptr);
            ThreadLock* threadLock = reinterpret_cast<ThreadLock*>(param_in);
            if (threadLock == nullptr) {
                LOG_E("ExecuteCallback get threadLock failed");
                return nullptr;
            }
            NAPI_CALL_BASE(env, napi_get_value_bool(env, argv_in[0], &threadLock->res), nullptr);
            threadLock->LockRelease("");
            LOG_I("ExecuteCallback jscallback return end, res = %{public}d", threadLock->res);
            return nullptr;
        };
        ctx->env = env;
        ctx->callbackId = callbackId;
        ctx->callbackRef = findCallback->second;
        ctx->timeout = timeout;
        ctx->cb = cb;
    }

    void CallbackCodeNapi::BindPromiseCallbacks(napi_env env, napi_value promise, ThreadLock* threadLock)
    {
        bool is_promise = false;
        NAPI_CALL_RETURN_VOID(env, napi_is_promise(env, promise, &is_promise));
        if (!is_promise) {
            return;
        }
        napi_value catch_property;
        NAPI_CALL_RETURN_VOID(env, napi_get_named_property(env, promise, "catch", &catch_property));
        auto catch_handle = [](napi_env env, napi_callback_info info) -> napi_value {
            size_t argc_in = 1;
            napi_value argv_in[1] = {nullptr};
            void* param_in = nullptr;
            NAPI_CALL_BASE(env, napi_get_cb_info(env, info, &argc_in, argv_in, nullptr, &param_in), nullptr);
            napi_value error_str;
            napi_coerce_to_string(env, argv_in[0], &error_str);
            size_t buf_size = 0;
            char buf[NAPI_MAX_BUF_LEN] = {0};
            napi_get_value_string_utf8(env, error_str, buf, NAPI_MAX_BUF_LEN, &buf_size);
            ThreadLock* threadLock = reinterpret_cast<ThreadLock*>(param_in);
            if (threadLock == nullptr) {
                LOG_E("ExecuteCallback get threadLock failed");
                return nullptr;
            }
            threadLock->LockRelease("Exception raised during call js_cb_function: " + string(buf, buf_size));
            return nullptr;
        };
        napi_value catch_callback;
        NAPI_CALL_RETURN_VOID(env, napi_create_function(env, "CatchCallack", NAPI_AUTO_LENGTH, catch_handle,
                                                        threadLock, &catch_callback));
        NAPI_CALL_RETURN_VOID(env, napi_call_function(env, promise, catch_property, 1, &catch_callback, nullptr));
    }

    void CallbackCodeNapi::WaitforCallbackFinish(napi_env env, shared_ptr<CodeCallbackContext> context,
                                                 ThreadLock* threadLock, ApiReplyInfo &out)
    {
        unique_lock<mutex> lock(threadLock->mutex);
        const auto timeout = chrono::milliseconds(context->timeout);
        if (!threadLock->condition.wait_for(lock, timeout, [&threadLock] { return threadLock->ready; })) {
            LOG_E("Jscallback call timeout, has been waiting for %{public}d ms", context->timeout);
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "Code execution has been timeout.");
            return;
        }
        LOG_I("Jscallback call has finished, res = %{public}d", threadLock->res);
        if (!threadLock->errMsg.empty()) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, threadLock->errMsg);
            LOG_E("%{public}s", out.exception_.message_.c_str());
        }
        delete threadLock;
        threadLock = nullptr;
    }

    /**Handle api callback from server end.*/
    void CallbackCodeNapi::ExecuteCallback(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        shared_ptr<CodeCallbackContext> context = make_shared<CodeCallbackContext>();
        InitCallbackContext(env, in, out, context);
        if (out.exception_.code_ != NO_ERROR) {
            LOG_W("InitCallbackContext failed, cannot perform callback");
            return;
        }
        ThreadLock* threadLock = new (nothrow) ThreadLock();
        auto task = [context, threadLock, this]() {
            napi_value argv[1] = { nullptr };
            napi_value jsCallback = nullptr;
            napi_get_reference_value(context->env, context->callbackRef, &jsCallback);
            napi_create_function(context->env, "Complete", NAPI_AUTO_LENGTH, context->cb, threadLock, &argv[0]);
            napi_value promise;
            napi_call_function(context->env, nullptr, jsCallback, 1, argv, &promise);
            // accessing callback from js-caller, do check and warn
            auto hasError = false;
            napi_is_exception_pending(context->env, &hasError);
            if (hasError) {
                napi_value exception;
                napi_get_and_clear_last_exception(context->env, &exception);
                napi_value error_str;
                napi_coerce_to_string(context->env, exception, &error_str);
                size_t buf_size = 0;
                char buf[NAPI_MAX_BUF_LEN] = {0};
                napi_get_value_string_utf8(context->env, error_str, buf, NAPI_MAX_BUF_LEN, &buf_size);
                if (threadLock == nullptr) {
                    return;
                }
                threadLock->LockRelease("Exception raised during call js_cb_function: " + string(buf, buf_size));
                return;
            }
            this->BindPromiseCallbacks(context->env, promise, threadLock);
        };
        if (napi_send_event(env, task, napi_eprio_immediate) != napi_status::napi_ok) {
            LOG_E("Exception raised during call js_cb_function!");
        }
        WaitforCallbackFinish(env, context, threadLock, out);
    }

    void CallbackCodeNapi::DestroyCallbacks(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        DCHECK(in.paramList_.size() == ONE);
        DCHECK(in.paramList_.at(INDEX_ZERO).type() == detail::value_t::array);
        for (auto& callbackId : in.paramList_.at(INDEX_ZERO)) {
            auto findCallback = g_jsRefs.find(callbackId);
            if (findCallback == g_jsRefs.end()) {
                continue;
            }
            napi_delete_reference(env, findCallback->second);
            g_jsRefs.erase(callbackId);
        }
    }

    void CallbackCodeNapi::HandleCallbackEvent(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        if (in.apiId_ == "PerfTest.run") {
            ExecuteCallback(env, in, out);
        } else if (in.apiId_ == "PerfTest.destroy") {
            DestroyCallbacks(env, in, out);
        } else {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Api dose not support callback: " + in.apiId_);
            LOG_E("%{public}s", out.exception_.message_.c_str());
        }
    }
}
