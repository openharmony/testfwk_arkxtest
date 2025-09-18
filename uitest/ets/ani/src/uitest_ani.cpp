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
#include <ani_signature_builder.h>
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
using namespace arkts::ani_signature;
static ApiTransactor g_apiTransactClient(false);
static future<void> g_establishConnectionFuture;

template <typename T>
void compareAndReport(const T &param1, const T &param2, const std::string &errorMessage, const std::string &message)
{
    if (param1 != param2) {
        HiLog::Error(LABEL, "compareAndReport: %{public}s", errorMessage.c_str());
        return;
    } else {
        HiLog::Info(LABEL, "compareAndReport: %{public}s", message.c_str());
        return;
    }
}

static void pushParam(ani_env *env, ani_object input, ApiCallInfo &callInfo_, bool isInt)
{
    ani_boolean ret;
    env->Reference_IsUndefined(reinterpret_cast<ani_ref>(input), &ret);
    if (ret == ANI_FALSE) {
        if (isInt) {
            ani_int param;
            env->Object_CallMethodByName_Int(input, "unboxed", nullptr, &param);
            callInfo_.paramList_.push_back(param);
        } else {
            ani_double param;
            arkts::ani_signature::SignatureBuilder rd{};
            rd.SetReturnDouble();
            env->Object_CallMethodByName_Double(input, "unboxed", rd.BuildSignatureDescriptor().c_str(), &param);
            callInfo_.paramList_.push_back(param);
        }
    }
}

static string aniStringToStdString([[maybe_unused]] ani_env *env, ani_string string_object)
{
    ani_size strSize;
    env->String_GetUTF8Size(string_object, &strSize);
    std::vector<char> buffer(strSize + 1);
    char *utf8_buffer = buffer.data();
    ani_size bytes_written = 0;
    env->String_GetUTF8(string_object, utf8_buffer, strSize + 1, &bytes_written);
    utf8_buffer[bytes_written] = '\0';
    std::string s = std::string(utf8_buffer);
    std::cout << s << std::endl;
    return s;
}

static ani_class findCls(ani_env *env, const char *className)
{
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (ANI_OK != env->FindClass(className, &cls)) {
        HiLog::Error(LABEL, "Not found className: %{public}s", className);
    }
    return cls;
}

static ani_method findCtorMethod(ani_env *env, ani_class cls, const char *name)
{
    ani_method ctor = nullptr;
    if (ANI_OK != env->Class_FindMethod(cls, Builder::BuildConstructorName().c_str(), name, &ctor)) {
        HiLog::Error(LABEL, "Not found ctor: %{public}s", name);
    }
    return ctor;
}

static ani_int getEnumValue(ani_env *env, ani_enum_item pattern)
{
    ani_boolean ret;
    env->Reference_IsUndefined(reinterpret_cast<ani_ref>(pattern), &ret);
    ani_int enumValue = 0;
    if (ret == ANI_FALSE) {
        if (ANI_OK != env->EnumItem_GetValue_Int(pattern, &enumValue)) {
            HiLog::Error(LABEL, "%{public}s Not get pattern !!!", __func__);
        }
    }
    return enumValue;
}

static void waitForConnectionIfNeed()
{
    if (g_establishConnectionFuture.valid()) {
        HiLog::Error(LABEL, "%{public}s. Begin Wait for Connection", __func__);
        g_establishConnectionFuture.get();
    }
}

static void Transact(ApiCallInfo callInfo_, ApiReplyInfo &reply_)
{
    waitForConnectionIfNeed();
    g_apiTransactClient.Transact(callInfo_, reply_);
}
static ani_ref UnmarshalObject(ani_env *env, nlohmann::json resultValue_)
{
    const auto resultType = resultValue_.type();
    ani_ref result = nullptr;
    if (resultType == nlohmann::detail::value_t::null) {
        return result;
    } else if (resultType != nlohmann::detail::value_t::string) {
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

static ani_ref UnmarshalReply(ani_env *env, const ApiCallInfo callInfo_, const ApiReplyInfo &reply_)
{
    if (callInfo_.fdParamIndex_ >= 0) {
        auto fd = callInfo_.paramList_.at(INDEX_ZERO).get<int>();
        (void)close(fd);
    }
    HiLog::Info(LABEL, "%{public}s.Start to UnmarshalReply", __func__);
    const auto &message = reply_.exception_.message_;
    ErrCode code = reply_.exception_.code_;
    if (code == INTERNAL_ERROR || code == ERR_INTERNAL) {
        HiLog::Error(LABEL, "UItest : ERRORINFO: code='%{public}u', message='%{public}s'", code, message.c_str());
        return nullptr;
    } else if (reply_.exception_.code_ != NO_ERROR) {
        HiLog::Error(LABEL, "UItest : ERRORINFO: code='%{public}u', message='%{public}s'", code, message.c_str());
        ErrorHandler::Throw(env, code, message);
        return nullptr;
    }
    HiLog::Info(LABEL, "UITEST: Start to unmarshall return value:%{public}s", reply_.resultValue_.dump().c_str());
    const auto resultType = reply_.resultValue_.type();
    if (resultType == nlohmann::detail::value_t::null) {
        return nullptr;
    } else if (resultType == nlohmann::detail::value_t::array) {
        ani_class arrayCls = nullptr;
        if (ANI_OK != env->FindClass(Builder::BuildClass({"escompat", "Array"}).Descriptor().c_str(), &arrayCls)) {
            HiLog::Error(LABEL, "%{public}s FindClass Array Failed", __func__);
        }
        ani_ref undefinedRef = nullptr;
        if (ANI_OK != env->GetUndefined(&undefinedRef)) {
            HiLog::Error(LABEL, "%{public}s GetUndefined Failed", __func__);
        }
        arkts::ani_signature::SignatureBuilder array_ctor{};
        array_ctor.AddInt();
        ani_method arrayCtor = findCtorMethod(env, arrayCls, array_ctor.BuildSignatureDescriptor().c_str());
        ani_object arrayObj;
        ani_size com_size = reply_.resultValue_.size();
        if (ANI_OK != env->Object_New(arrayCls, arrayCtor, &arrayObj, com_size)) {
            HiLog::Error(LABEL, "%{public}s Object New Array Failed", __func__);
            return reinterpret_cast<ani_ref>(arrayObj);
        }
        ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "Component"}).Descriptor().c_str());
        ani_method com_ctor;
        ani_object com_obj;
        if (cls != nullptr) {
            arkts::ani_signature::SignatureBuilder string_ctor{};
            string_ctor.AddClass({"std", "core", "String"});
            com_ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
        }
        if (cls == nullptr || com_ctor == nullptr) {
            return nullptr;
        }
        for (ani_size index = 0; index < reply_.resultValue_.size(); index++) {
            ani_ref item = UnmarshalObject(env, reply_.resultValue_.at(index));
            if (ANI_OK != env->Object_New(cls, com_ctor, &com_obj, reinterpret_cast<ani_object>(item))) {
                HiLog::Error(LABEL, "%{public}s component Object new failed !!!", __func__);
            }
            if (ANI_OK != env->Object_CallMethodByName_Void(arrayObj, "$_set", "ILstd/core/Object;:V", index, com_obj)) {
                HiLog::Error(LABEL, "%{public}s Object_CallMethodByName_Void set Failed", __func__);
                break;
            }
        }
        return reinterpret_cast<ani_ref>(arrayObj);
    } else {
        return UnmarshalObject(env, reply_.resultValue_);
    }
}

