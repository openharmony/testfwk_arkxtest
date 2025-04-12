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

#include "ani.h"
#include "hilog/log.h"
#include <array>
#include <iostream>
#include <unistd.h>
#include <future>
#include <queue>
#include <set>
#include <string>
#include "nlohmann/json.hpp"
#include "fcntl.h"
#include "common_utilities_hpp.h"
#include "frontend_api_handler.h"
#include "ipc_transactor.h"
#include "test_server_client.h"
#include "error_handler.h"
#include "ui_event_observer_ani.h"
#include <cstring>
#include <cstdio>

using namespace OHOS::uitest;
using namespace nlohmann;
using namespace std;
static ApiTransactor g_apiTransactClient(false);
static future<void> g_establishConnectionFuture;
using namespace OHOS::HiviewDFX;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LogType::LOG_CORE, 0xD003100, "UiTestKit"};
 
template<typename T>
void compareAndReport(const T& param1, const T& param2, const std::string& errorMessage, const std::string& message) {
    if (param1 != param2) {
        HiLog::Error(LABEL, "compareAndReport: %{public}s", errorMessage.c_str());
        return;
    } else {
        HiLog::Info(LABEL, "compareAndReport: %{public}s", message.c_str());
        return;
    }
}
 
static void pushParam(ani_env *env, ani_object input, ApiCallInfo &callInfo_, bool isInt) {    
    ani_boolean ret;
    env->Reference_IsUndefined(reinterpret_cast<ani_ref>(input), &ret);
    if (ret==ANI_FALSE) {
        if (isInt) {
            ani_double param;
            env->Object_CallMethodByName_Double(input, "unboxed", nullptr, &param);
            callInfo_.paramList_.push_back(int(param));
        } else {
            ani_double param;
            env->Object_CallMethodByName_Double(input, "unboxed", ":D", &param);
            callInfo_.paramList_.push_back(param);
        }
    }
}

static string aniStringToStdString([[maybe_unused]] ani_env *env, ani_string string_object) {
    ani_size strSize;
    env->String_GetUTF8Size(string_object, &strSize);
    std::vector<char> buffer(strSize + 1);
    char* utf8_buffer = buffer.data();
    ani_size bytes_written = 0;
    env->String_GetUTF8(string_object, utf8_buffer, strSize + 1, &bytes_written);
    utf8_buffer[bytes_written]='\0';
    std::string s = std::string(utf8_buffer);
    std::cout<<s<<std::endl;
    return s;
} 

static ani_class findCls(ani_env *env, const char *className) {
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "Not found className: %{public}s", className);
    }
    return cls;
}

static ani_method findCtorMethod(ani_env *env, ani_class cls, const char *name) {
    ani_method ctor = nullptr;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", name, &ctor)) {
        HiLog::Error(LABEL, "Not found ctor: %{public}s", name);
    }
    return ctor;
}
 
static void waitForConnectionIfNeed() {
    if (g_establishConnectionFuture.valid()) {
        HiLog::Error(LABEL, "%{public}s. Begin Wait for Connection", __func__);
        g_establishConnectionFuture.get();
    }
}

static void Transact(ApiCallInfo callInfo_, ApiReplyInfo &reply_) {
    waitForConnectionIfNeed();
    g_apiTransactClient.Transact(callInfo_, reply_);
}
static ani_ref UnmarshalObject(ani_env *env, nlohmann::json resultValue_) {
    const auto resultType = resultValue_.type();
    ani_ref result = nullptr;
    if (resultType == nlohmann::detail::value_t::null) {
        return result;
    } else if (resultType!=nlohmann::detail::value_t::string) {
        ani_string str;
        env->String_NewUTF8(resultValue_.dump().c_str(), resultValue_.dump().size(), &str);
        result = reinterpret_cast<ani_ref>(str);
        return result;
    }
    const auto cppString = resultValue_.get<string>();
    string frontendTypeName;
    bool bindJsThis = false;
    for (const auto &classDef : FRONTEND_CLASS_DEFS) {
        const auto objRefFormat = string(classDef->name_) + "#";
        if (cppString.find(objRefFormat) == 0) {
            frontendTypeName = string(classDef->name_);
            bindJsThis = classDef->bindUiDriver_;
            break;
        }
    }
    ani_string str;
    env->String_NewUTF8(cppString.c_str(), cppString.length(), &str);
    result = reinterpret_cast<ani_ref>(str);
    ani_size size;
    env->String_GetUTF16Size(str, &size);
    return result;    
}
  
static ani_ref UnmarshalReply(ani_env *env, const ApiCallInfo callInfo_, const ApiReplyInfo &reply_) {
    if (callInfo_.fdParamIndex_ >= 0) {
        auto fd = callInfo_.paramList_.at(INDEX_ZERO).get<int>();
        (void) close(fd);
    }     
    HiLog::Info(LABEL, "%{public}s.Start to UnmarshalReply", __func__);
    const auto &message = reply_.exception_.message_;
    ErrCode code = reply_.exception_.code_;
    if (code == INTERNAL_ERROR || code == ERR_INTERNAL) {
        HiLog::Error(LABEL, "UItest : ERRORINFO: code='%{public}u', message='%{public}s'", code, message.c_str());
    } else if (reply_.exception_.code_ != NO_ERROR) {
        HiLog::Error(LABEL, "UItest : ERRORINFO: code='%{public}u', message='%{public}s'", code, message.c_str());
        ErrorHandler::Throw(env, code, message);
    }
    HiLog::Info(LABEL, "UITEST: Start to unmarshall return value:%{public}s", reply_.resultValue_.dump().c_str());
    const auto resultType = reply_.resultValue_.type();
    if (resultType == nlohmann::detail::value_t::null) {
        return {};
    } else if (resultType == nlohmann::detail::value_t::array) {
        ani_class arrayCls = nullptr;
        if (ANI_OK!=env->FindClass("Lescompat/Array;", &arrayCls)) {
            HiLog::Error(LABEL, "%{public}s FindClass Array Failed", __func__);
        }
        ani_ref undefinedRef = nullptr;
        if (ANI_OK != env->GetUndefined(&undefinedRef)) {
            HiLog::Error(LABEL, "%{public}s GetUndefined Failed", __func__);
        }
        ani_method arrayCtor = findCtorMethod(env, arrayCls, "I:V");
        ani_object arrayObj;
        ani_size com_size = reply_.resultValue_.size();
        if (ANI_OK != env->Object_New(arrayCls, arrayCtor, &arrayObj, com_size)) {
            HiLog::Error(LABEL, "%{public}s Object New Array Failed", __func__);
            return reinterpret_cast<ani_ref>(arrayObj);
        }
        for (ani_size index = 0; index < reply_.resultValue_.size(); index++) {
            ani_ref item = UnmarshalObject(env, reply_.resultValue_.at(index));
            if (ANI_OK != env->Object_CallMethodByName_Void(arrayObj, "$_set", "ILstd/core/Object;:V", index, item)) {
                HiLog::Error(LABEL, "%{public}s Object_CallMethodByName_Void set Failed", __func__);
                break;
            }
        }
        return reinterpret_cast<ani_ref>(arrayObj);
    } else {
        return UnmarshalObject(env, reply_.resultValue_);
    }
}
  
