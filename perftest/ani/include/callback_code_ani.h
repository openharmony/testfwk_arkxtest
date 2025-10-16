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

#ifndef CALLBACK_CODE_ANI_H
#define CALLBACK_CODE_ANI_H

#include "ani.h"
#include "frontend_api_defines.h"

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
        ani_env *env;
        string callbackId;
        ani_ref callbackRef;
        uint32_t timeout;
        ani_ref cb;
    };

    class CallbackCodeAni {
    public:
        string PreprocessCallback(ani_env *env, ani_ref callbackRef, ApiReplyInfo &out);
        void InitCallbackContext(ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out,
                                 shared_ptr<CodeCallbackContext> ctx);
        void FinishCallback(bool res);
        void WaitforCallbackFinish(ani_env *env, shared_ptr<CodeCallbackContext> context,
                                   shared_ptr<ThreadLock> threadLock, ApiReplyInfo &out);
        void ExecuteCallback(ani_vm *vm, ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out);
        void DestroyCallbacks(ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out);
        void HandleCallbackEvent(ani_vm *vm, const ApiCallInfo &in, ApiReplyInfo &out);
        static CallbackCodeAni &Get();
        
    private:
        CallbackCodeAni() = default;
        ani_class finishCallbackClass_;
        ani_method finishCallbackCtor_;
        shared_ptr<ThreadLock> threadLock_;
    };
}

#endif