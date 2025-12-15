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
#include "common_utils.h"
#include "frontend_api_handler.h"
#include "api_caller_client.h"
#include "error_handler.h"
#include "callback_code_ani.h"
#include <cstring>
#include <cstdio>

using namespace OHOS::perftest;
using namespace nlohmann;
using namespace std;
using namespace arkts::ani_signature;
static ApiCallerClient g_apiCallerClient;
static future<void> g_establishConnectionFuture;

template <typename T>
void CompareAndReport(const T &param1, const T &param2, const std::string &errorMessage, const std::string &message)
{
    if (param1 != param2) {
        HiLog::Error(LABEL, "CompareAndReport: %{public}s, ret: %{public}d", errorMessage.c_str(), param1);
    } else {
        HiLog::Info(LABEL, "CompareAndReport: %{public}s", message.c_str());
    }
}

static ani_class FindCls(ani_env *env, const char *className)
{
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (env->FindClass(className, &cls) != ANI_OK) {
        HiLog::Error(LABEL, "Can not find className: %{public}s", className);
    }
    return cls;
}

static ani_enum_item FindEnumItem(ani_env *env, const char *enumName, const ani_int enumIndex)
{
    ani_enum enumType;
    ani_enum_item enumItem {};
    if (env->FindEnum(enumName, &enumType) != ANI_OK) {
        HiLog::Error(LABEL, "Find enum failed: %{public}s", __func__);
        return enumItem;
    }
    if (env->Enum_GetEnumItemByIndex(enumType, enumIndex, &enumItem) != ANI_OK) {
        HiLog::Error(LABEL, "Find enumitem failed: %{public}s", __func__);
    }
    return enumItem;
}

static ani_method FindCtorMethod(ani_env *env, ani_class cls, const char *name)
{
    ani_method ctor = nullptr;
    if (cls == nullptr) {
        return ctor;
    }
    if (env->Class_FindMethod(cls, Builder::BuildConstructorName().c_str(), name, &ctor) != ANI_OK) {
        HiLog::Error(LABEL, "Can not find ctor: %{public}s", name);
    }
    return ctor;
}

static string AniStringToStdString(ani_env *env, ani_string stringObject)
{
    ani_size stringSize;
    env->String_GetUTF8Size(stringObject, &stringSize);
    string result(stringSize + 1, 0);
    env->String_GetUTF8(stringObject, result.data(), result.size(), &stringSize);
    result.resize(stringSize);
    return result;
}

static ani_object CreateDouble(ani_env *env, ani_double number)
{
    static constexpr const char *className = "std.core.Double";
    ani_class doubleCls {};
    env->FindClass(className, &doubleCls);
    ani_method ctor {};
    env->Class_FindMethod(doubleCls, "<ctor>", "d:", &ctor);
    ani_object obj {};
    if (env->Object_New(doubleCls, ctor, &obj, number) != ANI_OK) {
        ani_ref undefinedRef;
        env->GetUndefined(&undefinedRef);
        return static_cast<ani_object>(undefinedRef);
    }
    return obj;
}

static void WaitForConnectionIfNeed()
{
    if (g_establishConnectionFuture.valid()) {
        HiLog::Error(LABEL, "%{public}s. Begin Wait for Connection", __func__);
        g_establishConnectionFuture.get();
    }
}

static void Transact(ApiCallInfo callInfo_, ApiReplyInfo &reply_)
{
    WaitForConnectionIfNeed();
    g_apiCallerClient.Transact(callInfo_, reply_);
}

static bool HasExceptionReply(ani_env *env, const ApiReplyInfo &reply)
{
    auto &message = reply.exception_.message_;
    ErrCode code = reply.exception_.code_;
    if (reply.exception_.code_ != NO_ERROR) {
        HiLog::Error(LABEL, "ERRORINFO: code=%{public}u, message=%{public}s", code, message.c_str());
        ErrorHandler::Throw(env, code, message);
        return true;
    }
    return false;
}

static ani_ref UnmarshalObject(ani_env *env, nlohmann::json resultValue)
{
    const auto resultType = resultValue.type();
    ani_ref result = nullptr;
    if (resultType == nlohmann::detail::value_t::null) {
        return result;
    } else if (resultType != nlohmann::detail::value_t::string) {
        ani_string str;
        env->String_NewUTF8(resultValue.dump().c_str(), resultValue.dump().size(), &str);
        result = static_cast<ani_ref>(str);
        return result;
    }
    const auto cppString = resultValue.get<string>();
    ani_string str;
    env->String_NewUTF8(cppString.c_str(), cppString.length(), &str);
    result = static_cast<ani_ref>(str);
    ani_size size;
    env->String_GetUTF16Size(str, &size);
    return result;
}

