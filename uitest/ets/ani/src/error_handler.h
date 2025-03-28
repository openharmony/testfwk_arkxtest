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

    constexpr const char* BUSINESS_ERROR_CLASS = "Luitest_ani/BusinessError;"
    class ErrorHandler {
    public:
        static ani_status Throw(ani_env *env, int32_t code, const string &errMsg)
        {
            return Throw(env, BUSINESS_ERROR_CLASS, code, errMsg);
        }
    }
    private:
        static ani_status Throw(ani_env *env, const char *className, int32_t code, const string &errMsg)
        {
            if (env == nullptr) {
                LOG_E("Invalid env");
                return ANI_INVALID_ARGS;
            }
            if (ANI_OK != env->FindClass(className, &cls)) {
                LOG_E("Not found class BusinessError");
                return ANI_ERROR;
            }
            ani_method method;
            if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", ":V", &method)) {
                LOG_E("Not found method of BusinessError");
                return ANI_ERROR;
            }
            ani_object obj;
            if (ANI_OK != env->Object_New(cls, method, &obj)) {
                LOG_E("Object_New BusinessError fail");
                return ANI_ERROR;
            }
            if (env->Object_SetFieldByName_Int(obj, "code", code) != ANI_OK) {
                LOG_E("Object_SetFieldByName_Int code fail");
                return ANI_ERROR;
            }
            ani_string msg;
            env->String_NewUTF8(errMsg.c_str(), errMsg.size(), &msg);
            if (env->Object_SetFieldByName_Ref(obj, "message", msg) != ANI_OK) {
                LOG_E("Object_SetFieldByName_Ref message fail");
                return ANI_ERROR;
            }
            return env->ThrowError(static_cast<ani_error>(obj));
        }
}

#endif