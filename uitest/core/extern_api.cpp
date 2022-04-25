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

#include <sstream>
#include "extern_api.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    /**Get the readable name of the error enum value.*/
    string GetErrorName(ErrCode code)
    {
        static const std::map<ErrCode, std::string> names = {
            {NO_ERROR,       "NO_ERROR"},
            {INTERNAL_ERROR, "INTERNAL_ERROR"},
            {WIDGET_LOST,    "WIDGET_LOST"},
            {USAGE_ERROR,    "USAGE_ERROR"}
        };
        const auto find = names.find(code);
        return (find == names.end()) ? "UNKNOWN" : find->second;
    }

    /**Constructor function to register all the ExternAPI creators and functions.*/
    static void __attribute__((constructor)) ApiRegistration()
    {
        RegisterExternApIs();
    }

    ExternApiServer &ExternApiServer::Get()
    {
        static ExternApiServer singleton;
        return singleton;
    }

    void ExternApiServer::AddHandler(ApiRequstHandler handler)
    {
        if (handler == nullptr) {
            return;
        }
        handlers_.emplace_back(handler);
    }

    void ExternApiServer::RemoveHandler(ApiRequstHandler handler)
    {
        auto finder = [handler](ApiRequstHandler hdl) {
            return hdl == handler;
        };
        handlers_.erase(remove_if(handlers_.begin(), handlers_.end(), finder), handlers_.end());
    }

    void ExternApiServer::Call(string_view apiId, json &caller, const json &in, json &out, ApiCallErr &err) const
    {
        string api(apiId);
        for (auto handler:handlers_) {
            try {
                if (handler(apiId, caller, in, out, err)) {
                    return;
                }
            } catch (exception &ex) {
                // catch possible json-parsing error
                err = ApiCallErr(INTERNAL_ERROR, "Exception raised when handling '" + api + "':" + ex.what());
                return;
            }
        }
        err = ApiCallErr(INTERNAL_ERROR, "No handler found for extern-api: " + api);
    }

    string ApiTransact(string_view funcStr, string_view callerStr, string_view paramsStr)
    {
        LOG_D("Begin to invoke api '%{public}s', caller=%{public}s, parameters=%{public}s",
              funcStr.data(), callerStr.data(), paramsStr.data());
        auto error = ApiCallErr(NO_ERROR);
        auto out = json::array();
        json returnData;
        try {
            auto caller = json::parse(callerStr);
            auto in = json::parse(paramsStr);
            ExternApiServer::Get().Call(funcStr, caller, in, out, error);
            returnData[KEY_UPDATED_CALLER] = caller;
            returnData[KEY_RESULT_VALUES] = out;
        } catch (exception &ex) {
            error = ApiCallErr(INTERNAL_ERROR, string("Convert transaction parameters failed: ") + ex.what());
        }

        if (error.code_ != NO_ERROR) {
            // deliver exception
            LOG_W("Transact on api '%{public}s' failed, caller='%{public}s', params='%{public}s', error='%{public}s'",
                  funcStr.data(), callerStr.data(), paramsStr.data(), error.message_.c_str());
            json exceptionInfo;
            exceptionInfo[KEY_CODE] = GetErrorName(error.code_);
            exceptionInfo[KEY_MESSAGE] = error.message_;
            returnData[KEY_EXCEPTION] = exceptionInfo;
        }
        return returnData.dump();
    }
}