static ani_boolean ScheduleEstablishConnection(ani_env *env, ani_string connToken) {
    auto token = aniStringToStdString(env, connToken);
    ani_vm *vm = nullptr;
    auto vmWorkable = env->GetVM(&vm);
    if (vmWorkable != env->GetVM(&vm)) {
        HiLog::Error(LABEL, "%{public}s GetVM failed", __func__);
    }
    bool result = false;
    g_establishConnectionFuture = async(launch::async, [vm, token, &result]() {
        using namespace std::placeholders;
        auto &instance = UiEventObserverAni::Get();
        auto callbackHandler = std::bind(&UiEventObserverAni::HandleEventCallback, &instance, vm, _1, _2);
        result = g_apiTransactClient.InitAndConnectPeer(token, callbackHandler);
        HiLog::Error(LABEL, "End setup transaction connection, result=%{public}d", result);
    });
    return result;
}

static ani_double GetConnectionStat(ani_env *env) {
    return g_apiTransactClient.GetConnectionStat();
}

static ani_string unwrapp(ani_env *env, ani_object object, const char *name) {
    ani_ref it;
    if (ANI_OK!= env->Object_GetFieldByName_Ref(object, name, &it)) {
        return nullptr;
    }
    return reinterpret_cast<ani_string>(it);
}

static json getPoint(ani_env *env, ani_object p) {
    auto point = json();
    static const char *className = "L@ohos/UiTest/PointInner;";
    ani_class cls = findCls(env, className);
    ani_method xGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>x", nullptr, &xGetter)) {
        HiLog::Error(LABEL, "%{public}s Find Method <get>x failed", __func__);
    }
    ani_double x;
    if (ANI_OK != env->Object_CallMethod_Double(p, xGetter, &x)) {
        HiLog::Error(LABEL, "%{public}s call xgetter failed", __func__);
        return point;
    }
    point["x"] = int(x);
    ani_method yGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>y", nullptr, &yGetter)) {
        HiLog::Error(LABEL, "%{public}s Find Method <get>y failed", __func__);
    }
    ani_double y;
    if (ANI_OK != env->Object_CallMethod_Double(p, yGetter, &y)) {
        HiLog::Error(LABEL, "%{public}s call ygetter failed", __func__);
        return point;
    }
    point["y"] = int(y);
    return point;
}

static json getRect(ani_env *env, ani_object p) {
    auto rect = json();
    static const char *className = "L@ohos/UiTest/RectInner;";
    ani_class cls = findCls(env, className);
    ani_method leftGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>left", nullptr, &leftGetter)) {
        HiLog::Error(LABEL, "%{public}s Find Method <get>left failed", __func__);
    }
    ani_double left;
    if (ANI_OK != env->Object_CallMethod_Double(p, leftGetter, &left)) {
        HiLog::Error(LABEL, "%{public}s call leftgetter failed", __func__);
        return rect;
    }
    rect["left"] = int(left);
    ani_method rightGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>right", nullptr, &rightGetter)) {
        HiLog::Error(LABEL, "%{public}s Find Method <get>right failed", __func__);
    }
    ani_double right;
    if (ANI_OK != env->Object_CallMethod_Double(p, rightGetter, &right)) {
        HiLog::Error(LABEL, "%{public}s call rightgetter failed", __func__);
        return rect;
    }
    rect["right"] = int(right);
    ani_method topGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>top", nullptr, &topGetter)) {
        HiLog::Error(LABEL, "%{public}s call rightgetter failed", __func__);
    }
    ani_double top;
    if (ANI_OK != env->Object_CallMethod_Double(p, topGetter, &top)) {
        return rect;
    }
    rect["top"] = int(top);

        ani_method bottomGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>bottom", nullptr, &bottomGetter)) {
        HiLog::Error(LABEL, "%{public}s Find Method <get>bottom failed", __func__);
    }
    ani_double bottom;
    if (ANI_OK != env->Object_CallMethod_Double(p, bottomGetter, &bottom)) {
        HiLog::Error(LABEL, "%{public}s call bottomGetter failed", __func__);
        return rect;
    }
    rect["bottom"] = int(bottom);
    return rect;
}

static ani_object newRect(ani_env *env, ani_object object, nlohmann::json num) {
    ani_object rect_obj = {};
    static const char *className = "L@ohos/UiTest/RectInner;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        static const char *name = nullptr;
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &rect_obj)) {
        HiLog::Error(LABEL, "Create Rect object failed");
        return rect_obj;
    }
    ani_method setter;
    string direct[] = {"left", "top", "right", "bottom" };
    for (int index=0; index<4; index++) {
        string tag = direct[index];
        char *method_name = strdup(("<set>" + tag).c_str());
        if (ANI_OK != env->Class_FindMethod(cls, method_name, nullptr, &setter)) {
            HiLog::Error(LABEL, "Find Method <set>tag failed");
        }
        if (ANI_OK != env->Object_CallMethod_Void(rect_obj, setter, ani_double(num[tag]))) {
            HiLog::Error(LABEL, "call setter failed %{public}s", direct[index].c_str());
            return rect_obj;
        }
    }
    return rect_obj;
}

static ani_object newPoint(ani_env *env, ani_object obj, int x, int y) {
    ani_object point_obj = {};
    static const char *className = "L@ohos/UiTest/PointInner;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        static const char *name = nullptr;
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &point_obj)) {
        HiLog::Error(LABEL, "Create Point object failed");
        return point_obj;
    }    
    ani_method setter;
    string direct[] = { "x", "y" };
    int num[2] = { x, y };
    for (int index = 0; index < 2; index ++) {
        string tag = direct[index];
        char *method_name = strdup(("<set>" + tag).c_str());
        if (ANI_OK != env->Class_FindMethod(cls, method_name, nullptr, &setter)) {
            HiLog::Error(LABEL, "Find Method <set>tag failed");
        }
        if (ANI_OK != env->Object_CallMethod_Void(point_obj, setter, ani_double(num[index]))) {
            HiLog::Error(LABEL, "call setter failed %{public}s", direct[index].c_str());
            return point_obj;
        }
    }
    return point_obj;
}

