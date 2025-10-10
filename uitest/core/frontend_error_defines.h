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

#ifndef FRONTEND_ERROR_DEFINES_H
#define FRONTEND_ERROR_DEFINES_H

namespace OHOS::uitest {
    enum ErrCode : uint32_t {
        /**Old ErrorCode*/
        NO_ERROR = 0,
        /**Internal error, not expected to happen.*/
        INTERNAL_ERROR = 1,
        /**Widget that is expected to be exist lost.*/
        WIDGET_LOST = 2,
        /**Window that is expected to be exist lost.*/
        WINDOW_LOST = 3,
        /**The user assertion failure.*/
        ASSERTION_FAILURE = 4,
        USAGE_ERROR = 5,
        /**New ErrorCode*/
        /**Initialize failed.*/
        ERR_INITIALIZE_FAILED = 17000001,
        /**API does not allow calling concurrently.*/
        ERR_API_USAGE = 17000002,
        /**Component existence assertion failed.*/
        ERR_ASSERTION_FAILED = 17000003,
        /**Component lost/UiWindow lost.*/
        ERR_COMPONENT_LOST = 17000004,
        /**This operation is not supported.*/
        ERR_OPERATION_UNSUPPORTED = 17000005,
        /**Internal error.*/
        ERR_INTERNAL = 17000006,
        /**Invalid input parameter.*/
        ERR_INVALID_INPUT = 401,
        /**The specified SystemCapability name was not found.*/
        ERR_NO_SYSTEM_CAPABILITY = 801,
        /**Invalid input parameter.*/
        ERR_INVALID_PARAM = 17000007,
    };

    const std::map<ErrCode, std::string> ErrDescMap = {
        /**Correspondence between error codes and descriptions*/
        {NO_ERROR, "No Error"},
        {ERR_INITIALIZE_FAILED, "Initialization failed."},
        {ERR_API_USAGE, "The async function is not called with await."},
        {ERR_ASSERTION_FAILED, "Assertion failed."},
        {ERR_COMPONENT_LOST, "The window or component is invisible or destroyed."},
        {ERR_OPERATION_UNSUPPORTED, "This operation is not supported."},
        {ERR_INTERNAL, "Internal error."},
        {ERR_NO_SYSTEM_CAPABILITY, "The specified SystemCapability name was not found."},
        {ERR_INVALID_INPUT, "Invalid input parameter."},
    };

    /**API invocation error detail wrapper.*/
    struct ApiCallErr {
    public:
        ErrCode code_;
        std::string message_ = "";

        ApiCallErr() = delete;

        explicit ApiCallErr(ErrCode ec)
        {
            code_ = ec;
            message_ = ErrDescMap.find(ec)->second;
        }

        ApiCallErr(ErrCode ec, std::string_view msg)
        {
            code_ = ec;
            message_ = std::string(msg);
        }
    };
} // namespace OHOS::uitest

#endif