static ani_boolean ScheduleEstablishConnection(ani_env *env, ani_string connToken)
{
    auto token = aniStringToStdString(env, connToken);
    ani_vm *vm = nullptr;
    if (ANI_OK != env->GetVM(&vm)) {
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

static ani_int GetConnectionStat(ani_env *env)
{
    return g_apiTransactClient.GetConnectionStat();
}

static ani_string unwrapp(ani_env *env, ani_object object, const char *name)
{
    ani_ref it;
    if (ANI_OK != env->Object_GetFieldByName_Ref(object, name, &it)) {
        return nullptr;
    }
    return reinterpret_cast<ani_string>(it);
}

static json getPoint(ani_env *env, ani_object p)
{
    auto point = json();
    string list[] = {"x", "y", "displayId"};
    for (int index = 0; index < THREE; index++) {
        char *cstr = new char[list[index].length() + 1];
        strcpy(cstr, list[index].c_str());
        if (index == TWO) {
            ani_ref ref;
            if (env->Object_GetPropertyByName_Ref(p, cstr, &ref) != ANI_OK) {
                HiLog::Error(LABEL, "GetPropertyByName %{public}s fail", cstr);
                continue;
            }
            ani_boolean ret;
            env->Reference_IsUndefined(ref, &ret);
            if (ret == ANI_TRUE) {
                continue;
            }
        }
        ani_int value;
        compareAndReport(ANI_OK,
                         env->Object_GetPropertyByName_Int(p, cstr, &value),
                         "Object_GetField_Int Failed '" + std::string(cstr) + "'",
                         "Successful!!get int proprty");
        point[list[index]] = value;
    }
    return point;
}

static json getRect(ani_env *env, ani_object p)
{
    ani_boolean ret;
    auto rect = json();
    env->Reference_IsUndefined(reinterpret_cast<ani_ref>(p), &ret);
    if (ret == ANI_FALSE) {
        string list[] = {"left", "right", "top", "bottom"};
        for (int index = 0; index < 4; index++) {
            char *cstr = new char[list[index].length() + 1];
            strcpy(cstr, list[index].c_str());
            ani_int value;
            ani_status status = env->Object_GetPropertyByName_Int(p, cstr, &value);
            HiLog::Error(LABEL, "Object_GetPropertyByName_Int  status : %{public}d", status);
            compareAndReport(ANI_OK, status,
                             "Object_GetField_Int Failed '" + std::string(cstr) + "'",
                             "Successful!!get int proprty");
            rect[list[index]] = value;
        }
    } else {
        HiLog::Error(LABEL, "Rect Reference_IsUndefined");
    }
    return rect;
}

static ani_object newRect(ani_env *env, ani_object object, nlohmann::json num)
{
    ani_object rect_obj = {};
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "RectInner"}).Descriptor().c_str());
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
    string direct[] = {"left", "top", "right", "bottom"};
    for (int index = 0; index < 4; index++) {
        string tag = direct[index];
        char *setter_name = strdup((Builder::BuildSetterName(tag)).c_str());
        if (ANI_OK != env->Class_FindMethod(cls, setter_name, nullptr, &setter)) {
            HiLog::Error(LABEL, "Find Method <set>tag failed");
        }
        if (ANI_OK != env->Object_CallMethod_Void(rect_obj, setter, ani_int(num[tag]))) {
            HiLog::Error(LABEL, "call setter failed %{public}s", direct[index].c_str());
            return rect_obj;
        }
    }
    return rect_obj;
}

ani_object createInt(ani_env *env, ani_int displayId)
{
    static constexpr const char *className = "std.core.Int";
    ani_class IntCls {};
    env->FindClass(className, &IntCls);
    ani_method ctor {};
    env->Class_FindMethod(IntCls, "<ctor>", "i:", &ctor);
    ani_object obj {};
    if (env->Object_New(IntCls, ctor, &obj, displayId)!=ANI_OK) {
        ani_ref unRef;
        env->GetUndefined(&unRef);
        return reinterpret_cast<ani_object>(unRef);
    }
    return obj;
}

static ani_object newPoint(ani_env *env, ani_object obj, int x, int y, int displayId)
{
    ani_object point_obj = {};
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "PointInner"}).Descriptor().c_str());
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
    string direct[] = {"x", "y", "displayId"};
    int num[] = {x, y, displayId};
    for (int index = 0; index < THREE; index++)
    {
        if (index == TWO) {
            auto ret1 = env->Object_SetPropertyByName_Ref(point_obj, "displayId",
                reinterpret_cast<ani_ref>(createInt(env, ani_int(displayId))));
            if (ANI_OK != ret1) {
                HiLog::Error(LABEL, "Object_SetPropertyByName_Ref  failed, %{public}d", ret1);
            }
        } else {
            string tag = direct[index];
            char *method_name = strdup((Builder::BuildSetterName(tag)).c_str());
            if (ANI_OK != env->Class_FindMethod(cls, method_name, nullptr, &setter)) {
                HiLog::Error(LABEL, "Find Method <set>tag failed");
            }
            if (ANI_OK != env->Object_CallMethod_Void(point_obj, setter, ani_int(num[index]))) {
                HiLog::Error(LABEL, "call setter failed %{public}s", direct[index].c_str());
                return point_obj;
            }
        }
    }
    return point_obj;
}

static ani_ref createMatrix(ani_env *env, ani_object object, ani_int fingers, ani_int steps)
{
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "PointerMatrix"}).Descriptor().c_str());
    ani_ref nullref;
    env->GetNull(&nullref);
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
    } else {
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "PointerMatrix.create";
    callInfo_.paramList_.push_back(fingers);
    callInfo_.paramList_.push_back(steps);
    Transact(callInfo_, reply_);
    ani_ref nativePointerMatrix = UnmarshalReply(env, callInfo_, reply_);
    if (nativePointerMatrix == nullptr) {
        return nullref;
    }
    ani_object pointer_matrix_object;
    if (ANI_OK != env->Object_New(cls, ctor, &pointer_matrix_object, reinterpret_cast<ani_object>(nativePointerMatrix))) {
        HiLog::Error(LABEL, "New PointerMatrix Failed %{public}s", __func__);
    }
    return pointer_matrix_object;
}

