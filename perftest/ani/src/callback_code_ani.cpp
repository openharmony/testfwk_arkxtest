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
#include <ani_signature_builder.h>
#include <sstream>
#include <iostream>
#include <thread>
#include "nlohmann/json.hpp"
#include "common_utils.h"
#include "callback_code_ani.h"
#include "hilog/log.h"
using namespace OHOS::HiviewDFX;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LogType::LOG_CORE, 0xD003120, "PerfTest_ANI"};

namespace OHOS::perftest {
    using namespace nlohmann;
    using namespace std;
    using namespace arkts::ani_signature;

    void ThreadLock::LockRelease(string errorMessage)
    {
        errMsg = errorMessage;
        ready = true;
        condition.notify_all();
    }

    static map<string, ani_ref> g_jsRefs;
    static size_t g_incJsCbId = 0;

    CallbackCodeAni &CallbackCodeAni::Get()
    {
        static CallbackCodeAni instance;
        return instance;
    }

    string CallbackCodeAni::PreprocessCallback(ani_env *env, ani_ref callbackRef, ApiReplyInfo &out)
    {
        DCHECK(env != nullptr);
        const auto jsCbId = string("js_callback#") + to_string(++g_incJsCbId);
        ani_status res;
        const char *className = "@ohos.test.PerfTest.FinishCallback";
        if ((res = env->FindClass(className, &finishCallbackClass_)) != ANI_OK) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "PreprocessCallback failed");
            HiLog::Error(LABEL, "Can not find className: %{public}s, %{public}d", className, res);
            return "";
        }
        if ((res = env->Class_FindMethod(finishCallbackClass_, "<ctor>", nullptr, &finishCallbackCtor_)) != ANI_OK) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "PreprocessCallback failed");
            HiLog::Error(LABEL, "Can not find ctor of class %{public}s", className);
            return "";
        }
        ani_ref globalCallbackRef;
        if (env->GlobalReference_Create(callbackRef, &globalCallbackRef) != ANI_OK) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Create observerRef fail");
            HiLog::Error(LABEL, "Create globalCallbackRef failed");
            return "";
        }
        HiLog::Info(LABEL, "CbId = %{public}s, CbRef = %{public}p", jsCbId.c_str(), globalCallbackRef);
        g_jsRefs.insert({ jsCbId, globalCallbackRef });
        return jsCbId;
    }

    void CallbackCodeAni::InitCallbackContext(ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out,
                                              shared_ptr<CodeCallbackContext> ctx)
    {
        HiLog::Info(LABEL, "Handler api callback: %{public}s", in.apiId_.c_str());
        DCHECK(env != nullptr);
        DCHECK(in.paramList_.size() == TWO);
        DCHECK(in.paramList_.at(INDEX_ZERO).type() == detail::value_t::string);
        DCHECK(in.paramList_.at(INDEX_ONE).type() == detail::value_t::number_unsigned);
        auto callbackId = in.paramList_.at(INDEX_ZERO).get<string>();
        auto timeout = in.paramList_.at(INDEX_ONE).get<uint32_t>();
        HiLog::Info(LABEL, "Begin to callback function: callback=%{public}s", callbackId.c_str());
        auto findCallback = g_jsRefs.find(callbackId);
        if (findCallback == g_jsRefs.end()) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "JsCallbackFunction is not referenced: " + callbackId);
            HiLog::Error(LABEL, "%{public}s", out.exception_.message_.c_str());
            return;
        }
        ani_status res;
        auto className = Builder::BuildClass({"@ohos", "test", "PerfTest", "FinishCallback"}).Descriptor();
        ani_object classObj = {};
        if ((res = env->Object_New(finishCallbackClass_, finishCallbackCtor_, &classObj)) != ANI_OK) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "Execute callback %{public}d failed" + callbackId);
            HiLog::Error(LABEL, "New object of %{public}s failed, %{public}d", className.c_str(), res);
            return;
        }
        ani_ref cb;
        if ((res = env->Object_GetFieldByName_Ref(classObj, "finish", &cb)) != ANI_OK) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "Execute callback %{public}d failed" + callbackId);
            HiLog::Error(LABEL, "Can not find fieldName: finish, %{public}d", res);
            return;
        }
        ctx->env = env;
        ctx->callbackId = callbackId;
        ctx->callbackRef = findCallback->second;
        ctx->timeout = timeout;
        ctx->cb = cb;
    }

    void CallbackCodeAni::FinishCallback(bool res)
    {
        threadLock_->res = res;
        threadLock_->LockRelease("");
    }

    void CallbackCodeAni::WaitforCallbackFinish(ani_env *env, shared_ptr<CodeCallbackContext> context,
                                                shared_ptr<ThreadLock> threadLock, ApiReplyInfo &out)
    {
        unique_lock<mutex> lock(threadLock->mutex);
        const auto timeout = chrono::milliseconds(context->timeout);
        if (!threadLock->condition.wait_for(lock, timeout, [&threadLock] { return threadLock->ready; })) {
            HiLog::Error(LABEL, "Jscallback call timeout, has been waiting for %{public}d ms", context->timeout);
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "Code execution has been timeout.");
            return;
        }
        HiLog::Info(LABEL, "Jscallback call has finished, res = %{public}d", threadLock->res);
        if (!threadLock->errMsg.empty()) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, threadLock->errMsg);
            HiLog::Error(LABEL, "%{public}s", out.exception_.message_.c_str());
        } else if (!threadLock->res) {
            out.exception_ = ApiCallErr(ERR_CALLBACK_FAILED, "Callback execution return false");
        }
    }

    /**Handle api callback from server end.*/
    void CallbackCodeAni::ExecuteCallback(ani_vm *vm, ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        HiLog::Debug(LABEL, "%{public}s called", __func__);
        shared_ptr<CodeCallbackContext> context = make_shared<CodeCallbackContext>();
        InitCallbackContext(env, in, out, context);
        if (out.exception_.code_ != NO_ERROR) {
            HiLog::Warn(LABEL, "InitCallbackContext failed, cannot perform callback");
            return;
        }
        threadLock_ = make_shared<ThreadLock>();

        thread executethread = thread([this, context, vm] {
            ani_env *env = nullptr;
            ani_options aniArgs {0, nullptr};
            auto re = vm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &env);
            if (re != ANI_OK) {
                HiLog::Error(LABEL, "AttachCurrentThread fail, result: %{public}d", re);
                this->threadLock_->LockRelease("Execute js_cb_function failed");
                return;
            }

            ani_fn_object fnObject = reinterpret_cast<ani_fn_object>(context->callbackRef);
            std::vector<ani_ref> argvRef = { context->cb };
            ani_ref executeResult;
            ani_status res = env->FunctionalObject_Call(fnObject, argvRef.size(), argvRef.data(), &executeResult);
            if (res != ANI_OK) {
                env->DescribeError();
                HiLog::Error(LABEL, "FunctionalObject_Call failed, %{public}d", res);
                this->threadLock_->LockRelease("Exception raised during call js_cb_function");
            }
            vm->DetachCurrentThread();
        });
        executethread.detach();
        WaitforCallbackFinish(env, context, threadLock_, out);
    }

    void CallbackCodeAni::DestroyCallbacks(ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        HiLog::Debug(LABEL, "%{public}s called", __func__);
        DCHECK(in.paramList_.size() == ONE);
        DCHECK(in.paramList_.at(INDEX_ZERO).type() == detail::value_t::array);
        for (auto& callbackId : in.paramList_.at(INDEX_ZERO)) {
            auto findCallback = g_jsRefs.find(callbackId);
            if (findCallback == g_jsRefs.end()) {
                continue;
            }
            env->GlobalReference_Delete(findCallback->second);
            g_jsRefs.erase(callbackId);
        }
    }

    void CallbackCodeAni::HandleCallbackEvent(ani_vm *vm, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        HiLog::Debug(LABEL, "%{public}s start", __func__);
        ani_env *env;
        if (vm->GetEnv(ANI_VERSION_1, &env) != ANI_OK) {
            HiLog::Error(LABEL, "%{public}s PerfTest: Unsupported ANI_VERSION_1", __func__);
            return;
        }
        if (in.apiId_ == "PerfTest.run") {
            ExecuteCallback(vm, env, in, out);
        } else if (in.apiId_ == "PerfTest.destroy") {
            DestroyCallbacks(env, in, out);
        } else {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Api dose not support callback: " + in.apiId_);
            HiLog::Error(LABEL, "%{public}s", out.exception_.message_.c_str());
        }
        HiLog::Debug(LABEL, "%{public}s end", __func__);
    }
}