static ani_ref UnmarshalReply(ani_env *env, const ApiCallInfo callInfo_, const ApiReplyInfo &reply_)
{
    HiLog::Info(LABEL, "%{public}s. Start to UnmarshalReply: %{public}s", __func__, reply_.resultValue_.dump().c_str());
    if (HasExceptionReply(env, reply_)) {
        return nullptr;
    }
    const auto resultType = reply_.resultValue_.type();
    if (resultType == nlohmann::detail::value_t::null) {
        return nullptr;
    } else {
        return UnmarshalObject(env, reply_.resultValue_);
    }
}

static ani_boolean ScheduleEstablishConnection(ani_env *env, ani_string connToken)
{
    auto token = AniStringToStdString(env, connToken);
    ani_vm *vm = nullptr;
    if (env->GetVM(&vm) != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s GetVM failed", __func__);
    }
    bool result = false;
    g_establishConnectionFuture = async(launch::async, [vm, token, &result]() {
        using namespace std::placeholders;
        auto &instance = CallbackCodeAni::Get();
        auto callbackHandler = std::bind(&CallbackCodeAni::HandleCallbackEvent, &instance, vm, _1, _2);
        result = g_apiCallerClient.InitAndConnectPeer(token, callbackHandler);
        HiLog::Info(LABEL, "End setup transaction connection, result=%{public}d", result);
    });
    return result;
}

static ani_int GetConnectionStat(ani_env *env)
{
    return g_apiCallerClient.GetConnectionStat();
}

static ani_string Unwrapp(ani_env *env, ani_object object, const char *name)
{
    ani_ref it;
    if (env->Object_GetFieldByName_Ref(object, name, &it) != ANI_OK) {
        return nullptr;
    }
    return static_cast<ani_string>(it);
}

static ani_int GetEnumValue(ani_env *env, ani_enum_item item)
{
    ani_boolean ret;
    env->Reference_IsUndefined(static_cast<ani_ref>(item), &ret);
    ani_int enumValue = 0;
    if (ret == ANI_FALSE) {
        if (env->EnumItem_GetValue_Int(item, &enumValue) != ANI_OK) {
            HiLog::Error(LABEL, "%{public}s Get item failed", __func__);
        }
    }
    return enumValue;
}

static ani_object NewPerfMeasureResult(ani_env *env, ani_object object, nlohmann::json datas)
{
    ani_object resultObj = {};
    const char *className = "@ohos.test.PerfTest.PerfMeasureResultInner";
    ani_class cls = FindCls(env, className);
    ani_method ctor = FindCtorMethod(env, cls, nullptr);
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (env->Object_New(cls, ctor, &resultObj) != ANI_OK) {
        HiLog::Error(LABEL, "Create PerfMeasureResult object failed");
        return resultObj;
    }
    ani_method setter;
    string attrs[] = {"metric", "roundValues", "maximum", "minimum", "average"};
    for (int index = ZERO; index < FIVE; index++) {
        string tag = attrs[index];
        char *setterName = strdup((Builder::BuildSetterName(tag)).c_str());
        if (env->Class_FindMethod(cls, setterName, nullptr, &setter) != ANI_OK) {
            HiLog::Error(LABEL, "Find Method %{public}s failed", setterName);
            return resultObj;
        }
        if (index == ZERO) {
            auto enumName = Builder::BuildEnum({"@ohos", "test", "PerfTest", "PerfMetric"}).Descriptor();
            ani_enum_item enumItem = FindEnumItem(env, enumName.c_str(), static_cast<ani_int>(datas[tag]));
            if (env->Object_CallMethod_Void(resultObj, setter, enumItem) != ANI_OK) {
                HiLog::Error(LABEL, "Call setter failed %{public}s", attrs[index].c_str());
                return resultObj;
            }
        } else if (index == ONE) {
            auto valueArray = datas[tag];
            ani_array_double aniValueArray;
            env->Array_New_Double(valueArray.size(), &aniValueArray);
            for (int arrayIndex = 0; arrayIndex < valueArray.size(); arrayIndex++) {
                ani_double aniDoubleValue = static_cast<ani_double>(valueArray[arrayIndex]);
                env->Array_Set(aniValueArray, arrayIndex, CreateDouble(env, aniDoubleValue));
            }
            if (env->Object_CallMethod_Void(resultObj, setter, aniValueArray) != ANI_OK) {
                HiLog::Error(LABEL, "Call setter failed %{public}s", attrs[index].c_str());
                return resultObj;
            }
        } else {
            if (env->Object_CallMethod_Void(resultObj, setter, ani_double(datas[tag])) != ANI_OK) {
                HiLog::Error(LABEL, "Call setter failed %{public}s", attrs[index].c_str());
                return resultObj;
            }
        }
    }
    return resultObj;
}