static ani_ref createMatrix(ani_env *env, ani_object object) {
    static const char *className = "L@ohos/UiTest/PointerMatrix;";
    ani_class cls = findCls(env, className);
    ani_ref nullref;
    env->GetNull(&nullref);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    } else {
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "PointerMatrix.create";
    Transact(callInfo_, reply_);
    ani_ref nativePointerMatrix = UnmarshalReply(env, callInfo_, reply_);
    ani_object pointer_matrix_object;
    if (ANI_OK != env->Object_New(cls, ctor, &pointer_matrix_object, reinterpret_cast<ani_object>(nativePointerMatrix))) {    
        HiLog::Error(LABEL, "New PointerMatrix Failed %{public}s", __func__);
    }
    return pointer_matrix_object;
}

static void setPoint(ani_env *env, ani_object object, ani_double finger, ani_double step, ani_object point) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_="PointerMatrix.setPoint";
    callInfo_.paramList_.push_back(int(finger));
    callInfo_.paramList_.push_back(int(step));
    callInfo_.paramList_.push_back(getPoint(env, point));
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, object, "nativePointerMatrix"));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return;
}

static ani_boolean BindPointMatrix(ani_env *env) {
    static const char *className = "L@ohos/UiTest/PointerMatrix;";
    ani_class cls = findCls(env, className);
    if (cls == nullptr) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function {"create", nullptr, reinterpret_cast<void*>(createMatrix)},
        ani_native_function {"setPoint", nullptr, reinterpret_cast<void*>(setPoint)},
    };
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}

static ani_ref createOn(ani_env *env, ani_object object, nlohmann::json params, string apiId_) {
    static const char *className = "L@ohos/UiTest/On;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    }
    if (ctor == nullptr || cls== nullptr) {
        return nullptr;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.paramList_ = params;
    string on_obj = aniStringToStdString(env, unwrapp(env, object, "nativeOn"));
    if (on_obj != "") {
        callInfo_.callerObjRef_ = on_obj;
    } else {
        callInfo_.callerObjRef_ = string(REF_SEED_ON);
    }
    Transact(callInfo_, reply_);
    ani_ref nativeOn = UnmarshalReply(env, callInfo_, reply_);
    ani_object on_object;
    if (ANI_OK != env->Object_New(cls, ctor, &on_object, reinterpret_cast<ani_ref>(nativeOn))) {
        HiLog::Error(LABEL, "%{public}s New ON failed !!!", __func__);
    }
    return on_object;
}

static ani_ref within(ani_env *env, ani_object obj, ani_object on) {
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.within");
}

static ani_ref isBefore(ani_env *env, ani_object obj, ani_object on) {
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.isBefore");
}

static ani_ref isAfter(ani_env *env, ani_object obj, ani_object on) {
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.isAfter");
}
static ani_ref id(ani_env *env, ani_object obj, ani_string id, ani_enum_item pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, id));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(pattern, &enumValue);
    params.push_back(enumValue);
    return createOn(env, obj, params, "On.id");
}
static ani_ref text(ani_env *env, ani_object obj, ani_string text, ani_enum_item pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(pattern, &enumValue);
    params.push_back(enumValue);    
    return createOn(env, obj, params, "On.text");
}
static ani_ref type(ani_env *env, ani_object obj, ani_string type, ani_enum_item pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, type));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(pattern, &enumValue);
    params.push_back(enumValue);    
    return createOn(env, obj, params, "On.type");
}
static ani_ref hint(ani_env *env, ani_object obj, ani_string text, ani_enum_item pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(pattern, &enumValue);
    params.push_back(enumValue);    
    return createOn(env, obj, params, "On.hint");
}
static ani_ref description(ani_env *env, ani_object obj, ani_string text, ani_enum_item pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(pattern, &enumValue);
    params.push_back(enumValue);    
    return createOn(env, obj, params, "On.description");
}
static ani_ref inWindow(ani_env *env, ani_object obj, ani_string bundleName) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, bundleName));
    return createOn(env, obj, params, "On.inWindow");
}
static void pushBool(ani_env *env, ani_object input, nlohmann::json &params) {
    ani_boolean ret;
    env->Reference_IsUndefined(reinterpret_cast<ani_ref>(input), &ret);
    if (ret==ANI_FALSE) {
        ani_boolean param;
        HiLog::Info(LABEL, "%{public}s ani_boolean !!!", __func__);
        env->Object_CallMethodByName_Boolean(input, "unboxed", ":Z", &param);
        HiLog::Info(LABEL, "%{public}d ani_boolean !!!", static_cast<int>(param));
        params.push_back(static_cast<bool>(param));
    } else {
        params.push_back(true);
    }
}
static ani_ref enabled(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.enabled");
}
static ani_ref focused(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.focused");
}  
static ani_ref clickable(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.clickable");
}
static ani_ref checked(ani_env *env, ani_object obj, ani_object b) {    
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
     return createOn(env, obj, params, "On.checked");
}
static ani_ref checkable(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
     return createOn(env, obj, params, "On.checkable");
}
static ani_ref longClickable(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
     return createOn(env, obj, params, "On.longClickable");
}
static ani_ref selected(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.selected");
}
static ani_ref scrollable(ani_env *env, ani_object obj, ani_object b) {
    nlohmann::json params = nlohmann::json::array();
    HiLog::Info(LABEL, "%{public}s scrollable !!!", __func__);
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.scrollable");
}
  
static ani_boolean BindOn(ani_env *env) {
    static const char *className = "L@ohos/UiTest/On;";
    ani_class cls = findCls(env, className);
    if (cls == nullptr) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function {"id", nullptr, reinterpret_cast<void*>(id)},
        ani_native_function {"text", nullptr, reinterpret_cast<void*>(text)},
        ani_native_function {"type", nullptr, reinterpret_cast<void*>(type)},
        ani_native_function {"hint", nullptr, reinterpret_cast<void*>(hint)},
        ani_native_function {"description", nullptr, reinterpret_cast<void*>(description)},
        ani_native_function {"inWindow", nullptr, reinterpret_cast<void*>(inWindow)},
        ani_native_function {"enabled", nullptr, reinterpret_cast<void*>(enabled)},
        ani_native_function {"within", nullptr, reinterpret_cast<void*>(within)},
        ani_native_function {"selected", nullptr, reinterpret_cast<void*>(selected)},
        ani_native_function {"focused", nullptr, reinterpret_cast<void*>(focused)},
        ani_native_function {"clickable", nullptr, reinterpret_cast<void*>(clickable)},
        ani_native_function {"checkable", nullptr, reinterpret_cast<void*>(checkable)},
        ani_native_function {"checked", nullptr, reinterpret_cast<void*>(checked)},
        ani_native_function {"scrollable", nullptr, reinterpret_cast<void*>(scrollable)},
        ani_native_function {"isBefore", nullptr, reinterpret_cast<void*>(isBefore)},
        ani_native_function {"isAfter", nullptr, reinterpret_cast<void*>(isAfter)},
        ani_native_function {"longClickable", nullptr, reinterpret_cast<void*>(longClickable)},
    };
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {        
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}
 