static void setPoint(ani_env *env, ani_object object, ani_int finger, ani_int step, ani_object point)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "PointerMatrix.setPoint";
    callInfo_.paramList_.push_back(finger);
    callInfo_.paramList_.push_back(step);
    callInfo_.paramList_.push_back(getPoint(env, point));
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, object, "nativePointerMatrix"));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return;
}

static json getInputTextModeOptions(ani_env *env, ani_object f)
{
    auto options = json();
    string list[] = {"paste", "addition"};
    for (int i = 0; i < TWO; i++) {
        char *cstr = new char[list[i].length() + 1];
        strcpy(cstr, list[i].c_str());
        ani_ref value;
        if (env->Object_GetPropertyByName_Ref(f, cstr, &value) != ANI_OK) {
            HiLog::Error(LABEL, "GetPropertyByName %{public}s fail", cstr);
            continue;
        }
        ani_boolean ret;
        env->Reference_IsUndefined(value, &ret);
        if (ret == ANI_TRUE) {
            continue;
        }
        ani_boolean b;
        compareAndReport(ANI_OK,
                         env->Object_CallMethodByName_Boolean(static_cast<ani_object>(value), "unboxed", nullptr, &b),
                         "Object_CallMethodByName_Boolean Failed",
                         "get boolean value");
        HiLog::Info(LABEL, "%{public}d ani_boolean !!!", static_cast<int>(b));
        options[list[i]] = static_cast<bool>(b);
    }
    return options;
}

static ani_boolean BindPointMatrix(ani_env *env)
{
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "PointerMatrix"}).Descriptor().c_str());
    if (cls == nullptr) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    ani_native_function setPointMethod {"setPoint", nullptr, reinterpret_cast<void *>(setPoint)};
    if (ANI_OK != env->Class_BindNativeMethods(cls, &setPointMethod, 1)) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    ani_native_function createMethod {"create", nullptr, reinterpret_cast<void *>(createMatrix)};
    if (ANI_OK != env->Class_BindStaticNativeMethods(cls, &createMethod, 1)) {
        HiLog::Error(LABEL, "%{public}s Cannot bind static native methods to !!!", __func__);
        return false;
    }
    return true;
}

static ani_ref createOn(ani_env *env, ani_object object, nlohmann::json params, string apiId_)
{
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "On"}).Descriptor().c_str());
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
    }
    if (ctor == nullptr || cls == nullptr) {
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
    if (nativeOn == nullptr) {
        return nullptr;
    }
    ani_object on_object;
    if (ANI_OK != env->Object_New(cls, ctor, &on_object, reinterpret_cast<ani_ref>(nativeOn))) {
        HiLog::Error(LABEL, "%{public}s New ON failed !!!", __func__);
    }
    return on_object;
}

static ani_ref within(ani_env *env, ani_object obj, ani_object on)
{
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.within");
}

static ani_ref isBefore(ani_env *env, ani_object obj, ani_object on)
{
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.isBefore");
}

static ani_ref isAfter(ani_env *env, ani_object obj, ani_object on)
{
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.isAfter");
}
static ani_ref id(ani_env *env, ani_object obj, ani_string id, ani_enum_item pattern)
{
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, id));
    ani_int enumValue = getEnumValue(env, pattern);
    params.push_back(enumValue);
    return createOn(env, obj, params, "On.id");
}
static ani_ref text(ani_env *env, ani_object obj, ani_string text, ani_enum_item pattern)
{
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    ani_int enumValue = getEnumValue(env, pattern);
    params.push_back(enumValue);
    return createOn(env, obj, params, "On.text");
}
static ani_ref type(ani_env *env, ani_object obj, ani_string type, ani_enum_item pattern)
{
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, type));
    ani_int enumValue = getEnumValue(env, pattern);
    params.push_back(enumValue);
    return createOn(env, obj, params, "On.type");
}
static ani_ref hint(ani_env *env, ani_object obj, ani_string text, ani_enum_item pattern)
{
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    ani_int enumValue = getEnumValue(env, pattern);
    params.push_back(enumValue);
    return createOn(env, obj, params, "On.hint");
}
static ani_ref description(ani_env *env, ani_object obj, ani_string text, ani_enum_item pattern)
{
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    ani_int enumValue = getEnumValue(env, pattern);
    params.push_back(enumValue);
    return createOn(env, obj, params, "On.description");
}
static ani_ref inWindow(ani_env *env, ani_object obj, ani_string bundleName)
{
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, bundleName));
    return createOn(env, obj, params, "On.inWindow");
}
static void pushBool(ani_env *env, ani_object input, nlohmann::json &params)
{
    ani_boolean ret;
    env->Reference_IsUndefined(reinterpret_cast<ani_ref>(input), &ret);
    if (ret == ANI_FALSE) {
        ani_boolean param;
        HiLog::Info(LABEL, "%{public}s ani_boolean !!!", __func__);
        env->Object_CallMethodByName_Boolean(input, "unboxed", ":Z", &param);
        HiLog::Info(LABEL, "%{public}d ani_boolean !!!", static_cast<int>(param));
        params.push_back(static_cast<bool>(param));
    }
}
static ani_ref enabled(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.enabled");
}
static ani_ref focused(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.focused");
}
static ani_ref clickable(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.clickable");
}
static ani_ref checked(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.checked");
}
static ani_ref checkable(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.checkable");
}
static ani_ref longClickable(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.longClickable");
}
static ani_ref selected(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.selected");
}
static ani_ref scrollable(ani_env *env, ani_object obj, ani_object b)
{
    nlohmann::json params = nlohmann::json::array();
    HiLog::Info(LABEL, "%{public}s scrollable !!!", __func__);
    pushBool(env, b, params);
    return createOn(env, obj, params, "On.scrollable");
}

static ani_boolean BindOn(ani_env *env)
{
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "On"}).Descriptor().c_str());
    if (cls == nullptr) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function{"id", nullptr, reinterpret_cast<void *>(id)},
        ani_native_function{"text", nullptr, reinterpret_cast<void *>(text)},
        ani_native_function{"type", nullptr, reinterpret_cast<void *>(type)},
        ani_native_function{"hint", nullptr, reinterpret_cast<void *>(hint)},
        ani_native_function{"description", nullptr, reinterpret_cast<void *>(description)},
        ani_native_function{"inWindow", nullptr, reinterpret_cast<void *>(inWindow)},
        ani_native_function{"enabled", nullptr, reinterpret_cast<void *>(enabled)},
        ani_native_function{"within", nullptr, reinterpret_cast<void *>(within)},
        ani_native_function{"selected", nullptr, reinterpret_cast<void *>(selected)},
        ani_native_function{"focused", nullptr, reinterpret_cast<void *>(focused)},
        ani_native_function{"clickable", nullptr, reinterpret_cast<void *>(clickable)},
        ani_native_function{"checkable", nullptr, reinterpret_cast<void *>(checkable)},
        ani_native_function{"checked", nullptr, reinterpret_cast<void *>(checked)},
        ani_native_function{"scrollable", nullptr, reinterpret_cast<void *>(scrollable)},
        ani_native_function{"isBefore", nullptr, reinterpret_cast<void *>(isBefore)},
        ani_native_function{"isAfter", nullptr, reinterpret_cast<void *>(isAfter)},
        ani_native_function{"longClickable", nullptr, reinterpret_cast<void *>(longClickable)},
    };
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}

