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

#ifndef CALLBACK_CODE_NAPI_H
#define CALLBACK_CODE_NAPI_H

#include <napi/native_api.h>
#include <napi/native_node_api.h>
#include "frontend_api_defines.h"
#include "api_caller_client.h"

namespace OHOS::perftest {
    // Define a lock structure to synchronously wait for the callback function to complete execution
    class ThreadLock {
    public:
        ThreadLock() = default;
        ~ThreadLock() = default;
        void LockRelease(string errorMessage);
    public:
        mutex mutex;
        condition_variable condition;
        bool ready = false;
        bool res = false;
        string errMsg;
    };

    struct CodeCallbackContext {
        napi_env env;
        string callbackId;
        napi_ref callbackRef;
        uint32_t timeout;
        napi_callback cb;
    };

    class CallbackCodeNapi {
    public:
        void PreprocessCallback(napi_env env, ApiCallInfo &call, napi_value jsThis,
                                napi_value jsParam, ApiCallErr &err);
        void InitCallbackContext(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out,
                                 shared_ptr<CodeCallbackContext> ctx);
        void BindPromiseCallbacks(napi_env env, napi_value promise, ThreadLock* threadLock);
        void WaitforCallbackFinish(napi_env env, shared_ptr<CodeCallbackContext> context,
                                   ThreadLock* threadLock, ApiReplyInfo &out);
        void ExecuteCallback(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out);
        void DestroyCallbacks(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out);
        void HandleCallbackEvent(napi_env env, const ApiCallInfo &in, ApiReplyInfo &out);
        static CallbackCodeNapi &Get();

    private:
        CallbackCodeNapi() = default;
        const string ACTION_CODE_FUNC_NAME = "actionCode";
        const string RESET_CODE_FUNC_NAME = "resetCode";
    };
} // namespace OHOS::perftest

#endif