static ani_ref create([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_class clazz) {
    static const char *className = "L@ohos/UiTest/Driver;";
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found !!!", className);
        return nullref;
    }
    ani_method ctor = nullptr;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "Lstd/core/String;:V", &ctor)) {
        HiLog::Error(LABEL, "%{public}s Ctor Not found !!!", className);
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.create";
    Transact(callInfo_, reply_);

    ani_ref nativeDriver = UnmarshalReply(env, callInfo_, reply_);
    ani_object driver_object;
    if (ANI_OK != env->Object_New(cls, ctor, &driver_object, reinterpret_cast<ani_object>(nativeDriver))) {
        HiLog::Error(LABEL, "%{public}s New Driver Failed!!!", __func__);
    }
    return driver_object;
}

static ani_boolean delayMsSync(ani_env *env, ani_object obj, ani_double t) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.delayMs";
    callInfo_.paramList_.push_back(int(t));
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref findComponentSync(ani_env *env, ani_object obj, ani_object on_obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.findComponent";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    ani_object com_obj;
    static const char *className = "L@ohos/UiTest/Component;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &com_obj, reinterpret_cast<ani_object>(nativeComponent))) {
        HiLog::Error(LABEL, "%{public}s New Component Failed !!!", __func__);
    }
    return com_obj;
}

static ani_object findComponentsSync(ani_env *env, ani_object obj, ani_object on_obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.findComponents";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    Transact(callInfo_, reply_);
    ani_object nativeComponents = reinterpret_cast<ani_object>(UnmarshalReply(env, callInfo_, reply_));
    ani_object com_obj;
    ani_object com_objs;
    ani_class arrayCls = findCls(env, "Lescompat/Array");
    ani_method ctor = nullptr;
    if (arrayCls != nullptr) {
        ctor = findCtorMethod(env, arrayCls, "I:V");
    }
    ani_size com_size = reply_.resultValue_.size();
    if (ANI_OK!= env->Object_New(arrayCls, ctor, &com_objs, com_size)) {
        std::cout<<"Object_New array failed" << std::endl;
        return com_objs;
    }
    static const char *className = "L@ohos/UiTest/Component;";
    ani_class cls = findCls(env, className);
    ani_method com_ctor;
    if (cls != nullptr) {
        com_ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    }
    if (cls == nullptr || com_ctor == nullptr) {
        return nullptr;
    }
    for (ani_size i = 0; i < com_size; i++) {
        ani_ref item;
        if (ANI_OK != env->Object_CallMethodByName_Ref(nativeComponents, "$_get", nullptr, &item, i)) {            
            HiLog::Error(LABEL, "%{public}s Object_CallMethodByName_Ref failed !!!", __func__);
            break;
        }
        if (ANI_OK != env->Object_New(cls, com_ctor, &com_obj, reinterpret_cast<ani_object>(item))) {
            HiLog::Error(LABEL, "%{public}s component Object new failed !!!", __func__);
        }
        if (ANI_OK != env->Object_CallMethodByName_Void(com_objs, "$_set", "ILstd/core/Object;:V", i, com_obj)) {
            HiLog::Error(LABEL, "Object callmethod by name failed :$_set");
            break;
        }
    }
    return com_objs;
}

static ani_boolean assertComponentExistSync(ani_env *env, ani_object obj, ani_object on_obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.assertComponentExist";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static void performClick(ani_env *env, ani_object obj, ani_double x, ani_double y, string api) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = api;
    callInfo_.paramList_.push_back(int(x));
    callInfo_.paramList_.push_back(int(y));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean clickSync(ani_env *env, ani_object obj, ani_double x, ani_double y) {
    string api_name = "Driver.click";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_boolean doubleClickSync(ani_env *env, ani_object obj, ani_double x, ani_double y) {
    string api_name = "Driver.doubleClick";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_boolean longClickSync(ani_env *env, ani_object obj, ani_double x, ani_double y) {
    string api_name = "Driver.longClick";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_ref performDriver(ani_env *env, ani_object obj, string api) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = api;
    Transact(callInfo_, reply_);
    return UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean pressBackSync(ani_env *env, ani_object obj) {
    performDriver(env, obj, "Driver.pressBack");
    return true;
}

static ani_boolean flingSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_double stepLen, ani_double speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.fling";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    callInfo_.paramList_.push_back(int(stepLen));
    callInfo_.paramList_.push_back(int(speed));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean flingSyncDirection(ani_env *env, ani_object obj, ani_enum_item direction, ani_double speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.fling";
    ani_int enumValue;
    env->EnumItem_GetValue_Int(direction, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    callInfo_.paramList_.push_back(int(speed));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean inputTextSync(ani_env *env, ani_object obj, ani_object p, ani_string text)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.inputText";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(aniStringToStdString(env, text));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean swipeSync(ani_env *env, ani_object obj, ani_double x1, ani_double y1, ani_double x2, ani_double y2, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.swipe";
    callInfo_.paramList_.push_back(int(x1));
    callInfo_.paramList_.push_back(int(y1));
    callInfo_.paramList_.push_back(int(x2));
    callInfo_.paramList_.push_back(int(y2));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean dragSync(ani_env *env, ani_object obj, ani_double x1, ani_double y1, ani_double x2, ani_double y2, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.drag";
    callInfo_.paramList_.push_back(int(x1));
    callInfo_.paramList_.push_back(int(y1));
    callInfo_.paramList_.push_back(int(x2));
    callInfo_.paramList_.push_back(int(y2));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static json getWindowFilter(ani_env *env, ani_object f)
{
    auto filter = json();
    static const char *className = "L@ohos/UiTest/WindowFilterInner;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "Not found className: %{public}s", className);
        return filter;
    }
    
    string list[] = { "bundleName", "title", "focused", "active" };
    for (int i = 0; i < 4; i++) {
        char *cstr = new char[list[i].length() + 1];
        strcpy(cstr, list[i].c_str());
        if (i < 2) {
          ani_ref value;
          if (env->Object_GetPropertyByName_Ref(f, cstr, &value) != ANI_OK) {
              HiLog::Error(LABEL, "GetPropertyByName %{public}s fail", cstr);
              continue;
          }
          ani_boolean ret = false;
          if (env->Reference_IsUndefined(value, &ret) == ANI_OK) {
              filter[list[i]] = aniStringToStdString(env, reinterpret_cast<ani_string>(value));
          }
        } else {
            ani_boolean value;
            if (env->Object_GetPropertyByName_Boolean(f, cstr, &value) == ANI_OK) {
              filter[list[i]] = value;
            }
        }
    }
    return filter;
}

 
static ani_object findWindowSync(ani_env *env, ani_object obj, ani_object filter)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.findWindow";
    callInfo_.paramList_.push_back(getWindowFilter(env, filter));
    Transact(callInfo_, reply_);
    ani_ref nativeWindow = UnmarshalReply(env, callInfo_, reply_);
    ani_object window_obj;
    static const char *className = "L@ohos/UiTest/UiWindow;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        static const char *name = "Lstd/core/String;:V";
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &window_obj, reinterpret_cast<ani_object>(nativeWindow))) {
        HiLog::Error(LABEL, "New UiWindow failed");
    }
    return window_obj;
}

static ani_object createUIEventObserverSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.createUIEventObserver";
    Transact(callInfo_, reply_);
    ani_ref nativeUIEventObserver = UnmarshalReply(env, callInfo_, reply_);
    ani_object observer_obj;
    static const char *className = "L@ohos/UiTest/UIEventObserver;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        static const char *name = "Lstd/core/String;:V";
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        HiLog::Error(LABEL, "Not found className/ctor: %{public}s", className);
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &observer_obj, reinterpret_cast<ani_object>(nativeUIEventObserver))) {
        HiLog::Error(LABEL, "New UIEventObserver fail");
    }
    return observer_obj;
}