static ani_ref create([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_class clazz)
{
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (ANI_OK != env->FindClass(Builder::BuildClass({"@ohos", "UiTest", "Driver"}).Descriptor().c_str(), &cls)) {
        HiLog::Error(LABEL, "@ohos/uitest/Driver Not found !!!");
        return nullref;
    }
    ani_method ctor = nullptr;
    arkts::ani_signature::SignatureBuilder string_ctor{};
    string_ctor.AddClass({"std", "core", "String"});
    if (ANI_OK != env->Class_FindMethod(cls, Builder::BuildConstructorName().c_str(), string_ctor.BuildSignatureDescriptor().c_str(), &ctor)) {
        HiLog::Error(LABEL, "Driver Ctor Not found !!!");
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.create";
    Transact(callInfo_, reply_);
    ani_ref nativeDriver = UnmarshalReply(env, callInfo_, reply_);
    if (nativeDriver == nullptr) {
        return nullref;
    }
    ani_object driver_object;
    if (ANI_OK != env->Object_New(cls, ctor, &driver_object, reinterpret_cast<ani_object>(nativeDriver))) {
        HiLog::Error(LABEL, "%{public}s New Driver Failed!!!", __func__);
    }
    return driver_object;
}

static ani_boolean delayMsSync(ani_env *env, ani_object obj, ani_int t)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.delayMs";
    callInfo_.paramList_.push_back(t);
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref findComponentSync(ani_env *env, ani_object obj, ani_object on_obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.findComponent";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    if (nativeComponent == nullptr) {
        return nativeComponent;
    }
    ani_object com_obj;
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "Component"}).Descriptor().c_str());
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &com_obj, reinterpret_cast<ani_object>(nativeComponent))) {
        HiLog::Error(LABEL, "%{public}s New Component Failed !!!", __func__);
    }
    return com_obj;
}

static ani_object findComponentsSync(ani_env *env, ani_object obj, ani_object on_obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.findComponents";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    Transact(callInfo_, reply_);
    ani_object nativeComponents = reinterpret_cast<ani_object>(UnmarshalReply(env, callInfo_, reply_));
    return nativeComponents;
}

static ani_boolean assertComponentExistSync(ani_env *env, ani_object obj, ani_object on_obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.assertComponentExist";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static void performClick(ani_env *env, ani_object obj, ani_int x, ani_int y, string api)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = api;
    callInfo_.paramList_.push_back(x);
    callInfo_.paramList_.push_back(y);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean clickSync(ani_env *env, ani_object obj, ani_int x, ani_int y)
{
    string api_name = "Driver.click";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_boolean doubleClickSync(ani_env *env, ani_object obj, ani_int x, ani_int y)
{
    string api_name = "Driver.doubleClick";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_boolean longClickSync(ani_env *env, ani_object obj, ani_int x, ani_int y)
{
    string api_name = "Driver.longClick";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_ref performDriver(ani_env *env, ani_object obj, string api)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = api;
    Transact(callInfo_, reply_);
    return UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean pressBackSync(ani_env *env, ani_object obj)
{
    performDriver(env, obj, "Driver.pressBack");
    return true;
}

static ani_boolean pressBackWithDisplayIdSync(ani_env *env, ani_object obj, ani_int displayId)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.pressBack";
    callInfo_.paramList_.push_back(displayId);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean flingSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_int stepLen, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.fling";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    callInfo_.paramList_.push_back(stepLen);
    callInfo_.paramList_.push_back(speed);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean flingSyncDirection(ani_env *env, ani_object obj, ani_enum_item direction, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.fling";
    ani_int enumValue = getEnumValue(env, direction);
    callInfo_.paramList_.push_back(enumValue);
    callInfo_.paramList_.push_back(speed);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean flingWithDisplayIdSync(ani_env *env, ani_object obj, ani_enum_item direction, ani_int speed,
    ani_int displayId)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.fling";
    ani_int enumValue = getEnumValue(env, direction);
    callInfo_.paramList_.push_back(enumValue);
    callInfo_.paramList_.push_back(speed);
    callInfo_.paramList_.push_back(displayId);
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

static ani_boolean inputTextWithModeSync(ani_env *env, ani_object obj, ani_object p, ani_string text,
    ani_object inputMode)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.inputText";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(aniStringToStdString(env, text));
    callInfo_.paramList_.push_back(getInputTextModeOptions(env, inputMode));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean swipeSync(ani_env *env, ani_object obj, ani_int x1, ani_int y1, ani_int x2, ani_int y2, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.swipe";
    callInfo_.paramList_.push_back(x1);
    callInfo_.paramList_.push_back(y1);
    callInfo_.paramList_.push_back(x2);
    callInfo_.paramList_.push_back(y2);
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean dragSync(ani_env *env, ani_object obj, ani_int x1, ani_int y1, ani_int x2, ani_int y2, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.drag";
    callInfo_.paramList_.push_back(x1);
    callInfo_.paramList_.push_back(y1);
    callInfo_.paramList_.push_back(x2);
    callInfo_.paramList_.push_back(y2);
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static json getWindowFilter(ani_env *env, ani_object f)
{
    auto filter = json();
    string list[] = {"bundleName", "title", "focused", "active"};
    for (int i = 0; i < 4; i++) {
        char *cstr = new char[list[i].length() + 1];
        strcpy(cstr, list[i].c_str());
        ani_ref value;
        if (env->Object_GetPropertyByName_Ref(f, cstr, &value) != ANI_OK) {
            HiLog::Error(LABEL, "GetPropertyByName %{public}s fail", cstr);
            continue;
        }
        ani_boolean ret;
        env->Reference_IsUndefined(value, &ret);
        if (ret == ANI_TRUE) {
            continue;
        }
        if (i < 2) {
            filter[list[i]] = aniStringToStdString(env, reinterpret_cast<ani_string>(value));
        } else {
            ani_boolean b;
            compareAndReport(ANI_OK,
                             env->Object_CallMethodByName_Boolean(static_cast<ani_object>(value), "unboxed", nullptr, &b),
                             "CallMethodByName_Boolean Failed",
                             "get boolean value");
            HiLog::Info(LABEL, "%{public}d ani_boolean !!!", static_cast<int>(b));
            filter[list[i]] = static_cast<bool>(b);
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
    if (nativeWindow == nullptr) {
        return reinterpret_cast<ani_object>(nativeWindow);
    }
    ani_object window_obj;
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "UiWindow"}).Descriptor().c_str());
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
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
    if (nativeUIEventObserver == nullptr) {
        return nullptr;
    }
    ani_object observer_obj;
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "UIEventObserver"}).Descriptor().c_str());
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
    }
    if (cls == nullptr || ctor == nullptr) {
        HiLog::Error(LABEL, "Not found className/ctor: %{public}s", "UIEventObserver");
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
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, pointers, "nativePointerMatrix")));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean triggerKeySync(ani_env *env, ani_object obj, ani_int keyCode)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerKey";
    callInfo_.paramList_.push_back(keyCode);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean triggerKeyWithDisplayIdSync(ani_env *env, ani_object obj, ani_int keyCode, ani_int displayId)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerKey";
    callInfo_.paramList_.push_back(keyCode);
    callInfo_.paramList_.push_back(displayId);
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

static ani_boolean pressHomeWithDisplayIdSync(ani_env *env, ani_object obj, ani_int displayId)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.pressHome";
    callInfo_.paramList_.push_back(displayId);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref getDisplaySizeSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplaySize";
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return nullptr;
    }
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"],
        reply_.resultValue_["displayId"]);
    return p;
}

