/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "uitest_ffi.h"

#include <cstdlib>
#include <grp.h>
#include <pthread.h>
#include <pwd.h>
#include <sched.h>
#include <future>
#include <queue>
#include <set>
#include <string>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include "nlohmann/json.hpp"
#include "fcntl.h"
#include "common_utilities_hpp.h"
#include "frontend_api_defines.h"
#include "ipc_transactor.h"
#include "ui_event_observer_impl.h"
#include "test_server_client.h"

namespace OHOS::cjuitest {
    using namespace nlohmann;
    using namespace std;
    using namespace OHOS::uitest;

    static constexpr size_t BACKEND_OBJ_GC_BATCH = 100;
    /**For gc usage, records the backend objRefs about to delete. */
    static queue<string> g_backendObjsAboutToDelete;
    static mutex g_gcQueueMutex;
    /**IPC client. */
    static ApiTransactor g_apiTransactClient(false);
    static future<void> g_establishConnectionFuture;
    
    char *MallocCString(const string &origin)
    {
        if (origin.empty()) {
            return nullptr;
        }
        auto len = origin.length() + 1;
        char *res = static_cast<char *>(malloc(sizeof(char) * len));
        if (res == nullptr) {
            return nullptr;
        }
        return char_traits<char>::copy(res, origin.c_str(), len);
    }

    /**Wait connection result sync if need.*/
    static void WaitForConnectionIfNeed()
    {
        if (g_establishConnectionFuture.valid()) {
            LOG_I("Begin WaitForConnection");
            g_establishConnectionFuture.get();
        }
    }

    static void SetPasteBoardData(string_view text)
    {
        OHOS::testserver::TestServerClient::GetInstance().SetPasteData(string(text));
    }

    void PreprocessTransaction(ApiCallInfo &callInfo, ApiCallErr &error)
    {
        auto &paramList = callInfo.paramList_;
        const auto &id = callInfo.apiId_;
        if (id == "Component.inputText" && paramList.size() > 0) {
            if (paramList.at(INDEX_ZERO).type() == nlohmann::detail::value_t::string) {
                SetPasteBoardData(paramList.at(INDEX_ZERO).get<string>());
            }
        } else if (id  == "Driver.inputText" && paramList.size() > 1) {
            if (paramList.at(INDEX_ONE).type() == nlohmann::detail::value_t::string) {
                SetPasteBoardData(paramList.at(INDEX_ONE).get<string>());
            }
        } else if (id == "Driver.screenCap" || id == "UiDriver.screenCap" || id == "Driver.screenCapture") {
            if (paramList.size() < 1 || paramList.at(0).type() != nlohmann::detail::value_t::string) {
                LOG_E("Missing file path argument");
                error = ApiCallErr{ERR_INVALID_INPUT, "Missing file path argument"};
                return;
            }
            auto path = paramList.at(INDEX_ZERO).get<string>();
            auto fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
            if (fd == -1) {
                LOG_E("Invalid file path: %{public}s", path.data());
                error = ApiCallErr{ERR_INVALID_INPUT, "Invalid file path:" + path};
                return;
            }
            paramList[INDEX_ZERO] = fd;
            callInfo.fdParamIndex_ = INDEX_ZERO;
        } else if (id  == "UIEventObserver.once") {
            LOG_I("preprocess callback");
            int64_t callbackId = paramList.at(1).get<int64_t>();
            UiEventObserverImpl::Get().PreprocessCallOnce(callInfo, callbackId, error);
        }
    }

