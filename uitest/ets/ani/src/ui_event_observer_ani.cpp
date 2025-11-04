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

#include <set>
#include <string>
#include <unistd.h>
#include "nlohmann/json.hpp"
#include "common_utilities_hpp.h"
#include "ui_event_observer_ani.h"
#include "hilog/log.h"
#include "utils.h"

using namespace OHOS::HiviewDFX;
using namespace arkts::ani_signature;

namespace OHOS::uitest {
    using namespace nlohmann;
    using namespace std;
    
    /** Cached reference of UIEventObserver and JsCallbackFunction, key is the unique id.*/
    static map<string, ani_ref> g_jsRefs;
    static size_t g_incJsCbId = 0;

    UiEventObserverAni &UiEventObserverAni::Get()
    {
        static UiEventObserverAni instance;
        return instance;
    }

    void UiEventObserverAni::PreprocessCallOnce(ani_env *env, ApiCallInfo &call, ani_object &observerObj,
                                                ani_object &callbackObj, ApiReplyInfo &out)
    {
        DCHECK(env != nullptr);
        auto &paramList = call.paramList_;
        // pop js_cb_function and save at local side
        if (paramList.size() < 1 || paramList.at(0).type() != detail::value_t::string) {
            HiLog::Error(LABEL,"Missing event type argument");
            out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Missing event type argument");
            return;
        }
        auto event = paramList.at(0).get<string>();
        if (g_jsRefs.find(call.callerObjRef_) == g_jsRefs.end()) {
            // hold the jsObserver to avoid it be recycled, it's needed in performing callback
            ani_ref observerRef;
            if (env->GlobalReference_Create(static_cast<ani_ref>(observerObj), &observerRef) != ANI_OK) {
                HiLog::Error(LABEL,"Create observerRef fail");
                out.exception_ = ApiCallErr(ERR_INTERNAL, "Create observerRef fail");
                return;
            }
            HiLog::Debug(LABEL,"Hold reference of %{public}s, ref: %{public}p", call.callerObjRef_.c_str(), observerRef);
            g_jsRefs.insert({ call.callerObjRef_, observerRef });
        }
        const auto jsCbId = string("js_callback#") + to_string(++g_incJsCbId);
        // hold the const  to avoid it be recycled, it's needed in performing callback
        ani_ref callbackRef;
        if (env->GlobalReference_Create(static_cast<ani_ref>(callbackObj), &callbackRef) != ANI_OK) {
            HiLog::Error(LABEL,"Create callbackRef fail");
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Create callbackRef fail");
            return;
        }
        HiLog::Info(LABEL,"CbId = %{public}s, CbRef = %{public}p", jsCbId.c_str(), callbackRef);
        g_jsRefs.insert({ jsCbId, callbackRef });
        // pass jsCbId instread of the function body
        call.paramList_.push_back(jsCbId); // observer.once(type, cllbackId)
    }

    struct EventCallbackContext {
        ani_env *env;
        string observerId;
        string callbackId;
        ani_ref observerRef;
        ani_ref callbackRef;
        bool releaseObserver; // if or not release observer object after performing this callback
        bool releaseCallback; // if or not release callback function after performing this callback
        nlohmann::json elmentInfo;
    };

