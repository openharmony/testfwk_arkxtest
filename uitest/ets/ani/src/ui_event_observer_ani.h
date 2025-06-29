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

#ifndef UI_EVENT_OBSERVER_ANI_H
#define UI_EVENT_OBSERVER_ANI_H

#include "ani.h"
#include "frontend_api_defines.h"

namespace OHOS::uitest {
    class UiEventObserverAni {
    public:
        void PreprocessCallOnce(ani_env *env, ApiCallInfo &call, ani_object &observerObj, ani_object &callbackObj,
            ApiReplyInfo &out);
        void HandleEventCallback(ani_vm *vm, const ApiCallInfo &in, ApiReplyInfo &out);
        static UiEventObserverAni &Get();

    private:
        UiEventObserverAni() = default;
    };
}

#endif