    /**Call api with parameters out, wait for and return result value or throw raised exception.*/
    ApiReplyInfo CJTransact(ApiCallInfo callInfo)
    {
        WaitForConnectionIfNeed();
        LOG_D("TargetApi=%{public}s", callInfo.apiId_.data());
        auto reply = ApiReplyInfo();
        g_apiTransactClient.Transact(callInfo, reply);
        LOG_I("return value: %{public}s", reply.resultValue_.dump().c_str());
        // notify backend objects deleting
        if (g_backendObjsAboutToDelete.size() >= BACKEND_OBJ_GC_BATCH) {
            auto gcCall = ApiCallInfo {.apiId_ = "BackendObjectsCleaner"};
            unique_lock<mutex> lock(g_gcQueueMutex);
            for (size_t count = 0; count < BACKEND_OBJ_GC_BATCH; count++) {
                gcCall.paramList_.emplace_back(g_backendObjsAboutToDelete.front());
                g_backendObjsAboutToDelete.pop();
            }
            lock.unlock();
            auto gcReply = ApiReplyInfo();
            g_apiTransactClient.Transact(gcCall, gcReply);
        }
        return reply;
    }

    int32_t GetUid()
    {
        auto processGetuid = static_cast<int32_t>(getuid());
        return processGetuid;
    }

    int32_t GetPid()
    {
        auto proPid = static_cast<int32_t>(getpid());
        return proPid;
    }

    int32_t GetTid()
    {
        auto proTid = static_cast<int32_t>(gettid());
        return proTid;
    }

    extern "C" {
        void CJ_InitConnection(char *token)
        {
            string realToken{token};
            g_establishConnectionFuture = async(launch::async, [realToken]() {
                auto &instance = UiEventObserverImpl::Get();
                using namespace std::placeholders;
                auto callbackHandler = std::bind(&UiEventObserverImpl::HandleEventCallback, &instance, _1, _2);
                auto result = g_apiTransactClient.InitAndConnectPeer(realToken, callbackHandler);
                LOG_I("End setup transaction connection, result=%{public}d", result);
            });
        }

        RetDataCString CJ_ApiCall(ApiCallParams params)
        {
            RetDataCString ret{.code = uitest::ErrCode::NO_ERROR, .data = nullptr};
            ApiCallInfo callInfo_;
            LOG_D("apiId: %{public}s", params.apiId);
            callInfo_.apiId_ = string{params.apiId};
            if (params.callerObjRef != nullptr) {
                LOG_D("callerObjRef_: %{public}s", params.callerObjRef);
                callInfo_.callerObjRef_ = string{params.callerObjRef};
            }
            if (params.paramList == nullptr) {
                LOG_D("paramList_: \"\"");
                callInfo_.paramList_ = "";
            } else {
                LOG_D("paramList_: %{public}s", params.paramList);
                callInfo_.paramList_ = nlohmann::json::parse(string{params.paramList});
            }
            ApiCallErr err{uitest::ErrCode::NO_ERROR};
            PreprocessTransaction(callInfo_, err);
            if (err.code_ != uitest::ErrCode::NO_ERROR) {
                ret.code = err.code_;
                ret.data = MallocCString(err.message_);
                return ret;
            }
            auto result = CJTransact(callInfo_);
            if (callInfo_.fdParamIndex_ >= 0) {
                auto fd = callInfo_.paramList_.at(INDEX_ZERO).get<int>();
                (void) close(fd);
            }
            if (result.exception_.code_ != uitest::ErrCode::NO_ERROR) {
                ret.code = result.exception_.code_;
                ret.data = MallocCString(result.exception_.message_);
                return ret;
            }
            ret.data = MallocCString(result.resultValue_.dump());
            return ret;
        }

        void CJ_UITestObjDelete(char *objref)
        {
            unique_lock<mutex> lock(g_gcQueueMutex);
            g_backendObjsAboutToDelete.push(string(objref));
        }

        int32_t FfiOHOSProcessManagerGetUid()
        {
            auto result = GetUid();
            return result;
        }

        int32_t FfiOHOSProcessManagerGetPid()
        {
            auto result = GetPid();
            return result;
        }

        int32_t FfiOHOSProcessManagerGetTid()
        {
            auto result = GetTid();
            return result;
        }
    }
} // namespace OHOS::uitest