    static void InitCallbackContext(ani_env *env, const ApiCallInfo &in, ApiReplyInfo &out, EventCallbackContext &ctx)
    {
        HiLog::Info(LABEL,"Handler api callback: %{public}s", in.apiId_.c_str());
        if (in.apiId_ != "UIEventObserver.once") {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Api dose not support callback: " + in.apiId_);
            HiLog::Error(LABEL,"%{public}s", out.exception_.message_.c_str());
            return;
        }
        DCHECK(env != nullptr);
        DCHECK(in.paramList_.size() > INDEX_ZERO && in.paramList_.at(INDEX_ZERO).type() == detail::value_t::object);
        DCHECK(in.paramList_.size() > INDEX_ONE && in.paramList_.at(INDEX_ONE).type() == detail::value_t::string);
        DCHECK(in.paramList_.size() > INDEX_TWO && in.paramList_.at(INDEX_TWO).type() == detail::value_t::boolean);
        DCHECK(in.paramList_.size() > INDEX_THREE && in.paramList_.at(INDEX_THREE).type() == detail::value_t::boolean);
        auto &observerId = in.callerObjRef_;
        auto &elementInfo = in.paramList_.at(INDEX_ZERO);
        auto callbackId = in.paramList_.at(INDEX_ONE).get<string>();
        HiLog::Info(LABEL,"Begin to callback UiEvent: observer=%{public}s, callback=%{public}s",
              observerId.c_str(), callbackId.c_str());
        auto findObserver = g_jsRefs.find(observerId);
        auto findCallback = g_jsRefs.find(callbackId);
        if (findObserver == g_jsRefs.end()) {
            out.exception_ = ApiCallErr(INTERNAL_ERROR, "UIEventObserver is not referenced: " + observerId);
            HiLog::Error(LABEL,"%{public}s", out.exception_.message_.c_str());
            return;
        }
        if (findCallback == g_jsRefs.end()) {
            out.exception_ = ApiCallErr(INTERNAL_ERROR, "JsCallbackFunction is not referenced: " + callbackId);
            HiLog::Error(LABEL,"%{public}s", out.exception_.message_.c_str());
            return;
        }
        ctx.env = env;
        ctx.observerId = observerId;
        ctx.callbackId = callbackId;
        ctx.observerRef = findObserver->second;
        ctx.callbackRef = findCallback->second;
        ctx.elmentInfo = move(elementInfo);
        ctx.releaseObserver = in.paramList_.at(INDEX_TWO).get<bool>();
        ctx.releaseCallback = in.paramList_.at(INDEX_THREE).get<bool>();
    }

