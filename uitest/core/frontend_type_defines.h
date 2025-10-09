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

#ifndef FRONTEND_TYPE_DEFINES_H
#define FRONTEND_TYPE_DEFINES_H

#include "frontend_error_defines.h"

namespace OHOS::uitest {
    /**Structure wraps the api-call data.*/
    struct ApiCallInfo {
        std::string apiId_;
        std::string callerObjRef_;
        nlohmann::json paramList_ = nlohmann::json::array();
        int32_t fdParamIndex_ = -1; // support fd as param
        bool convertError_ = false;
    };

    /**Structure wraps the api-call reply.*/
    struct ApiReplyInfo {
        nlohmann::json resultValue_ = nullptr;
        ApiCallErr exception_ = ApiCallErr(NO_ERROR);
        bool convertError_ = false;
    };

    /** Specifications of a frontend enumerator value.*/
    struct FrontendEnumValueDef {
        std::string_view name_;
        std::string_view valueJson_;
    };

    /** Specifications of a frontend enumerator.*/
    struct FrontendEnumeratorDef {
        std::string_view name_;
        const FrontendEnumValueDef *values_;
        size_t valueCount_;
    };

    /** Specifications of a frontend json data property.*/
    struct FrontEndJsonPropDef {
        std::string_view name_;
        std::string_view type_;
        bool required_;
    };
    /** Specifications of a frontend json object.*/
    struct FrontEndJsonDef {
        std::string_view name_;
        const FrontEndJsonPropDef *props_;
        size_t propCount_;
    };

    /** Specifications of a frontend class method.*/
    struct FrontendMethodDef {
        std::string_view name_;
        std::string_view signature_;
        bool static_;
        bool fast_;
        bool convertError_ = false;
    };

    /** Specifications of a frontend class.*/
    struct FrontEndClassDef {
        std::string_view name_;
        const FrontendMethodDef *methods_;
        size_t methodCount_;
        bool bindUiDriver_;
    };
} // namespace OHOS::uitest

#endif