static ani_ref getDisplaySizeWithDisplayIdSync(ani_env *env, ani_object obj, ani_int displayId)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplaySize";
    callInfo_.paramList_.push_back(displayId);
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return nullptr;
    }
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"],
        reply_.resultValue_["displayId"]);
    return p;
}

static ani_object getDisplayDensitySync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplayDensity";
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return nullptr;
    }
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"],
        reply_.resultValue_["displayId"]);
    return p;
}

static ani_object getDisplayRotationSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplayRotation";
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return nullptr;
    }
    ani_enum enumType;
    if (ANI_OK != env->FindEnum(Builder::BuildEnum({"@ohos", "UiTest", "DisplayRotation"}).Descriptor().c_str(), &enumType)) {
        HiLog::Error(LABEL, "Find Enum Faild: %{public}s", __func__);
    }
    ani_enum_item enumItem;
    auto index = static_cast<uint8_t>(reply_.resultValue_.get<int>());
    ani_status status = env->Enum_GetEnumItemByIndex(enumType, index, &enumItem);
    HiLog::Info(LABEL, "Enum_GetEnumItemByIndex status : %{public}d", status);
    return enumItem;
}

static ani_boolean waitForIdleSync(ani_env *env, ani_object obj, ani_int idleTime, ani_int timeout)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.waitForIdle";
    callInfo_.paramList_.push_back(idleTime);
    callInfo_.paramList_.push_back(timeout);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_object waitForComponentSync(ani_env *env, ani_object obj, ani_object on_obj, ani_int time)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.waitForComponent";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    callInfo_.paramList_.push_back(time);
    Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    if (nativeComponent == nullptr) {
        return reinterpret_cast<ani_object>(nativeComponent);
    }
    ani_object component_obj;
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "Component"}).Descriptor().c_str());
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &component_obj, reinterpret_cast<ani_object>(nativeComponent))) {
        HiLog::Error(LABEL, "New Component fail: %{public}s", __func__);
    }
    return component_obj;
}

static ani_boolean triggerCombineKeysSync(ani_env *env, ani_object obj, ani_int key0, ani_int key1, ani_object key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerCombineKeys";
    callInfo_.paramList_.push_back(key0);
    callInfo_.paramList_.push_back(key1);
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
    ani_int enumValue = getEnumValue(env, rotation);
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
    HiLog::Info(LABEL, "savePath: %{public}s", savePath.c_str());
    auto fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false;
    }
    HiLog::Info(LABEL, "savePath: %{public}d", fd);
    callInfo_.paramList_[INDEX_ZERO] = fd;
    callInfo_.paramList_[INDEX_ONE] = getRect(env, rect);
    callInfo_.fdParamIndex_ = INDEX_ZERO;
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return false;
    }
    return reply_.resultValue_.get<bool>();
}

static ani_boolean screenCapSync(ani_env *env, ani_object obj, ani_string path)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.screenCap";
    string savePath = aniStringToStdString(env, path);
    HiLog::Info(LABEL, "savePath: %{public}s", savePath.c_str());
    int32_t fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false;
    }
    HiLog::Info(LABEL, "savePath: %{public}d", fd);
    callInfo_.paramList_[INDEX_ZERO] = fd;
    callInfo_.fdParamIndex_ = INDEX_ZERO;
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return false;
    }
    return reply_.resultValue_.get<bool>();
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

static ani_boolean mouseScrollSync(ani_env *env, ani_object obj, ani_object p, ani_boolean down, ani_int dis, ani_object key1, ani_object key2,
                                   ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseScroll";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(static_cast<bool>(down));
    callInfo_.paramList_.push_back(dis);
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

static ani_boolean mouseDragSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseDrag";
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
    callInfo_.apiId_ = "Driver.mouseClick";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    ani_int enumValue = getEnumValue(env, btnId);
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
    ani_int enumValue = getEnumValue(env, btnId);
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
    ani_int enumValue = getEnumValue(env, btnId);
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
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, pointers, "nativePointerMatrix")));
    pushParam(env, speed, callInfo_, true);
    pushParam(env, pressure, callInfo_, false);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static json getTouchPadSwipeOptions(ani_env *env, ani_object f)
{
    auto options = json();
    string list[] = {"stay", "speed"};
    for (int i = 0; i < TWO; i++) {
        char *cstr = new char[list[i].length() + 1];
        strcpy(cstr, list[i].c_str());
        ani_ref value;
        if (env->Object_GetPropertyByName_Ref(f, cstr, &value) != ANI_OK) {
            HiLog::Error(LABEL, "GetPropertyByName %{public}s fail", cstr);
            continue;
        }
        ani_boolean ret;
        env->Reference_IsUndefined(value, &ret);
        if (ret == ANI_TRUE) {
            continue;
        }
        if (i == ONE) {
            ani_int speed;
            compareAndReport(ANI_OK,
                             env->Object_CallMethodByName_Int(static_cast<ani_object>(value), "unboxed", nullptr, &speed),
                             "Object_CallMethodByName_Boolean Failed",
                             "get boolean value");
            options[list[i]] = speed;
        } else {
            ani_boolean b;
            compareAndReport(ANI_OK,
                             env->Object_CallMethodByName_Boolean(static_cast<ani_object>(value), "unboxed", nullptr, &b),
                             "Object_CallMethodByName_Boolean Failed",
                             "get boolean value");
            HiLog::Info(LABEL, "%{public}d ani_boolean !!!", static_cast<int>(b));
            options[list[i]] = static_cast<bool>(b);
        }
    }
    return options;
}