static json ParseStrategyJsonValue(ani_env *env, int index, ani_ref value, ApiReplyInfo &reply)
{
    json jsonItem;
    if (index == ZERO) {
        auto metricArrayObj = static_cast<ani_object>(value);
        auto metricArray = static_cast<ani_array>(metricArrayObj);
        jsonItem = json::array();
        ani_size arraySize;
        if (env->Array_GetLength(metricArray, &arraySize) != ANI_OK) {
            HiLog::Warn(LABEL, "GetLength of metricArray failed");
            return jsonItem;
        }
        for (size_t arrayIndex = 0; arrayIndex < arraySize; arrayIndex++) {
            ani_ref itemValue;
            if (env->Array_Get(metricArray, arrayIndex, &itemValue) != ANI_OK) {
                HiLog::Warn(LABEL, "ArrayGet index %{public}zu failed", arrayIndex);
                continue;
            }
            auto metricItem = static_cast<ani_enum_item>(itemValue);
            jsonItem.push_back(GetEnumValue(env, metricItem));
        }
    } else if (index <= TWO) {
        jsonItem = CallbackCodeAni::Get().PreprocessCallback(env, value, reply);
    } else if (index <= THREE) {
        jsonItem = AniStringToStdString(env, static_cast<ani_string>(value));
    } else {
        ani_int intValue;
        CompareAndReport(env->Object_CallMethodByName_Int(static_cast<ani_object>(value), "toInt", nullptr,
            &intValue), ANI_OK, "GetProperty failed", "GetProperty succeed");
        jsonItem = intValue;
    }
    return jsonItem;
}

static json GetPerfTestStrategy(ani_env *env, ani_object obj, ApiReplyInfo &reply)
{
    auto strategy = json();
    string list[] = {"metrics", "actionCode", "resetCode", "bundleName", "iterations", "timeout"};
    for (int index = 0; index < SIX; index++) {
        const char *cstr = list[index].c_str();
        ani_ref value;
        if (env->Object_GetPropertyByName_Ref(obj, cstr, &value) != ANI_OK) {
            HiLog::Warn(LABEL, "GetPropertyByName %{public}s failed", cstr);
            continue;
        }
        ani_boolean ret;
        env->Reference_IsUndefined(value, &ret);
        if (ret == ANI_TRUE) {
            continue;
        }
        strategy[list[index]] = ParseStrategyJsonValue(env, index, value, reply);
    }
    return strategy;
}

static ani_ref Create(ani_env *env, [[maybe_unused]] ani_class clazz, ani_object perfTestStrategy)
{
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (env->FindClass(Builder::BuildClass({"@ohos", "test", "PerfTest", "PerfTest"}).Descriptor().c_str(), &cls)
        != ANI_OK) {
        HiLog::Error(LABEL, "@ohos/test/PerfTest/PerfTest Not found");
        return nullref;
    }
    ani_method ctor = nullptr;
    arkts::ani_signature::SignatureBuilder string_ctor{};
    string_ctor.AddClass({"std", "core", "String"});
    if (env->Class_FindMethod(cls, Builder::BuildConstructorName().c_str(),
                              string_ctor.BuildSignatureDescriptor().c_str(), &ctor) != ANI_OK) {
        HiLog::Error(LABEL, "PerfTest Constructor CanNot found !!!");
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "PerfTest.create";
    auto strategy = GetPerfTestStrategy(env, perfTestStrategy, reply_);
    if (HasExceptionReply(env, reply_)) {
        return nullref;
    }
    callInfo_.paramList_.push_back(strategy);
    Transact(callInfo_, reply_);
    ani_ref nativePerfTest = UnmarshalReply(env, callInfo_, reply_);
    if (nativePerfTest == nullptr) {
        return nullref;
    }
    ani_object perfTestObject;
    if (env->Object_New(cls, ctor, &perfTestObject, static_cast<ani_object>(nativePerfTest)) != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s New driver failed", __func__);
    }
    return perfTestObject;
}

static ani_boolean RunSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = AniStringToStdString(env, Unwrapp(env, obj, "nativePerfTest"));
    callInfo_.apiId_ = "PerfTest.run";
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref GetMeasureResult(ani_env *env, ani_object obj, ani_enum_item metric)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = AniStringToStdString(env, Unwrapp(env, obj, "nativePerfTest"));
    callInfo_.apiId_ = "PerfTest.getMeasureResult";
    callInfo_.paramList_.push_back(GetEnumValue(env, metric));
    Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    if (result == nullptr) {
        ani_ref nullref;
        env->GetNull(&nullref);
        return nullref;
    }
    ani_object perfMeasureResult = NewPerfMeasureResult(env, obj, reply_.resultValue_);
    return perfMeasureResult;
}