static ani_boolean injectMultiPointerActionSync(ani_env *env, ani_object obj, ani_object pointers, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.injectMultiPointerAction";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, pointers, "nativeDriver")));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean triggerKeySync(ani_env *env, ani_object obj, ani_double keyCode)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerKey";
    callInfo_.paramList_.push_back(int(keyCode));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean wakeUpDisplaySync(ani_env *env, ani_object obj)
{
    performDriver(env, obj, "Driver.wakeUpDisplay");
    return true;
}

static ani_boolean pressHomeSync(ani_env *env, ani_object obj)
{
    performDriver(env, obj, "Driver.pressHome");
    return true;
}

static ani_ref getDisplaySizeSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplaySize";
    Transact(callInfo_, reply_);
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"]);
    return p;
}

static ani_object getDisplayDensitySync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplayDensity";
    Transact(callInfo_, reply_);
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"]);
    return p;
}

static ani_object getDisplayRotationSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplayRotation";
    Transact(callInfo_, reply_);
    ani_enum enumType;
    if(ANI_OK != env->FindEnum("L@ohos/UiTest/DisplayRotation;", &enumType)) {
        HiLog::Error(LABEL, "Find Enum Faild: %{public}s", __func__); 
    }
    ani_enum_item enumItem;
    auto index = static_cast<uint8_t>(reply_.resultValue_.get<int>());
    env->Enum_GetEnumItemByIndex(enumType, index, &enumItem);
    HiLog::Info(LABEL, " DisplayRotation:  %{public}d ", index);
    return enumItem;
}

static ani_boolean waitForIdleSync(ani_env *env, ani_object obj, ani_double idleTime, ani_double timeout)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.waitForIdle";
    callInfo_.paramList_.push_back(int(idleTime));
    callInfo_.paramList_.push_back(int(timeout));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_object waitForComponentSync(ani_env *env, ani_object obj, ani_object on_obj, ani_double time)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.waitForComponent";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, obj, "nativeOn")));
    callInfo_.paramList_.push_back(int(time));
    Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    ani_object component_obj;
    static const char *className = "L@ohos/UiTest/Component;";
    ani_class cls = findCls(env, className);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        static const char *name = "Lstd/core/String;:V";
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &component_obj, reinterpret_cast<ani_object>(nativeComponent))) {
        HiLog::Error(LABEL, "New Component fail: %{public}s", __func__); 
    }
    return component_obj;
}
  
