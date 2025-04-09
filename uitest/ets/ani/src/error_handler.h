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

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "ani.h"
#include <string>
#include <cstdint>

namespace OHOS::uitest {
    using namespace nlohmann;
    using namespace std;

    constexpr const char* BUSINESS_ERROR_CLASS = "L@ohos/base/BusinessError;";
    class ErrorHandler {
    public:
        static ani_status Throw(ani_env *env, int32_t code, const string &errMsg)
        {
            return Throw(env, BUSINESS_ERROR_CLASS, code, errMsg);
        }
    private:
        static ani_object WrapError(ani_env *env, const std::string &msg)
        {
            if (env == nullptr) {
                return nullptr;
            }
            ani_class cls = nullptr;
            ani_method method = nullptr;
            ani_object obj = nullptr;
            ani_string aniMsg = nullptr;
            if (env->String_NewUTF8(msg.c_str(), msg.size(), &aniMsg) != ANI_OK) {
                LOG_E("StringToAniStr failed");
                return nullptr;
            }
            ani_ref undefRef;
            env->GetUndefined(&undefRef);
            ani_status status = env->FindClass("Lescompat/Error;", &cls);
            if (status != ANI_OK) {
                LOG_E("FindClass : %{public}d", status);
                return nullptr;
            }
            status = env->Class_FindMethod(cls, "<ctor>", "Lstd/core/String;Lescompat/ErrorOptions;:V", &method);
            if (status != ANI_OK) {
                LOG_E("Class_FindMethod : %{public}d", status);
                return nullptr;
            }
            status = env->Object_New(cls, method, &obj, aniMsg, undefRef);
            if (status != ANI_OK) {
                LOG_E("Object_New : %{public}d", status);
                return nullptr;
            }
            return obj;

        }


        static ani_status Throw(ani_env *env, const char *className, int32_t code, const string &errMsg)
        {
            if (env == nullptr) {
                LOG_E("Invalid env");
                return ANI_INVALID_ARGS;
            }
            ani_class cls;
            if (ANI_OK != env->FindClass(className, &cls)) {
                LOG_E("Not found class BusinessError");
                return ANI_ERROR;
            }
            ani_method method;
            if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "DLescompat/Error;:V", &method)) {
                LOG_E("Not found method of BusinessError");
                return ANI_ERROR;
            }
            ani_object error = WrapError(env, errMsg);
            if (error == nullptr) {
                LOG_E("WrapError failed");
                return ANI_ERROR;
            }
            ani_object obj;
            ani_double dCode(code);
            if (env->Object_New(cls, method, &obj, dCode, error) != ANI_OK) {
                LOG_E("Object_New error fail");
                return ANI_ERROR;
            }
            return env->ThrowError(static_cast<ani_error>(obj));
        }
    };
}

#endif