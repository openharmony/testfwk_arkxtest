/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "common_utilities_hpp.h"
#include "frontend_api_defines.h"
#include "json.hpp"

namespace OHOS::uitest {
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
         * Register api invocation  handler.
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
         * Get the singleton instance.
         * */
        static FrontendApiServer &Get();

        ~FrontendApiServer() {}

    private:
        /** constructor, initialize apiMap_ and apiMapReverse_*/
        FrontendApiServer();
        /** handle result value/error code of old api call*/
        void ApiMapPost(const std::string &oldApiName, ApiReplyInfo &out) const;
        /** convert old api call to new api call*/
        std::string ApiMapPre(ApiCallInfo &inModifier) const;
        /** Command apiCall pre-processors before it's dispatched to target handler.*/
        std::map<std::string, ApiInvokeHandler> commonPreprocessors_;
        /** Registered api handlers.*/
        std::map<std::string, ApiInvokeHandler> handlers_;
        /** mapping classes of old API to classes of new API*/
        std::map<std::string, std::string> old2NewApiMap_;
        /** mapping classes of new API to classes of old API*/
        std::map<std::string, std::string> new2OldApiMap_;
    };

    /** Function serving external api-invocation request.*/
    void ApiTransact(const ApiCallInfo& in, ApiReplyInfo& out);
}

#endif