static ani_boolean triggerCombineKeysSync(ani_env *env, ani_object obj, ani_double key0, ani_double key1, ani_object key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerCombineKeys";
    callInfo_.paramList_.push_back(int(key0));
    callInfo_.paramList_.push_back(int(key1));
    pushParam(env, key2, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean setDisplayRotationSync(ani_env *env, ani_object obj, ani_enum_item rotation)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.setDisplayRotation";
    ani_int enumValue;
    env->EnumItem_GetValue_Int(rotation, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean screenCaptureSync(ani_env *env, ani_object obj, ani_string path, ani_object rect)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.screenCapture";
    string savePath = aniStringToStdString(env, path);
    auto fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false;
    }
    callInfo_.paramList_.push_back(fd);
    callInfo_.paramList_.push_back(getRect(env, rect));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean screenCapSync(ani_env *env, ani_object obj, ani_string path)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.screenCap";
    string savePath = aniStringToStdString(env, path);
    auto fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false;
    }
    callInfo_.paramList_.push_back(fd);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean setDisplayRotationEnabledSync(ani_env *env, ani_object obj, ani_boolean enable)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.setDisplayRotationEnabled";
    callInfo_.paramList_.push_back(static_cast<bool>(enable));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penSwipeSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_object speed, ani_object pressure)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penSwipe";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    pushParam(env, speed, callInfo_, true);
    pushParam(env, pressure, callInfo_, false);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penClickSync(ani_env *env, ani_object obj, ani_object p)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penClick";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penDoubleClickSync(ani_env *env, ani_object obj, ani_object p)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penDoubleClick";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penLongClickSync(ani_env *env, ani_object obj, ani_object p, ani_object pressure)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penLongClick";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    pushParam(env, pressure, callInfo_, false);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseScrollSync(ani_env *env, ani_object obj, ani_object p, ani_boolean down, ani_double dis, ani_object key1, ani_object key2,
    ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseScroll";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(static_cast<bool>(down));
    callInfo_.paramList_.push_back(int(dis));
    pushParam(env, key1, callInfo_, true);
    pushParam(env, key2, callInfo_, true);
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseMoveWithTrackSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseMoveWithTrack";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseDragkSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseDragk";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseMoveToSync(ani_env *env, ani_object obj, ani_object p)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseMoveTo";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseClickSync(ani_env *env, ani_object obj, ani_object p, ani_enum_item btnId, ani_object key1, ani_object key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseClickSync";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    ani_int enumValue;
    env->EnumItem_GetValue_Int(btnId, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    pushParam(env, key1, callInfo_, true);
    pushParam(env, key2, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseDoubleClickSync(ani_env *env, ani_object obj, ani_object p, ani_enum_item btnId, ani_object key1, ani_object key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseDoubleClick";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    ani_int enumValue;
    env->EnumItem_GetValue_Int(btnId, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    pushParam(env, key1, callInfo_, true);
    pushParam(env, key2, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseLongClickSync(ani_env *env, ani_object obj, ani_object p, ani_enum_item btnId, ani_object key1, ani_object key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseLongClick";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    ani_int enumValue;
    env->EnumItem_GetValue_Int(btnId, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    pushParam(env, key1, callInfo_, true);
    pushParam(env, key2, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean injectPenPointerActionSync(ani_env *env, ani_object obj, ani_object pointers, ani_object speed, ani_object pressure)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.injectPenPointerAction";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, obj, "nativePointerMatrix")));
    pushParam(env, speed, callInfo_, true);
    pushParam(env, pressure, callInfo_, false);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}


static json getTouchPadSwipeOptions(ani_env *env, ani_object f)
{
    auto options = json();
    static const char *className = "L@ohos/UiTest/TouchPadSwipeOptionsInner;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "Not found: %{public}s", className);
        return options;
    }
    string list[] = { "stay", "speed" };
    for (int index = 0; index<2; index++) {
        ani_field field;
        char* cstr = new char[list[index].length() + 1];
        strcpy(cstr, list[index].c_str()); 
        compareAndReport(ANI_OK ,env->Class_FindField(cls, cstr, &field),
        "Class_FindField Failed '"+std::string(className) +"'", "Find field?:?");
        ani_ref ref;
        compareAndReport(ANI_OK ,env->Object_GetField_Ref(f, field, &ref),
        "Object_GetField_Ref Failed '"+std::string(className) +"'", "get ref");
        if (index==0) {
            ani_boolean value;
            compareAndReport(ANI_OK ,env->Object_CallMethodByName_Boolean(static_cast<ani_object>(ref), "unboxed", nullptr ,&value),
            "Object_CallMethodByName_Boolean Failed '"+std::string(className) +"'", "get boolean value");
            compareAndReport(1,1,"", std::to_string(value));
            options[list[index]] = static_cast<bool>(value);
        } else {
            ani_double value;
            compareAndReport(ANI_OK ,env->Object_CallMethodByName_Double(static_cast<ani_object>(ref), "unboxed", nullptr ,&value),
            "Object_CallMethodByName_Double Failed '"+std::string(className) +"'", "get Int value");
            compareAndReport(1,1,"", std::to_string(value));
            options[list[index]] = int(value);
        }
    }
    return options;
 }
 
static ani_boolean touchPadMultiFingerSwipeSync(ani_env *env, ani_object obj, ani_double fingers, ani_enum_item direction, ani_object touchPadOpt)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.touchPadMultiFingerSwipe";
    callInfo_.paramList_.push_back(int(fingers));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(direction, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    callInfo_.paramList_.push_back(getTouchPadSwipeOptions(env, touchPadOpt));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}
static ani_boolean BindDriver(ani_env *env)
{
    static const char *className = "L@ohos/UiTest/Driver;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found !!!", __func__);
        return false;
    }

    std::array methods = {
        ani_native_function {"create", ":L@ohos/UiTest/Driver;", reinterpret_cast<void *>(create)},
        ani_native_function {"delayMsSync", nullptr, reinterpret_cast<void *>(delayMsSync)},
        ani_native_function {"clickSync", nullptr, reinterpret_cast<void *>(clickSync)},
        ani_native_function {"longClickSync", nullptr, reinterpret_cast<void *>(longClickSync)},
        ani_native_function {"doubleClickSync", nullptr, reinterpret_cast<void *>(doubleClickSync)},
        ani_native_function {"flingSync", nullptr, reinterpret_cast<void *>(flingSync)},
        ani_native_function {"flingSyncDirection", nullptr, reinterpret_cast<void *>(flingSyncDirection)},
        ani_native_function {"swipeSync", nullptr, reinterpret_cast<void *>(swipeSync)},
        ani_native_function {"dragSync", nullptr, reinterpret_cast<void *>(dragSync)},
        ani_native_function {"pressBackSync", nullptr, reinterpret_cast<void *>(pressBackSync)},
        ani_native_function {"assertComponentExistSync", nullptr, reinterpret_cast<void *>(assertComponentExistSync)},
        ani_native_function {"triggerKeySync", nullptr, reinterpret_cast<void *>(triggerKeySync)},
        ani_native_function {"inputTextSync", nullptr, reinterpret_cast<void *>(inputTextSync)},
        ani_native_function {"findWindowSync", nullptr, reinterpret_cast<void *>(findWindowSync)},
        ani_native_function {"createUIEventObserver", nullptr,
            reinterpret_cast<void *>(createUIEventObserverSync)},
        ani_native_function {"wakeUpDisplaySync", nullptr, reinterpret_cast<void *>(wakeUpDisplaySync)},
        ani_native_function {"pressHomeSync", nullptr, reinterpret_cast<void *>(pressHomeSync)},
        ani_native_function {"getDisplaySizeSync", nullptr, reinterpret_cast<void *>(getDisplaySizeSync)},
        ani_native_function {"getDisplayDensitySync", nullptr, reinterpret_cast<void *>(getDisplayDensitySync)},
        ani_native_function {"getDisplayRotationSync", nullptr, reinterpret_cast<void *>(getDisplayRotationSync)},
        ani_native_function {"findComponentsSync", nullptr, reinterpret_cast<void *>(findComponentsSync)},
        ani_native_function {"findComponentSync", nullptr, reinterpret_cast<void *>(findComponentSync)},
        ani_native_function {"waitForIdleSync", nullptr, reinterpret_cast<void *>(waitForIdleSync)},
        ani_native_function {"waitForComponentSync", nullptr, reinterpret_cast<void *>(waitForComponentSync)},
        ani_native_function {"triggerCombineKeysSync", nullptr, reinterpret_cast<void *>(triggerCombineKeysSync)},
        ani_native_function {"setDisplayRotationEnabledSync", nullptr, reinterpret_cast<void *>(setDisplayRotationEnabledSync)},
        ani_native_function {"setDisplayRotationSync", nullptr, reinterpret_cast<void *>(setDisplayRotationSync)},
        ani_native_function {"screenCaptureSync", nullptr, reinterpret_cast<void *>(screenCaptureSync)},
        ani_native_function {"screenCapSync", nullptr, reinterpret_cast<void *>(screenCapSync)},
        ani_native_function {"penSwipeSync", nullptr, reinterpret_cast<void *>(penSwipeSync)},
        ani_native_function {"penClickSync", nullptr, reinterpret_cast<void *>(penClickSync)},
        ani_native_function {"penDoubleClickSync", nullptr, reinterpret_cast<void *>(penDoubleClickSync)},
        ani_native_function {"penLongClickSync", nullptr, reinterpret_cast<void *>(penLongClickSync)},
        ani_native_function {"mouseScrollSync", nullptr, reinterpret_cast<void *>(mouseScrollSync)},
        ani_native_function {"mouseMoveWithTrackSync", nullptr, reinterpret_cast<void *>(mouseMoveWithTrackSync)},
        ani_native_function {"mouseMoveToSync", nullptr, reinterpret_cast<void *>(mouseMoveToSync)},
        ani_native_function {"mouseDragkSync", nullptr, reinterpret_cast<void *>(mouseDragkSync)},
        ani_native_function {"mouseClickSync", nullptr, reinterpret_cast<void *>(mouseClickSync)},
        ani_native_function {"mouseDoubleClickSync", nullptr, reinterpret_cast<void *>(mouseDoubleClickSync)},
        ani_native_function {"mouseLongClickSync", nullptr, reinterpret_cast<void *>(mouseLongClickSync)},
        ani_native_function {"injectMultiPointerActionSync", nullptr, reinterpret_cast<void *>(injectMultiPointerActionSync)},
        ani_native_function {"injectPenPointerActionSync", nullptr, reinterpret_cast<void *>(injectPenPointerActionSync)},
        ani_native_function {"touchPadMultiFingerSwipeSync", nullptr, reinterpret_cast<void *>(touchPadMultiFingerSwipeSync)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {         
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}


static void performWindow(ani_env *env, ani_object obj, string api)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = api;
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean splitSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.split");
    return true;
}

static ani_boolean resumeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.resume");
    return true;
}

static ani_boolean closeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.close");
    return true;
}

static ani_boolean minimizeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.minimize");
    return true;
}

static ani_boolean maximizeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.maximize");
    return true;
}

static ani_boolean focusSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.focus");
    return true;
}

static ani_ref isFocusedSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.isFocused";
    Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref isActiveSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.isActive";
    Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_boolean resizeSync(ani_env *env, ani_object obj, ani_double w, ani_double h, ani_enum_item d)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.resize";
    callInfo_.paramList_.push_back(int(w));
    callInfo_.paramList_.push_back(int(h));
    ani_int enumValue;
    env->EnumItem_GetValue_Int(d, &enumValue);
    callInfo_.paramList_.push_back(enumValue);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean moveToSync(ani_env *env, ani_object obj, ani_double x, ani_double y)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.moveTo";
    callInfo_.paramList_.push_back(int(x));
    callInfo_.paramList_.push_back(int(y));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref getWindowModeSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getWindowMode";
    Transact(callInfo_, reply_);
    ani_enum enumType;
    if(ANI_OK != env->FindEnum("L@ohos/UiTest/WindowMode;", &enumType)) {
        HiLog::Error(LABEL, "Not found enum item: %{public}s", __func__);
    }
    ani_enum_item enumItem;
    auto index = static_cast<uint8_t>(reply_.resultValue_.get<int>());
    env->Enum_GetEnumItemByIndex(enumType, index, &enumItem);
    HiLog::Info(LABEL, " getWindowMode:  %{public}d ", index);
    return enumItem;
}