static ani_boolean touchPadMultiFingerSwipeSync(ani_env *env, ani_object obj, ani_int fingers, ani_enum_item direction, ani_object touchPadOpt)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.touchPadMultiFingerSwipe";
    callInfo_.paramList_.push_back(fingers);
    ani_int enumValue = getEnumValue(env, direction);
    callInfo_.paramList_.push_back(enumValue);
    ani_boolean ret;
    env->Reference_IsUndefined(touchPadOpt, &ret);
    if (ret == ANI_FALSE) {
        callInfo_.paramList_.push_back(getTouchPadSwipeOptions(env, touchPadOpt));
    }
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean kunckleClickOnePointSync(ani_env *env, ani_object obj, ani_object p, ani_int times)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.knuckleKnock";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(times);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean kunckleClickTwoPointSync(ani_env *env, ani_object obj, ani_object p0, ani_object p1, ani_int times)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.knuckleKnock";
    auto point0 = getPoint(env, p0);
    auto point1 = getPoint(env, p1);
    callInfo_.paramList_.push_back(point0);
    callInfo_.paramList_.push_back(point1);
    callInfo_.paramList_.push_back(times);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean injectKnucklePointerActionSync(ani_env *env, ani_object obj, ani_object pointers, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.injectKnucklePointerAction";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, pointers, "nativePointerMatrix")));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean BindDriver(ani_env *env)
{
    ani_class cls;
    if (ANI_OK != env->FindClass(Builder::BuildClass({"@ohos", "UiTest", "Driver"}).Descriptor().c_str(), &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found !!!", __func__);
        return false;
    }

    std::array methods = {
        ani_native_function{"delayMsSync", nullptr, reinterpret_cast<void *>(delayMsSync)},
        ani_native_function{"clickSync", nullptr, reinterpret_cast<void *>(clickSync)},
        ani_native_function{"longClickSync", nullptr, reinterpret_cast<void *>(longClickSync)},
        ani_native_function{"doubleClickSync", nullptr, reinterpret_cast<void *>(doubleClickSync)},
        ani_native_function{"flingSync", nullptr, reinterpret_cast<void *>(flingSync)},
        ani_native_function{"flingSyncDirection", nullptr, reinterpret_cast<void *>(flingSyncDirection)},
        ani_native_function{"flingWithDisplayIdSync", nullptr, reinterpret_cast<void *>(flingWithDisplayIdSync)},
        ani_native_function{"swipeSync", nullptr, reinterpret_cast<void *>(swipeSync)},
        ani_native_function{"dragSync", nullptr, reinterpret_cast<void *>(dragSync)},
        ani_native_function{"pressBackSync", nullptr, reinterpret_cast<void *>(pressBackSync)},
        ani_native_function{"pressBackWithDisplayIdSync", nullptr,
                            reinterpret_cast<void *>(pressBackWithDisplayIdSync)},
        ani_native_function{"assertComponentExistSync", nullptr, reinterpret_cast<void *>(assertComponentExistSync)},
        ani_native_function{"triggerKeySync", nullptr, reinterpret_cast<void *>(triggerKeySync)},
        ani_native_function{"triggerKeyWithDisplayIdSync", nullptr,
                            reinterpret_cast<void *>(triggerKeyWithDisplayIdSync)},
        ani_native_function{"inputTextSync", nullptr, reinterpret_cast<void *>(inputTextSync)},
        ani_native_function{"inputTextWithModeSync", nullptr, reinterpret_cast<void *>(inputTextWithModeSync)},
        ani_native_function{"findWindowSync", nullptr, reinterpret_cast<void *>(findWindowSync)},
        ani_native_function{"createUIEventObserver", nullptr,
                            reinterpret_cast<void *>(createUIEventObserverSync)},
        ani_native_function{"wakeUpDisplaySync", nullptr, reinterpret_cast<void *>(wakeUpDisplaySync)},
        ani_native_function{"pressHomeSync", nullptr, reinterpret_cast<void *>(pressHomeSync)},
        ani_native_function{"pressHomeWithDisplayIdSync", nullptr,
                            reinterpret_cast<void *>(pressHomeWithDisplayIdSync)},
        ani_native_function{"getDisplaySizeSync", nullptr, reinterpret_cast<void *>(getDisplaySizeSync)},
        ani_native_function{"getDisplaySizeWithDisplayIdSync", nullptr,
                            reinterpret_cast<void *>(getDisplaySizeWithDisplayIdSync)},
        ani_native_function{"getDisplayDensitySync", nullptr, reinterpret_cast<void *>(getDisplayDensitySync)},
        ani_native_function{"getDisplayRotationSync", nullptr, reinterpret_cast<void *>(getDisplayRotationSync)},
        ani_native_function{"findComponentsSync", nullptr, reinterpret_cast<void *>(findComponentsSync)},
        ani_native_function{"findComponentSync", nullptr, reinterpret_cast<void *>(findComponentSync)},
        ani_native_function{"waitForIdleSync", nullptr, reinterpret_cast<void *>(waitForIdleSync)},
        ani_native_function{"waitForComponentSync", nullptr, reinterpret_cast<void *>(waitForComponentSync)},
        ani_native_function{"triggerCombineKeysSync", nullptr, reinterpret_cast<void *>(triggerCombineKeysSync)},
        ani_native_function{"setDisplayRotationEnabledSync", nullptr, reinterpret_cast<void *>(setDisplayRotationEnabledSync)},
        ani_native_function{"setDisplayRotationSync", nullptr, reinterpret_cast<void *>(setDisplayRotationSync)},
        ani_native_function{"screenCaptureSync", nullptr, reinterpret_cast<void *>(screenCaptureSync)},
        ani_native_function{"screenCapSync", nullptr, reinterpret_cast<void *>(screenCapSync)},
        ani_native_function{"penSwipeSync", nullptr, reinterpret_cast<void *>(penSwipeSync)},
        ani_native_function{"penClickSync", nullptr, reinterpret_cast<void *>(penClickSync)},
        ani_native_function{"penDoubleClickSync", nullptr, reinterpret_cast<void *>(penDoubleClickSync)},
        ani_native_function{"penLongClickSync", "L@ohos/UiTest/Point;Lstd/core/Double;:Z", reinterpret_cast<void *>(penLongClickSync)},
        ani_native_function{"mouseScrollSync", nullptr, reinterpret_cast<void *>(mouseScrollSync)},
        ani_native_function{"mouseMoveWithTrackSync", nullptr, reinterpret_cast<void *>(mouseMoveWithTrackSync)},
        ani_native_function{"mouseMoveToSync", nullptr, reinterpret_cast<void *>(mouseMoveToSync)},
        ani_native_function{"mouseDragSync", nullptr, reinterpret_cast<void *>(mouseDragSync)},
        ani_native_function{"mouseClickSync", nullptr, reinterpret_cast<void *>(mouseClickSync)},
        ani_native_function{"mouseDoubleClickSync", nullptr, reinterpret_cast<void *>(mouseDoubleClickSync)},
        ani_native_function{"mouseLongClickSync", nullptr, reinterpret_cast<void *>(mouseLongClickSync)},
        ani_native_function{"injectMultiPointerActionSync", nullptr, reinterpret_cast<void *>(injectMultiPointerActionSync)},
        ani_native_function{"injectPenPointerActionSync", nullptr, reinterpret_cast<void *>(injectPenPointerActionSync)},
        ani_native_function{"touchPadMultiFingerSwipeSync", nullptr, reinterpret_cast<void *>(touchPadMultiFingerSwipeSync)},
        ani_native_function{"kunckleClickOnePointSync", nullptr, reinterpret_cast<void *>(kunckleClickOnePointSync)},
        ani_native_function{"kunckleClickTwoPointSync", nullptr, reinterpret_cast<void *>(kunckleClickTwoPointSync)},
        ani_native_function{"injectKnucklePointerActionSync", nullptr, reinterpret_cast<void *>(injectKnucklePointerActionSync)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    ani_native_function createMethod {"create", ":L@ohos/UiTest/Driver;", reinterpret_cast<void *>(create)};
    if (ANI_OK != env->Class_BindStaticNativeMethods(cls, &createMethod, 1)) {
        HiLog::Error(LABEL, "%{public}s Cannot bind static native methods to !!!", __func__);
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

static ani_boolean isFocusedSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.isFocused";
    Transact(callInfo_, reply_);
    ani_ref ret = UnmarshalReply(env, callInfo_, reply_);
    if (ret == nullptr) {
        return false;
    }
    return reply_.resultValue_.get<bool>();
}

static ani_boolean isActiveSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.isActive";
    Transact(callInfo_, reply_);
    ani_ref ret = UnmarshalReply(env, callInfo_, reply_);
    if (ret == nullptr) {
        return false;
    }
    return reply_.resultValue_.get<bool>();
}

static ani_boolean resizeSync(ani_env *env, ani_object obj, ani_int w, ani_int h, ani_enum_item d)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.resize";
    callInfo_.paramList_.push_back(w);
    callInfo_.paramList_.push_back(h);
    ani_int enumValue = getEnumValue(env, d);
    callInfo_.paramList_.push_back(enumValue);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean moveToSync(ani_env *env, ani_object obj, ani_int x, ani_int y)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.moveTo";
    callInfo_.paramList_.push_back(x);
    callInfo_.paramList_.push_back(y);
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
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        ani_ref nullref;
        env->GetNull(&nullref);
        return nullref;
    }
    ani_enum enumType;
    if (ANI_OK != env->FindEnum(Builder::BuildEnum({"@ohos", "UiTest", "WindowMode"}).Descriptor().c_str(), &enumType)) {
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
    ani_ref ret = UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref getTitleSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getTitle";
    Transact(callInfo_, reply_);
    ani_ref ret = UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref getBoundsSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getBounds";
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        ani_ref nullref;
        env->GetNull(&nullref);
        return nullref;
    }
    ani_object r = newRect(env, obj, reply_.resultValue_);
    return r;
}

static ani_boolean BindWindow(ani_env *env)
{
    ani_class cls;
    if (ANI_OK != env->FindClass(Builder::BuildClass({"@ohos", "UiTest", "UiWindow"}).Descriptor().c_str(), &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }

    std::array methods = {
        ani_native_function{"splitSync", nullptr, reinterpret_cast<void *>(splitSync)},
        ani_native_function{"resumeSync", nullptr, reinterpret_cast<void *>(resumeSync)},
        ani_native_function{"closeSync", nullptr, reinterpret_cast<void *>(closeSync)},
        ani_native_function{"minimizeSync", nullptr, reinterpret_cast<void *>(minimizeSync)},
        ani_native_function{"maximizeSync", nullptr, reinterpret_cast<void *>(maximizeSync)},
        ani_native_function{"focusSync", nullptr, reinterpret_cast<void *>(focusSync)},
        ani_native_function{"isFocusedSync", nullptr, reinterpret_cast<void *>(isFocusedSync)},
        ani_native_function{"isActiveSync", nullptr, reinterpret_cast<void *>(isActiveSync)},
        ani_native_function{"resizeSync", nullptr, reinterpret_cast<void *>(resizeSync)},
        ani_native_function{"moveToSync", nullptr, reinterpret_cast<void *>(moveToSync)},
        ani_native_function{"getWindowModeSync", nullptr, reinterpret_cast<void *>(getWindowModeSync)},
        ani_native_function{"getBundleNameSync", nullptr, reinterpret_cast<void *>(getBundleNameSync)},
        ani_native_function{"getTitleSync", nullptr, reinterpret_cast<void *>(getTitleSync)},
        ani_native_function{"winGetBoundsSync", nullptr, reinterpret_cast<void *>(getBoundsSync)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}

static ani_ref getBoundsCenterSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.apiId_ = "Component.getBoundsCenter";
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        ani_ref nullref;
        env->GetNull(&nullref);
        return nullref;
    }
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"],
        reply_.resultValue_["displayId"]);
    HiLog::Info(LABEL, "reply_.resultValue_[x]:%{public}s", reply_.resultValue_["x"].dump().c_str());
    HiLog::Info(LABEL, "reply_.resultValue_[y]:%{public}s", reply_.resultValue_["y"].dump().c_str());
    return p;
}
static ani_ref comGetBounds(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.apiId_ = "Component.getBounds";
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        ani_ref nullref;
        env->GetNull(&nullref);
        return nullref;
    }
    ani_object r = newRect(env, obj, reply_.resultValue_);
    return r;
}

static ani_ref performComponentApi(ani_env *env, ani_object obj, string apiId_)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    return result;
}

