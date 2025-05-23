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

#ifndef FRONTEND_API_HANDLER_H
#define FRONTEND_API_HANDLER_H

#include <memory>
#include <map>
#include <string>
#include <set>
#include <functional>
#include <list>
#include "common_utils.h"
#include "frontend_api_defines.h"
#include "nlohmann/json.hpp"

namespace OHOS::perftest {
    class BackendClass {
    public:
        virtual const FrontEndClassDef& GetFrontendClassDef() const = 0;
        virtual ~BackendClass() = default;
    };

    /**Prototype of function that handles ExternAPI invocation request.*/
    using ApiInvokeHandler = std::function<void(const ApiCallInfo& in, ApiReplyInfo& out)>;

    /**Server that accepts and handles api invocation request.*/
    class FrontendApiServer {
    public:
        /**
         * Register api invocation handler.
         **/
        void AddHandler(std::string_view apiId, ApiInvokeHandler handler);

        /**
         * Check if any handler is registered for the target api.
         */
        bool HasHandlerFor(std::string_view apiId) const;

        /**
         * Remove api invocation  handler.
         **/
        void RemoveHandler(std::string_view apiId);

        /**
         * Add command pre-processor.
         **/
        void AddCommonPreprocessor(std::string_view name, ApiInvokeHandler processor);

         /**
         * Remove command pre-processor.
         **/
        void RemoveCommonPreprocessor(std::string_view name);

        /**
         * Handle api invocation request.
         *
         * */
        void Call(const ApiCallInfo& in, ApiReplyInfo& out) const;

        /**
         * Set handler to handle api callback from server.
         *
         * */
        void SetCallbackHandler(ApiInvokeHandler handler);

        /**
         * Handle api callback.
         *
         * */
        void Callback(const ApiCallInfo& in, ApiReplyInfo& out) const;

        /**
         * Get the singleton instance.
         * */
        static FrontendApiServer &Get();

        ~FrontendApiServer() {}

    private:
        /** constructor, initialize apiMap_ and apiMapReverse_*/
        FrontendApiServer() {};
        /** Command apiCall pre-processors before it's dispatched to target handler.*/
        std::map<std::string, ApiInvokeHandler> commonPreprocessors_;
        /** Registered api handlers.*/
        std::map<std::string, ApiInvokeHandler> handlers_;
        // function used for callback
        ApiInvokeHandler callbackHandler_ = nullptr;
    };
} // namespace OHOS::perftest

#endif