static ani_ref getBundleNameSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getBundleName";
    Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref getTitileSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getTitile";
    Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref getBoundsSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getBounds";
    Transact(callInfo_, reply_);
    ani_object r = newRect(env, obj, reply_.resultValue_);
    return r;
}

static ani_boolean BindWindow(ani_env *env)
{
    static const char *className = "L@ohos/UiTest/UiWindow;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }

    std::array methods = {
        ani_native_function {"splitSync", nullptr, reinterpret_cast<void *>(splitSync)},
        ani_native_function {"resumeSync", nullptr, reinterpret_cast<void *>(resumeSync)},
        ani_native_function {"closeSync", nullptr, reinterpret_cast<void *>(closeSync)},
        ani_native_function {"minimizeSync", nullptr, reinterpret_cast<void *>(minimizeSync)},
        ani_native_function {"maximizeSync", nullptr, reinterpret_cast<void *>(maximizeSync)},
        ani_native_function {"focusSync", nullptr, reinterpret_cast<void *>(focusSync)},
        ani_native_function {"isFocusedSync", nullptr, reinterpret_cast<void *>(isFocusedSync)},
        ani_native_function {"isActiveSync", nullptr, reinterpret_cast<void *>(isActiveSync)},
        ani_native_function {"resizeSync", nullptr, reinterpret_cast<void *>(resizeSync)},
        ani_native_function {"moveToSync", nullptr, reinterpret_cast<void *>(moveToSync)},
        ani_native_function {"getWindowModeSync", nullptr, reinterpret_cast<void *>(getWindowModeSync)},
        ani_native_function {"getBundleNameSync", nullptr, reinterpret_cast<void *>(getBundleNameSync)},
        ani_native_function {"getTitileSync", nullptr, reinterpret_cast<void *>(getTitileSync)},
        ani_native_function {"winGetBoundsSync", nullptr, reinterpret_cast<void *>(getBoundsSync)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {         
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}

static ani_ref getBoundsCenterSync(ani_env *env, ani_object obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.apiId_ = "Component.getBoundsCenter";
    Transact(callInfo_, reply_);
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"]);
    HiLog::Info(LABEL, " reply_.resultValue_[x]  %{public}s ", reply_.resultValue_["x"].dump().c_str());
    HiLog::Info(LABEL, " reply_.resultValue_[y]  %{public}s ", reply_.resultValue_["y"].dump().c_str());
    return p;
}  
static ani_ref comGetBounds(ani_env *env, ani_object obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.apiId_ = "Component.getBounds";
    Transact(callInfo_, reply_);
    ani_object r = newRect(env, obj, reply_.resultValue_);
    return r;
}

static ani_ref performComponentApi(ani_env *env, ani_object obj, string apiId_) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    return result;
}

static ani_boolean comClick(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.click");
    return true;
}

static ani_boolean comDoubleClick(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.doubleClick");
    return true;
}

static ani_boolean comLongClick(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.longClick");
    return true;
}

static ani_boolean comDragToSync(ani_env *env, ani_object obj, ani_object target) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.dragTo";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, target, "nativeComponent")));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref getText(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getText");
}

static ani_ref getType(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getType");
}

static ani_ref getId(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getId");
}

static ani_ref getHint(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getHint");
}

static ani_ref getDescription(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getDescription");
}

static ani_boolean comInputText(ani_env *env, ani_object obj, ani_string txt) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.inputText";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, txt));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean clearText(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.clearText");
    return true;
}

static ani_boolean scrollToTop(ani_env *env, ani_object obj, ani_object speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollToTop";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean scrollToBottom(ani_env *env, ani_object obj, ani_object speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollToBottom";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean scrollSearch(ani_env *env, ani_object obj, ani_object on, ani_boolean vertical, ani_double offset) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollSearch";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on, "nativeComponent")));
    callInfo_.paramList_.push_back(static_cast<bool>(vertical));
    callInfo_.paramList_.push_back(int(offset));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static void pinch(ani_env *env, ani_object obj, ani_double scale, string apiId_) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(scale);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return;
}