static void Destroy(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = AniStringToStdString(env, Unwrapp(env, obj, "nativePerfTest"));
    callInfo_.apiId_ = "PerfTest.destroy";
    Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static void FinishCallback(ani_env *env, ani_object obj, ani_boolean res)
{
    HiLog::Info(LABEL, "%{public}s called, res = %{public}d", __func__, res);
    CallbackCodeAni::Get().FinishCallback(static_cast<bool>(res));
}

static ani_boolean BindPerfTest(ani_env *env)
{
    ani_class cls;
    if (env->FindClass(Builder::BuildClass({"@ohos", "test", "PerfTest", "PerfTest"}).Descriptor().c_str(), &cls)
        != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s Not found!", __func__);
        return false;
    }
    ani_native_function createMethod {
        "createInner", "C{@ohos/test/PerfTest/PerfTestStrategy}:C{@ohos/test/PerfTest/PerfTest}",
         reinterpret_cast<void *>(Create)};
    if (env->Class_BindStaticNativeMethods(cls, &createMethod, ONE) != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s Cannot bind static native method", __func__);
        return false;
    }

    std::array methods = {
        ani_native_function{"runSync", nullptr, reinterpret_cast<void *>(RunSync)},
        ani_native_function{"getMeasureResult", nullptr, reinterpret_cast<void *>(GetMeasureResult)},
        ani_native_function{"destroy", nullptr, reinterpret_cast<void *>(Destroy)},
    };

    if (env->Class_BindNativeMethods(cls, methods.data(), methods.size()) != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods", __func__);
        return false;
    }
    return true;
}

static ani_boolean BindFinishCallback(ani_env *env)
{
    ani_class cls;
    if (env->FindClass(Builder::BuildClass({"@ohos", "test", "PerfTest", "FinishCallback"}).Descriptor().c_str(), &cls)
        != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s Not found!", __func__);
        return false;
    }
    std::array methods = {
        ani_native_function{"finishCallback", nullptr, reinterpret_cast<void *>(FinishCallback)},
    };

    if (env->Class_BindNativeMethods(cls, methods.data(), methods.size()) != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s Cannot bind native methods", __func__);
        return false;
    }
    return true;
}

void StsPerfTestInit(ani_env *env)
{
    HiLog::Info(LABEL, "%{public}s called", __func__);
    ani_status status = ANI_ERROR;
    if (env->ResetError() != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s ResetError failed", __func__);
    }
    ani_namespace ns;
    status = env->FindNamespace(Builder::BuildNamespace({"@ohos", "test", "PerfTest", "PerfTestInit"})
                                .Descriptor().c_str(), &ns);
    if (status != ANI_OK) {
        HiLog::Error(LABEL, "FindNamespace PerfTest failed status : %{public}d", status);
        return;
    }
    std::array kitFunctions = {
        ani_native_function{"ScheduleEstablishConnection", nullptr,
                            reinterpret_cast<void *>(ScheduleEstablishConnection)},
        ani_native_function{"GetConnectionStat", nullptr, reinterpret_cast<void *>(GetConnectionStat)},
    };
    status = env->Namespace_BindNativeFunctions(ns, kitFunctions.data(), kitFunctions.size());
    if (status != ANI_OK) {
        HiLog::Error(LABEL, "Namespace_BindNativeFunctions failed status : %{public}d", status);
    }
    if (env->ResetError() != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s ResetError failed", __func__);
    }
    HiLog::Info(LABEL, "%{public}s StsPERFTESTInit end", __func__);
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (vm->GetEnv(ANI_VERSION_1, &env) != ANI_OK) {
        HiLog::Error(LABEL, "%{public}s PerfTest: Unsupported ANI_VERSION_1", __func__);
        return (ani_status)ANI_ERROR;
    }
    StsPerfTestInit(env);
    auto status = true;
    status &= BindPerfTest(env);
    status &= BindFinishCallback(env);
    if (!status) {
        HiLog::Error(LABEL, "%{public}s ani_error", __func__);
        return ANI_ERROR;
    }
    HiLog::Info(LABEL, "%{public}s ani_bind success", __func__);
    *result = ANI_VERSION_1;
    return ANI_OK;
}