    void SetEnum(ani_vm *vm, ani_env *env, ani_enum enumType, ani_object &obj, char *cstr, uint8_t index) {
        ani_enum_item enumItem;
        env->Enum_GetEnumItemByIndex(enumType, index, &enumItem);
        if (ANI_OK != env->Object_SetPropertyByName_Ref(obj, cstr, reinterpret_cast<ani_ref>(enumItem))) {
            HiLog::Error(LABEL,"SetProperty fail: %{public}s", cstr);
            vm->DetachCurrentThread();
            return;
        }
    }
    /**Handle api callback from server end.*/
    void UiEventObserverAni::HandleEventCallback(ani_vm *vm, const ApiCallInfo &in, ApiReplyInfo &out)
    {
        ani_env *env = nullptr;
        ani_options aniArgs {0, nullptr};
        auto re = vm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &env);
        if (re != ANI_OK) {
            HiLog::Error(LABEL,"AttachCurrentThread fail, result: %{public}d", re);
            out.exception_ = ApiCallErr(ERR_INTERNAL, "AttachCurrentThread fail");
            return;
        }
        auto context = new EventCallbackContext();
        InitCallbackContext(env, in, out, *context);
        if (out.exception_.code_ != NO_ERROR) {
            delete context;
            HiLog::Warn(LABEL,"InitCallbackContext failed, cannot perform callback");
            vm->DetachCurrentThread();
            return;
        }
        static const char *className = "@ohos.UiTest.UIElementInfoInner";
        ani_class cls;
        if (ANI_OK != env->FindClass(className, &cls)) {
            HiLog::Error(LABEL,"Not found class UIElementInfoInner");
            out.exception_ = ApiCallErr(ERR_INTERNAL, "FindClass UIElementInfoInner fail");
            vm->DetachCurrentThread();
            return;
        }
        ani_method method;
        if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", ":", &method)) {
            HiLog::Error(LABEL,"Not found method of UIElementInfoInner");
            vm->DetachCurrentThread();
            return;
        }
        ani_object obj;
        if (ANI_OK != env->Object_New(cls, method, &obj)) {
            HiLog::Error(LABEL,"Object_New UIElementInfoInner fail");
            vm->DetachCurrentThread();
            return;
        }
        string list[] = {"bundleName", "type", "text", "windowChangeType", 
            "componentEventType", "windowId", "componentId", "componentRect"};
        for (int i = 0; i < EIGHT; i++) {
            char *cstr = new char[list[i].length() + 1];
            strcpy(cstr, list[i].c_str());
            if (i == THREE){
                ani_enum enumType;
                if (ANI_OK != env->FindEnum(Builder::BuildEnum({"@ohos", "UiTest", "WindowChangeType"}).Descriptor().c_str(), &enumType)) {
                    HiLog::Error(LABEL, "Not found enum item: %{public}s", __func__);
                }
                uint8_t index = static_cast<uint8_t>(context->elmentInfo[list[i]].get<int>());
                SetEnum(vm, env, enumType, obj, cstr, index);
            } else if (i == FOUR){
                ani_enum enumType;
                if (ANI_OK != env->FindEnum(Builder::BuildEnum({"@ohos", "UiTest", "ComponentEventType"}).Descriptor().c_str(), &enumType)) {
                    HiLog::Error(LABEL, "Not found enum item: %{public}s", __func__);
                }
                ani_enum_item enumItem;
                auto index = static_cast<uint8_t>(context->elmentInfo[list[i]].get<int>());
                env->Enum_GetEnumItemByIndex(enumType, index, &enumItem);
                HiLog::Info(LABEL, " getComponentEventType:  %{public}d ", index);
                if (ANI_OK != env->Object_SetPropertyByName_Ref(obj, cstr, reinterpret_cast<ani_ref>(enumItem))) {
                    HiLog::Error(LABEL,"SetProperty fail: %{public}d", i);
                    vm->DetachCurrentThread();
                    break;
                }
            } else if (i == FIVE){
                auto windowId = static_cast<uint8_t>(context->elmentInfo[list[i]].get<int>());
                HiLog::Info(LABEL, " getWindowId:  %{public}d ", windowId);
                auto status = env->Object_SetPropertyByName_Ref(obj, cstr, reinterpret_cast<ani_ref>(createInt(env, ani_int(windowId))));
                if (ANI_OK != status) {
                    HiLog::Error(LABEL,"SetProperty fail: %{public}d", i);
                    vm->DetachCurrentThread();
                    break;
                }
            } else if (i == SEVEN){
                ani_object componentRect = newRect(env, obj, context->elmentInfo[list[i]]);
                if (ANI_OK != env->Object_SetPropertyByName_Ref(obj, cstr, reinterpret_cast<ani_ref>(componentRect))) {
                    HiLog::Error(LABEL,"SetProperty fail: %{public}d", i);
                    vm->DetachCurrentThread();
                    break;
                }
            } else {
                const auto s = context->elmentInfo[list[i]].get<string>();
                ani_string ani_str;
                auto ret = env->String_NewUTF8(s.c_str(), s.size(), &ani_str);
                HiLog::Info(LABEL, " elmentInfo:  %{public}s ", list[i].c_str());
                if (ret != ANI_OK) {
                    HiLog::Error(LABEL,"Analysis uielementInfo fail");
                    out.exception_ = ApiCallErr(ERR_INTERNAL, "Analysis uielementInfo fail");
                    vm->DetachCurrentThread();
                    break;
                }
                auto status = env->Object_SetPropertyByName_Ref(obj, cstr, reinterpret_cast<ani_ref>(ani_str));
                if (ANI_OK != status) {
                    HiLog::Error(LABEL,"SetProperty fail: %{public}d", i);
                    vm->DetachCurrentThread();
                    break;
                }
            }
        }
        ani_ref argvRef = static_cast<ani_ref>(obj);
        std::vector<ani_ref> tmp = {argvRef};
        ani_ref result;
        if (ANI_OK != env->FunctionalObject_Call(reinterpret_cast<ani_fn_object>(context->callbackRef), tmp.size(),
            tmp.data(), &result)) {
            HiLog::Error(LABEL,"HandleEventCallback fail");
            vm->DetachCurrentThread();
            return;
        }
        if (context->releaseObserver) {
            HiLog::Debug(LABEL,"Unref jsObserver: %{public}s", context->observerId.c_str());
            env->GlobalReference_Delete(context->observerRef);
            g_jsRefs.erase(context->observerId);
        }
        if (context->releaseCallback) {
            HiLog::Debug(LABEL,"Unref jsCallback: %{public}s", context->callbackId.c_str());
            env->GlobalReference_Delete(context->callbackRef);
            g_jsRefs.erase(context->callbackId);
        }
        delete context;
        vm->DetachCurrentThread();
    }
}