static ani_boolean comClick(ani_env *env, ani_object obj)
{
    performComponentApi(env, obj, "Component.click");
    return true;
}

static ani_boolean comDoubleClick(ani_env *env, ani_object obj)
{
    performComponentApi(env, obj, "Component.doubleClick");
    return true;
}

static ani_boolean comLongClick(ani_env *env, ani_object obj)
{
    performComponentApi(env, obj, "Component.longClick");
    return true;
}

static ani_boolean comDragToSync(ani_env *env, ani_object obj, ani_object target)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.dragTo";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, target, "nativeComponent")));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref getText(ani_env *env, ani_object obj)
{
    return performComponentApi(env, obj, "Component.getText");
}

static ani_ref getType(ani_env *env, ani_object obj)
{
    return performComponentApi(env, obj, "Component.getType");
}

static ani_ref getId(ani_env *env, ani_object obj)
{
    return performComponentApi(env, obj, "Component.getId");
}

static ani_ref getHint(ani_env *env, ani_object obj)
{
    return performComponentApi(env, obj, "Component.getHint");
}

static ani_ref getDescription(ani_env *env, ani_object obj)
{
    return performComponentApi(env, obj, "Component.getDescription");
}

static ani_boolean comInputText(ani_env *env, ani_object obj, ani_string txt)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.inputText";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, txt));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean comInputTextWithMode(ani_env *env, ani_object obj, ani_string txt, ani_object inputMode)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.inputText";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, txt));
    callInfo_.paramList_.push_back(getInputTextModeOptions(env, inputMode));
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean clearText(ani_env *env, ani_object obj)
{
    performComponentApi(env, obj, "Component.clearText");
    return true;
}

