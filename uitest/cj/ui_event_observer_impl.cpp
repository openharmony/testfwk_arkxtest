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

#include <map>
#include <string>
#include <securec.h>
#include <functional>
#include "json.hpp"
#include "common_utilities_hpp.h"
#include "ui_event_observer_impl.h"
#include "cj_lambda.h"

namespace OHOS::uitest {

using namespace nlohmann;
using namespace std;
static map<string, shared_ptr<function<void(CUIElementInfo)>>> g_callbacks;
static size_t g_incCbId = 0;

UiEventObserverImpl &UiEventObserverImpl::Get()
{
    static UiEventObserverImpl instance;
    return instance;
}

void UiEventObserverImpl::PreprocessCallOnce(ApiCallInfo &call, int64_t callbackId, ApiCallErr &err)
{
    auto &paramList = call.paramList_;
    if (paramList.size() < 1 || paramList.at(0).type() != detail::value_t::string) {
        LOG_E("[UiEventObserverImpl]Missing event type argument");
        err = ApiCallErr(ERR_INVALID_INPUT, "Missing event type argument");
        return;
    }
    auto event = paramList.at(0).get<string>();
    if (callbackId <= 0) {
        LOG_E("[UiEventObserverImpl]Invalid callback function argument");
        err = ApiCallErr(ERR_INVALID_INPUT, "Invalid callback function argument");
        return;
    }
    auto func = reinterpret_cast<void (*)(CUIElementInfo)>(callbackId);
    auto sp = make_shared<function<void(CUIElementInfo)>>(CJLambda::Create(func));
    if (sp == nullptr) {
        err = ApiCallErr(INTERNAL_ERROR, "UIEventObserver memory error.");
        return;
    }
    auto key = string("cj_callback#") + to_string(++g_incCbId);
    LOG_I("CbId = %{public}s, CbRef = %{public}p", key.c_str(), sp.get());
    LOG_D("Hold reference of %{public}s", key.c_str());
    g_callbacks.insert({key, sp});
    paramList.at(1) = key;
}

struct EventCallbackContext {
    string observerId;
    string callbackId;
    weak_ptr<function<void(CUIElementInfo)>> callbackRef;
    bool releaseCallback; // if or not release callback function after performing this callback
    nlohmann::json elmentInfo;
};

static void InitCallbackContext(const ApiCallInfo &in, ApiReplyInfo &out, EventCallbackContext &ctx)
{
    LOG_I("[UiEventObserverImpl]Handler api callback: %{public}s", in.apiId_.c_str());
    if (in.apiId_ != "UIEventObserver.once") {
        out.exception_ = ApiCallErr(ERR_INTERNAL, "Api dose not support callback: " + in.apiId_);
        LOG_E("%{public}s", out.exception_.message_.c_str());
        return;
    }
    DCHECK(in.paramList_.size() > INDEX_ZERO && in.paramList_.at(INDEX_ZERO).type() == detail::value_t::object);
    DCHECK(in.paramList_.size() > INDEX_ONE && in.paramList_.at(INDEX_ONE).type() == detail::value_t::string);
    DCHECK(in.paramList_.size() > INDEX_TWO && in.paramList_.at(INDEX_TWO).type() == detail::value_t::boolean);
    DCHECK(in.paramList_.size() > INDEX_THREE && in.paramList_.at(INDEX_THREE).type() == detail::value_t::boolean);
    auto &observerId = in.callerObjRef_;
    auto &elementInfo = in.paramList_.at(INDEX_ZERO);
    auto callbackId = in.paramList_.at(INDEX_ONE).get<string>();
    LOG_I("[UiEventObserverImpl]Begin to callback UiEvent: observer=%{public}s, callback=%{public}s",
          observerId.c_str(), callbackId.c_str());
    auto findCallback = g_callbacks.find(callbackId);
    if (findCallback == g_callbacks.end()) {
        out.exception_ = ApiCallErr(INTERNAL_ERROR, "JsCallbackFunction is not referenced: " + callbackId);
        LOG_E("%{public}s", out.exception_.message_.c_str());
        return;
    }
    ctx.observerId = observerId;
    ctx.callbackId = callbackId;
    ctx.callbackRef = findCallback->second;
    ctx.elmentInfo = move(elementInfo);
    ctx.releaseCallback = in.paramList_.at(INDEX_THREE).get<bool>();
}

static void DestructElementInfo(CUIElementInfo &info)
{
    if (info.bundleName != nullptr) { free(info.bundleName); }
    if (info.componentType != nullptr) { free(info.componentType); }
    if (info.text != nullptr) { free(info.text); }
    return;
}

static CUIElementInfo CreateElementInfo(string str)
{
    int status = 0;
    CUIElementInfo result{};
    auto json = nlohmann::json::parse(str);
    string bundleName = json["bundleName"];
    string type = json["type"];
    string text = json["text"];
    result.bundleName = static_cast<char *>(malloc(bundleName.size() + 1));
    if (result.bundleName == nullptr) {
        LOG_E("[UiEventObserverImpl] memory error.");
        return result;
    }
    result.componentType = static_cast<char *>(malloc(type.size() + 1));
    if (result.componentType == nullptr) {
        LOG_E("[UiEventObserverImpl] memory error.");
        DestructElementInfo(result);
        return result;
    }
    result.text = static_cast<char *>(malloc(text.size() + 1));
    if (result.text == nullptr) {
        LOG_E("[UiEventObserverImpl] memory error.");
        DestructElementInfo(result);
        return result;
    }
    status |= memcpy_s(result.bundleName, bundleName.size() + 1,
        bundleName.c_str(), bundleName.size() + 1);
    status |= memcpy_s(result.componentType, type.size() + 1,
        type.c_str(), type.size() + 1);
    status |= memcpy_s(result.text, text.size() + 1,
        text.c_str(), text.size() + 1);
    if (status != 0) {
        LOG_E("[UiEventObserverImpl] memory error. %{public}d", status);
        DestructElementInfo(result);
        result.bundleName = nullptr;
        result.componentType = nullptr;
        result.text = nullptr;
    }
    return result;
}

void UiEventObserverImpl::HandleEventCallback(const ApiCallInfo &in, ApiReplyInfo &out)
{
    auto context = new (std::nothrow) EventCallbackContext();
    if (context == nullptr) {
        out.exception_ = ApiCallErr(INTERNAL_ERROR, "UIEventObserver memory error.");
        return;
    }
    InitCallbackContext(in, out, *context);
    if (out.exception_.code_ != NO_ERROR) {
        delete context;
        LOG_W("[UiEventObserverImpl]InitCallbackContext failed, cannot perform callback");
        return;
    }
    auto info = CreateElementInfo(context->elmentInfo.dump().c_str());
    if (info.bundleName == nullptr || info.componentType == nullptr || info.text == nullptr) {
        delete context;
        LOG_W("[UiEventObserverImpl]CreateUIElementInfo failed.");
        out.exception_ = ApiCallErr(INTERNAL_ERROR, "UIEventObserver memory error.");
        return;
    }
    auto callback = context->callbackRef.lock();
    callback->operator()(info);
    if (context->releaseCallback) {
        LOG_D("[UiEventObserverImpl]Unref callback: %{public}s", context->callbackId.c_str());
        g_callbacks.erase(context->callbackId);
    }
    DestructElementInfo(info);
    delete context;
}

}