static ani_boolean pinchIn(ani_env *env, ani_object obj, ani_double scale) {
    pinch(env, obj, scale, "Component.pinchIn");
    return true;
}

static ani_boolean pinchOut(ani_env *env, ani_object obj, ani_double scale) {
    pinch(env, obj, scale, "Component.pinchOut");
    return true;
}
static ani_boolean performComponentApiBool(ani_env *env, ani_object obj, string apiId_) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    Transact(callInfo_, reply_);
    return reply_.resultValue_.get<bool>();
}
static ani_boolean isSelected(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isSelected");
}

static ani_boolean isClickable(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isClickable");
}

static ani_boolean isLongClickable(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isLongClickable");
}

static ani_boolean isScrollable(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isScrollable");
}

static ani_boolean isEnabled(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isEnabled");
}

static ani_boolean isFocused(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isFocused");
}

static ani_boolean isChecked(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isChecked");
}

static ani_boolean isCheckable(ani_env *env, ani_object obj) {
    return performComponentApiBool(env, obj, "Component.isCheckable");
}

static ani_boolean BindComponent(ani_env *env) {
    static const char *className = "L@ohos/UiTest/Component;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function {"comClickSync", nullptr, reinterpret_cast<void *>(comClick)},
        ani_native_function {"comLongClickSync", nullptr, reinterpret_cast<void *>(comLongClick)},
        ani_native_function {"comDoubleClickSync", nullptr, reinterpret_cast<void *>(comDoubleClick)},
        ani_native_function {"comDragToSync", nullptr, reinterpret_cast<void *>(comDragToSync)},
        ani_native_function {"comGetBoundsSync", nullptr, reinterpret_cast<void *>(comGetBounds)},
        ani_native_function {"getBoundsCenterSync", nullptr, reinterpret_cast<void *>(getBoundsCenterSync)},
        ani_native_function {"getTextSync", nullptr, reinterpret_cast<void *>(getText)},
        ani_native_function {"getTypeSync", nullptr, reinterpret_cast<void *>(getType)},
        ani_native_function {"getIdSync", nullptr, reinterpret_cast<void *>(getId)},
        ani_native_function {"getHintSync", nullptr, reinterpret_cast<void *>(getHint)},
        ani_native_function {"getDescriptionSync", nullptr, reinterpret_cast<void *>(getDescription)},
        ani_native_function {"comInputTextSync", nullptr, reinterpret_cast<void *>(comInputText)},
        ani_native_function {"clearTextSync", nullptr, reinterpret_cast<void *>(clearText)},
        ani_native_function {"scrollToTopSync", nullptr, reinterpret_cast<void *>(scrollToTop)},
        ani_native_function {"scrollToBottomSync", nullptr, reinterpret_cast<void *>(scrollToBottom)},
        ani_native_function {"scrollSearchSync", nullptr, reinterpret_cast<void *>(scrollSearch)},
        ani_native_function {"pinchInSync", nullptr, reinterpret_cast<void *>(pinchIn)},
        ani_native_function {"pinchOutSync", nullptr, reinterpret_cast<void *>(pinchOut)},
        ani_native_function {"isScrollableSync", nullptr, reinterpret_cast<void *>(isScrollable)},
        ani_native_function {"isSelectedSync", nullptr, reinterpret_cast<void *>(isSelected)},
        ani_native_function {"isLongClickableSync", nullptr, reinterpret_cast<void *>(isLongClickable)},
        ani_native_function {"isClickableSync", nullptr, reinterpret_cast<void *>(isClickable)},
        ani_native_function {"isFocusedSync", nullptr, reinterpret_cast<void *>(isFocused)},
        ani_native_function {"isEnabledSync", nullptr, reinterpret_cast<void *>(isEnabled)},
        ani_native_function {"isCheckedSync", nullptr, reinterpret_cast<void *>(isChecked)},
        ani_native_function {"isCheckableSync", nullptr, reinterpret_cast<void *>(isCheckable)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {         
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}
static void onceSync(ani_env *env, ani_object obj, ani_string type, ani_object callback)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeUIEventObserver"));
    callInfo_.apiId_ = "UIEventObserver.once";
    callInfo_.paramList_.push_back(aniStringToStdString(env, type));
    UiEventObserverAni::Get().PreprocessCallOnce(env, callInfo_, obj, callback, reply_);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}
static ani_boolean BindUiEventObserver(ani_env *env)
{
    static const char *className = "L@ohos/UiTest/UIEventObserver;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function {"onceSync", nullptr, reinterpret_cast<void *>(onceSync)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}
void StsUiTestInit(ani_env *env)
{
    HiLog::Info(LABEL, "%{public}s StsUiTestInit call", __func__);
    ani_status status = ANI_ERROR;
    if (env->ResetError() != ANI_OK) {         
        HiLog::Error(LABEL, "%{public}s ResetError failed", __func__);
    }
    ani_namespace ns;
    status = env->FindNamespace("L@ohos/UiTest/UiTest;", &ns);
    if (status != ANI_OK) {
        HiLog::Error(LABEL, "FindNamespace UiTest failed status : %{public}d", status);
        return;
    }
    std::array kitFunctions = {
        ani_native_function {"ScheduleEstablishConnection", nullptr, reinterpret_cast<void *>(ScheduleEstablishConnection)},   
        ani_native_function {"GetConnectionStat", nullptr, reinterpret_cast<void *>(GetConnectionStat)},     
    };
    status = env->Namespace_BindNativeFunctions(ns, kitFunctions.data(), kitFunctions.size());
    if (status != ANI_OK) {
        HiLog::Error(LABEL,"Namespace_BindNativeFunctions failed status : %{public}d", status);
    }
    if (env->ResetError() != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s ResetError failed", __func__);
    }
    HiLog::Info(LABEL, "%{public}s StsUiTestInit end", __func__);
}
ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result) {
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        HiLog::Error(LABEL, "%{public}s UITest: Unsupported ANI_VERSION_1 !!!", __func__);
        return (ani_status)ANI_ERROR;
    }
    StsUiTestInit(env); 
    auto status = true;
    status &= BindDriver(env);
    status &= BindOn(env);
    status &= BindComponent(env);
    status &= BindWindow(env);
    status &= BindPointMatrix(env);
    status &= BindUiEventObserver(env);
    if (!status) {
        HiLog::Error(LABEL, "%{public}s ani_error", __func__);
        return ANI_ERROR;
    }
    HiLog::Info(LABEL, "%{public}s ani_bind success !!!", __func__);
    *result = ANI_VERSION_1;
    return ANI_OK;
}