static ani_boolean scrollToTop(ani_env *env, ani_object obj, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollToTop";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean scrollToBottom(ani_env *env, ani_object obj, ani_object speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollToBottom";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    pushParam(env, speed, callInfo_, true);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_object scrollSearch(ani_env *env, ani_object obj, ani_object on, ani_object vertical, ani_object offset)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollSearch";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on, "nativeOn")));
    pushBool(env, vertical, callInfo_.paramList_);
    pushParam(env, offset, callInfo_, true);
    Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    if (nativeComponent == nullptr) {
        return nullptr;
    }
    ani_object com_obj;
    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "Component"}).Descriptor().c_str());
    ani_method ctor = nullptr;
    if (cls != nullptr) {
        arkts::ani_signature::SignatureBuilder string_ctor{};
        string_ctor.AddClass({"std", "core", "String"});
        ctor = findCtorMethod(env, cls, string_ctor.BuildSignatureDescriptor().c_str());
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &com_obj, reinterpret_cast<ani_object>(nativeComponent))) {
        HiLog::Error(LABEL, "%{public}s New Component Failed !!!", __func__);
    }
    return com_obj;
}

static void pinch(ani_env *env, ani_object obj, ani_double scale, string apiId_)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(scale);
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return;
}

static ani_boolean pinchIn(ani_env *env, ani_object obj, ani_double scale)
{
    pinch(env, obj, scale, "Component.pinchIn");
    return true;
}

static ani_boolean pinchOut(ani_env *env, ani_object obj, ani_double scale)
{
    pinch(env, obj, scale, "Component.pinchOut");
    return true;
}
static ani_boolean performComponentApiBool(ani_env *env, ani_object obj, string apiId_)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        return false;
    }
    return reply_.resultValue_.get<bool>();
}
static ani_boolean isSelected(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isSelected");
}

static ani_boolean isClickable(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isClickable");
}

static ani_boolean isLongClickable(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isLongClickable");
}

static ani_boolean isScrollable(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isScrollable");
}

static ani_boolean isEnabled(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isEnabled");
}

static ani_boolean isFocused(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isFocused");
}

static ani_boolean isChecked(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isChecked");
}

static ani_boolean isCheckable(ani_env *env, ani_object obj)
{
    return performComponentApiBool(env, obj, "Component.isCheckable");
}

static ani_boolean BindComponent(ani_env *env)
{
    ani_class cls;
    if (ANI_OK != env->FindClass(Builder::BuildClass({"@ohos", "UiTest", "Component"}).Descriptor().c_str(), &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function{"comClickSync", nullptr, reinterpret_cast<void *>(comClick)},
        ani_native_function{"comLongClickSync", nullptr, reinterpret_cast<void *>(comLongClick)},
        ani_native_function{"comDoubleClickSync", nullptr, reinterpret_cast<void *>(comDoubleClick)},
        ani_native_function{"comDragToSync", nullptr, reinterpret_cast<void *>(comDragToSync)},
        ani_native_function{"comGetBoundsSync", nullptr, reinterpret_cast<void *>(comGetBounds)},
        ani_native_function{"getBoundsCenterSync", nullptr, reinterpret_cast<void *>(getBoundsCenterSync)},
        ani_native_function{"getTextSync", nullptr, reinterpret_cast<void *>(getText)},
        ani_native_function{"getTypeSync", nullptr, reinterpret_cast<void *>(getType)},
        ani_native_function{"getIdSync", nullptr, reinterpret_cast<void *>(getId)},
        ani_native_function{"getHintSync", nullptr, reinterpret_cast<void *>(getHint)},
        ani_native_function{"getDescriptionSync", nullptr, reinterpret_cast<void *>(getDescription)},
        ani_native_function{"comInputTextSync", nullptr, reinterpret_cast<void *>(comInputText)},
        ani_native_function{"comInputTextWithModeSync", nullptr, reinterpret_cast<void *>(comInputTextWithMode)},
        ani_native_function{"clearTextSync", nullptr, reinterpret_cast<void *>(clearText)},
        ani_native_function{"scrollToTopSync", nullptr, reinterpret_cast<void *>(scrollToTop)},
        ani_native_function{"scrollToBottomSync", nullptr, reinterpret_cast<void *>(scrollToBottom)},
        ani_native_function{"scrollSearchSync", nullptr, reinterpret_cast<void *>(scrollSearch)},
        ani_native_function{"pinchInSync", nullptr, reinterpret_cast<void *>(pinchIn)},
        ani_native_function{"pinchOutSync", nullptr, reinterpret_cast<void *>(pinchOut)},
        ani_native_function{"isScrollableSync", nullptr, reinterpret_cast<void *>(isScrollable)},
        ani_native_function{"isSelectedSync", nullptr, reinterpret_cast<void *>(isSelected)},
        ani_native_function{"isLongClickableSync", nullptr, reinterpret_cast<void *>(isLongClickable)},
        ani_native_function{"isClickableSync", nullptr, reinterpret_cast<void *>(isClickable)},
        ani_native_function{"isFocusedSync", nullptr, reinterpret_cast<void *>(isFocused)},
        ani_native_function{"isEnabledSync", nullptr, reinterpret_cast<void *>(isEnabled)},
        ani_native_function{"isCheckedSync", nullptr, reinterpret_cast<void *>(isChecked)},
        ani_native_function{"isCheckableSync", nullptr, reinterpret_cast<void *>(isCheckable)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods to !!!", __func__);
        return false;
    }
    return true;
}
static void once(ani_env *env, ani_object obj, ani_string type, ani_object callback)
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
    ani_class cls;
    if (ANI_OK != env->FindClass(Builder::BuildClass({"@ohos", "UiTest", "UIEventObserver"}).Descriptor().c_str(), &cls)) {
        HiLog::Error(LABEL, "%{public}s Not found className !!!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function{"once", nullptr, reinterpret_cast<void *>(once)},
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
    status = env->FindNamespace(Builder::BuildNamespace({"@ohos", "UiTest", "UiTest"}).Descriptor().c_str(), &ns);
    if (status != ANI_OK) {
        HiLog::Error(LABEL, "FindNamespace UiTest failed status : %{public}d", status);
        return;
    }
    std::array kitFunctions = {
        ani_native_function{"ScheduleEstablishConnection", nullptr, reinterpret_cast<void *>(ScheduleEstablishConnection)},
        ani_native_function{"GetConnectionStat", nullptr, reinterpret_cast<void *>(GetConnectionStat)},
    };
    status = env->Namespace_BindNativeFunctions(ns, kitFunctions.data(), kitFunctions.size());
    if (status != ANI_OK) {
        HiLog::Error(LABEL, "Namespace_BindNativeFunctions failed status : %{public}d", status);
    }
    if (env->ResetError() != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s ResetError failed", __func__);
    }
    HiLog::Info(LABEL, "%{public}s StsUiTestInit end", __func__);
}
ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
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
