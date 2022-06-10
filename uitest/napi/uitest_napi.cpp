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

#include <napi/native_api.h>
#include <napi/native_node_api.h>
#include <set>
#include <string>
#include <unistd.h>
#include "json.hpp"
#include "common_utilities_hpp.h"
#include "frontend_api_defines.h"

namespace OHOS::uitest {
    using namespace nlohmann;
    using namespace std;

    static constexpr size_t NAPI_MAX_BUF_LEN = 1024;
    static constexpr size_t NAPI_MAX_ARG_COUNT = 8;
    static constexpr size_t BACKEND_OBJ_CLEAN_THRESHOLD = 20;
    // type of unexpected or napi-internal error
    static constexpr napi_status NAPI_ERR = napi_status::napi_generic_failure;
    // the name of property that represents the objectRef of the backend object
    static constexpr char PROP_BACKEND_OBJ_REF[] = "backendObjRef";
    /**For dfx usage, records the uncalled js apis. */
    static set<string> g_unCalledJsFuncNames;
    /**For gc usage, records the backend objRefs about to delete. */
    static set<string> g_backendObjsAboutToDelete;

    // use external setup/transact/disposal callback functions
    extern bool SetupTransactionEnv(string_view token);

    extern void TransactionClientFunc(const ApiCallInfo &, ApiReplyInfo &);

    extern void DisposeTransactionEnv();

    static function<void(const ApiCallInfo &, ApiReplyInfo &)> transactFunc = TransactionClientFunc;

    /** Convert js string to cpp string.*/
    static string JsStrToCppStr(napi_env env, napi_value jsStr)
    {
        size_t bufSize = 0;
        char buf[NAPI_MAX_BUF_LEN] = {0};
        NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, jsStr, buf, NAPI_MAX_BUF_LEN, &bufSize), "");
        return string(buf, bufSize);
    }

    /**Lifecycle callback, setup transaction environment, called externally.*/
    static napi_value EnvironmentSetup(napi_env env, napi_callback_info info)
    {
        static auto setUpDone = false;
        if (setUpDone) {
            return nullptr;
        }
        LOG_I("Begin setup transaction environment");
        size_t argc = 1;
        napi_value value = nullptr;
        napi_value argv[1] = {0};
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &value, nullptr));
        NAPI_ASSERT(env, argc > 0, "Need session token argument!");
        setUpDone = SetupTransactionEnv(JsStrToCppStr(env, argv[0]));
        LOG_I("End setup transaction environment, result=%{public}d", setUpDone);
        NAPI_ASSERT(env, setUpDone, "SetupTransactionEnv failed");
        return nullptr;
    }

    /**Lifecycle callback, teardown transaction environment, called externally.*/
    static napi_value EnvironmentTeardown(napi_env env, napi_callback_info info)
    {
        LOG_I("Begin teardown transaction environment");
        DisposeTransactionEnv();
        LOG_I("End teardown transaction environment");
        return nullptr;
    }

    /**Encapsulates the data objects needed in once api transaction.*/
    struct TransactionContext {
        napi_value jsThis_ = nullptr;
        ApiCallInfo callInfo_;
    };

    static napi_value CreateJsException(napi_env env, string_view code, string_view msg)
    {
        napi_value codeValue, msgValue, errorValue;
        napi_create_string_utf8(env, code.data(), NAPI_AUTO_LENGTH, &codeValue);
        napi_create_string_utf8(env, msg.data(), NAPI_AUTO_LENGTH, &msgValue);
        napi_create_error(env, codeValue, msgValue, &errorValue);
        return errorValue;
    }

    /**Set object constructor function to global as attribute.*/
    static napi_status MountJsConstructorToGlobal(napi_env env, string_view typeName, napi_value function)
    {
        NAPI_ASSERT_BASE(env, function != nullptr, "Null constructor function", napi_invalid_arg);
        const string name = "constructor_" + string(typeName);
        napi_value global = nullptr;
        NAPI_CALL_BASE(env, napi_get_global(env, &global), NAPI_ERR);
        NAPI_CALL_BASE(env, napi_set_named_property(env, global, name.c_str(), function), NAPI_ERR);
        return napi_ok;
    }

    /**Get object constructor function from global as attribute.*/
    static napi_status GetJsConstructorFromGlobal(napi_env env, string_view typeName, napi_value *pFunction)
    {
        NAPI_ASSERT_BASE(env, pFunction != nullptr, "Null constructor receiver", napi_invalid_arg);
        const string name = "constructor_" + string(typeName);
        napi_value global = nullptr;
        NAPI_CALL_BASE(env, napi_get_global(env, &global), NAPI_ERR);
        NAPI_CALL_BASE(env, napi_get_named_property(env, global, name.c_str(), pFunction), NAPI_ERR);
        return napi_ok;
    }

    /**Conversion between value and string, using the builtin JSON methods.*/
    static napi_status ValueStringConvert(napi_env env, napi_value in, napi_value *out, bool stringify)
    {
        if (in == nullptr || out == nullptr) {
            return napi_invalid_arg;
        }
        napi_value global = nullptr;
        napi_value jsonProp = nullptr;
        napi_value jsonFunc = nullptr;
        NAPI_CALL_BASE(env, napi_get_global(env, &global), NAPI_ERR);
        NAPI_CALL_BASE(env, napi_get_named_property(env, global, "JSON", &jsonProp), NAPI_ERR);
        if (stringify) {
            NAPI_CALL_BASE(env, napi_get_named_property(env, jsonProp, "stringify", &jsonFunc), NAPI_ERR);
        } else {
            NAPI_CALL_BASE(env, napi_get_named_property(env, jsonProp, "parse", &jsonFunc), NAPI_ERR);
        }
        napi_value argv[1] = {in};
        NAPI_CALL_BASE(env, napi_call_function(env, jsonProp, jsonFunc, 1, argv, out), NAPI_ERR);
        return napi_ok;
    }

    /**Get the readable name of the error enum value.*/
    static string GetErrorName(ErrCode code)
    {
        static const std::map<ErrCode, std::string> names = {
            {NO_ERROR, "NO_ERROR"},       {INTERNAL_ERROR, "INTERNAL_ERROR"},
            {WIDGET_LOST, "WIDGET_LOST"}, {ASSERTION_FAILURE, "ASSERTION_FAILURE"},
            {USAGE_ERROR, "USAGE_ERROR"},
        };
        const auto find = names.find(code);
        return (find == names.end()) ? "UNKNOWN" : find->second;
    }

    /**Unmarshal object from json, throw error and return false if the object cannot be deserialized.*/
    static napi_status UnmarshalObject(napi_env env, const json &in, napi_value *pOut, napi_value jsThis)
    {
        NAPI_ASSERT_BASE(env, pOut != nullptr, "Illegal arguments", napi_invalid_arg);
        const auto type = in.type();
        if (type == nlohmann::detail::value_t::null) { // return null
            NAPI_CALL_BASE(env, napi_get_null(env, pOut), NAPI_ERR);
            return napi_ok;
        }
        if (type != nlohmann::detail::value_t::string) { // non-string value, convert and return object
            NAPI_CALL_BASE(env, napi_create_string_utf8(env, in.dump().c_str(), NAPI_AUTO_LENGTH, pOut), NAPI_ERR);
            NAPI_CALL_BASE(env, ValueStringConvert(env, *pOut, pOut, false), NAPI_ERR);
            return napi_ok;
        }
        const auto cppString = in.get<string>();
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
        NAPI_CALL_BASE(env, napi_create_string_utf8(env, cppString.c_str(), NAPI_AUTO_LENGTH, pOut), NAPI_ERR);
        if (frontendTypeName.empty()) { // plain string, return it
            return napi_ok;
        }
        LOG_D("Convert to frondend object: '%{public}s'", frontendTypeName.c_str());
        // covert to wrapper object and bind the backend objectRef
        napi_value refValue = *pOut;
        napi_value constructor = nullptr;
        NAPI_CALL_BASE(env, GetJsConstructorFromGlobal(env, frontendTypeName, &constructor), NAPI_ERR);
        NAPI_CALL_BASE(env, napi_new_instance(env, constructor, 0, nullptr, pOut), NAPI_ERR);
        NAPI_CALL_BASE(env, napi_set_named_property(env, *pOut, PROP_BACKEND_OBJ_REF, refValue), NAPI_ERR);
        if (bindJsThis) { // bind the jsThis object
            LOG_D("Bind jsThis");
            NAPI_ASSERT_BASE(env, jsThis != nullptr, "null jsThis", NAPI_ERR);
            NAPI_CALL_BASE(env, napi_set_named_property(env, *pOut, "boundObject", jsThis), NAPI_ERR);
        }
        return napi_ok;
    }

    /**Evaluate and convert transaction reply to object. Return the exception raised during the
     * transaction if any, else return the result object. */
    static napi_value UnmarshalReply(napi_env env, const TransactionContext &ctx, const ApiReplyInfo &reply)
    {
        LOG_D("Start to Unmarshal transaction result");
        if (reply.exception_.code_ != ErrCode::NO_ERROR) { // raise error
            const auto code = GetErrorName(reply.exception_.code_);
            const auto &message = reply.exception_.message_;
            LOG_I("ErrorInfo: code='%{public}s', message='%{public}s'", code.c_str(), message.c_str());
            return CreateJsException(env, code, message);
        }
        LOG_D("Start to Unmarshal resutrn value: %{public}s", reply.resultValue_.dump().c_str());
        const auto resultType = reply.resultValue_.type();
        napi_value result = nullptr;
        if (resultType == nlohmann::detail::value_t::null) { // return null
            NAPI_CALL(env, napi_get_null(env, &result));
        } else if (resultType == nlohmann::detail::value_t::array) { // return array
            NAPI_CALL(env, napi_create_array_with_length(env, reply.resultValue_.size(), &result));
            for (size_t idx = 0; idx < reply.resultValue_.size(); idx++) {
                napi_value item = nullptr;
                NAPI_CALL(env, UnmarshalObject(env, reply.resultValue_.at(idx), &item, ctx.jsThis_));
                NAPI_CALL(env, napi_set_element(env, result, idx, item));
            }
        } else { // return single value
            NAPI_CALL(env, UnmarshalObject(env, reply.resultValue_, &result, ctx.jsThis_));
        }
        return result;
    }

    /**Call api with parameters out, wait for and return result value or throw raised exception.*/
    napi_value TransactSync(napi_env env, TransactionContext &ctx)
    {
        LOG_D("TargetApi=%{public}s", ctx.callInfo_.apiId_.data());
        auto reply = ApiReplyInfo();
        transactFunc(ctx.callInfo_, reply);
        auto resultValue = UnmarshalReply(env, ctx, reply);
        auto isError = false;
        NAPI_CALL(env, napi_is_error(env, resultValue, &isError));
        if (isError) {
            NAPI_CALL(env, napi_throw(env, resultValue));
            NAPI_CALL(env, napi_get_undefined(env, &resultValue)); // return undefined it's error
        }
        // notify backend objects deleting
        if (g_backendObjsAboutToDelete.size() >= BACKEND_OBJ_CLEAN_THRESHOLD) {
            auto gcCall = ApiCallInfo {.apiId_ = "BackendObjectsCleaner"};
            auto gcReply = ApiReplyInfo();
            for (auto& ref : g_unCalledJsFuncNames) {
                gcCall.paramList_.emplace_back(ref);
            }
            transactFunc(gcCall, gcReply);
            g_backendObjsAboutToDelete.clear();
        }
        return resultValue;
    }

    /**Encapsulates the data objects needed for async transaction.*/
    struct AsyncTransactionCtx {
        TransactionContext ctx_;
        ApiReplyInfo reply_;
        napi_async_work asyncWork_ = nullptr;
        napi_deferred deferred_ = nullptr;
        napi_ref jsThisRef_ = nullptr;
    };

    /**Call api with parameters out, return a promise.*/
    static napi_value TransactAsync(napi_env env, TransactionContext &ctx)
    {
        constexpr uint32_t refCount = 1;
        LOG_D("TargetApi=%{public}s", ctx.callInfo_.apiId_.data());
        napi_value resName;
        NAPI_CALL(env, napi_create_string_latin1(env, __FUNCTION__, NAPI_AUTO_LENGTH, &resName));
        auto aCtx = new AsyncTransactionCtx();
        aCtx->ctx_ = ctx;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &(aCtx->deferred_), &promise));
        NAPI_CALL(env, napi_create_reference(env, ctx.jsThis_, refCount, &(aCtx->jsThisRef_)));
        napi_create_async_work(
            env, nullptr, resName,
            [](napi_env env, void *data) {
                auto aCtx = reinterpret_cast<AsyncTransactionCtx *>(data);
                // NOT:: use 'auto&' rather than 'auto', or the result will be set to copy-constructed temp-object
                auto &ctx = aCtx->ctx_;
                transactFunc(ctx.callInfo_, aCtx->reply_);
            },
            [](napi_env env, napi_status status, void *data) {
                auto aCtx = reinterpret_cast<AsyncTransactionCtx *>(data);
                napi_get_reference_value(env, aCtx->jsThisRef_, &(aCtx->ctx_.jsThis_));
                auto resultValue = UnmarshalReply(env, aCtx->ctx_, aCtx->reply_);
                napi_delete_reference(env, aCtx->jsThisRef_);
                auto isError = false;
                napi_is_error(env, resultValue, &isError);
                if (isError) {
                    napi_reject_deferred(env, aCtx->deferred_, resultValue);
                } else {
                    napi_resolve_deferred(env, aCtx->deferred_, resultValue);
                }
                delete aCtx;
            },
            (void *)aCtx, &(aCtx->asyncWork_));
        napi_queue_async_work(env, aCtx->asyncWork_);
        return promise;
    }

    static napi_status GetBackendObjRefProp(napi_env env, napi_value value, napi_value* pOut)
    {
        napi_valuetype type = napi_undefined;
        NAPI_CALL_BASE(env, napi_typeof(env, value, &type), NAPI_ERR);
        if (type != napi_object) {
            *pOut = nullptr;
            return napi_ok;
        }
        bool hasRef = false;
        NAPI_CALL_BASE(env, napi_has_named_property(env, value, PROP_BACKEND_OBJ_REF, &hasRef), NAPI_ERR);
        if (!hasRef) {
            *pOut = nullptr;
        } else {
            NAPI_CALL_BASE(env, napi_get_named_property(env, value, PROP_BACKEND_OBJ_REF, pOut), NAPI_ERR);
        }
        return napi_ok;
    }

    /**Generic js-api callback.*/
    static napi_value GenericCallback(napi_env env, napi_callback_info info)
    {
        // extract callback data
        TransactionContext ctx;
        napi_value argv[NAPI_MAX_ARG_COUNT] = {nullptr};
        auto count = NAPI_MAX_ARG_COUNT;
        void *pData = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &count, argv, &(ctx.jsThis_), &pData));
        NAPI_ASSERT(env, pData != nullptr, "Null methodDef");
        auto methodDef = reinterpret_cast<const FrontendMethodDef *>(pData);
        g_unCalledJsFuncNames.erase(string(methodDef->name_)); // api used
        // 1. marshal parameters into json-array
        napi_value paramArray = nullptr;
        NAPI_CALL(env, napi_create_array_with_length(env, count, &paramArray));
        for (size_t idx = 0; idx < count; idx++) {
            napi_value item = nullptr; // convert to backendObjRef if any
            NAPI_CALL(env, GetBackendObjRefProp(env, argv[idx], &item));
            if (item == nullptr) {
                item = argv[idx];
            }
            NAPI_CALL(env, napi_set_element(env, paramArray, idx, item));
        }
        napi_value jsTempObj = nullptr;
        NAPI_CALL(env, ValueStringConvert(env, paramArray, &jsTempObj, true));
        ctx.callInfo_.paramList_ = nlohmann::json::parse(JsStrToCppStr(env, jsTempObj));
        // 2. marshal jsThis into json (backendObjRef)
        if (!methodDef->static_) {
            NAPI_CALL(env, GetBackendObjRefProp(env, ctx.jsThis_, &jsTempObj));
            ctx.callInfo_.callerObjRef_ = JsStrToCppStr(env, jsTempObj);
        }
        // 3. fill-in apiId
        ctx.callInfo_.apiId_ = methodDef->name_;
        // 4. call out, aync or async
        if (methodDef->fast_) {
            return TransactSync(env, ctx);
        } else {
            return TransactAsync(env, ctx);
        }
    }

    /**Exports uitest js wrapper-classes and its global constructor.*/
    static napi_status ExportClass(napi_env env, napi_value exports, const FrontEndClassDef &classDef)
    {
        NAPI_ASSERT_BASE(env, exports != nullptr, "Illegal export params", NAPI_ERR);
        const auto name = classDef.name_.data();
        const auto methodNeatNameOffset = classDef.name_.length() + 1;
        auto descs = make_unique<napi_property_descriptor[]>(classDef.methodCount_);
        for (size_t idx = 0; idx < classDef.methodCount_; idx++) {
            const auto &methodDef = classDef.methods_[idx];
            g_unCalledJsFuncNames.insert(string(methodDef.name_));
            const auto neatName = methodDef.name_.substr(methodNeatNameOffset);
            napi_property_descriptor desc = DECLARE_NAPI_FUNCTION(neatName.data(), GenericCallback);
            if (methodDef.static_) {
                desc.attributes = napi_static;
            }
            desc.data = (void *)(&methodDef);
            descs[idx] = desc;
        }
        constexpr auto initializer = [](napi_env env, napi_callback_info info) {
            napi_value jsThis = nullptr;
            NAPI_CALL_BASE(env, napi_get_cb_info(env, info, nullptr, nullptr, &jsThis, nullptr), jsThis);
            napi_value refValue = nullptr;
            NAPI_CALL_BASE(env, napi_get_named_property(env, jsThis, PROP_BACKEND_OBJ_REF, &refValue), jsThis);
            auto ref = make_unique<string>(JsStrToCppStr(env, refValue));
            auto finalizer = [](napi_env env, void *data, void *hint) {
                auto ref = reinterpret_cast<string *>(data);
                if (ref->length() > 0) {
                    LOG_D("Finalizing object: %{public}s", ref->c_str());
                    g_backendObjsAboutToDelete.insert(*ref);
                }
                delete ref;
            };
            NAPI_CALL_BASE(env, napi_wrap(env, jsThis, ref.release(), finalizer, nullptr, nullptr), jsThis);
            return jsThis;
        };
        // define class, provide the js-class members(property) and initializer.
        napi_value ctor = nullptr;
        NAPI_CALL_BASE(env, napi_define_class(env, name, NAPI_AUTO_LENGTH, initializer, nullptr,
                                              classDef.methodCount_, descs.get(), &ctor), NAPI_ERR);
        NAPI_CALL_BASE(env, napi_set_named_property(env, exports, name, ctor), NAPI_ERR);
        NAPI_CALL_BASE(env, MountJsConstructorToGlobal(env, name, ctor), NAPI_ERR);
        if (string_view(name) == "By") {
            // create seed-by with special objectRef and mount to exporter
            napi_value bySeed = nullptr;
            NAPI_CALL_BASE(env, napi_new_instance(env, ctor, 0, nullptr, &bySeed), NAPI_ERR);
            napi_value prop = nullptr;
            NAPI_CALL_BASE(env, napi_create_string_utf8(env, REF_SEED_BY.data(), NAPI_AUTO_LENGTH, &prop), NAPI_ERR);
            NAPI_CALL_BASE(env, napi_set_named_property(env, bySeed, PROP_BACKEND_OBJ_REF, prop), NAPI_ERR);
            NAPI_CALL_BASE(env, napi_set_named_property(env, exports, "BY", bySeed), NAPI_ERR);
        }
        return napi_ok;
    }

    /**Exports enumrator class.*/
    static napi_status ExportEnumrator(napi_env env, napi_value exports, const FrontendEnumratorDef &enumDef)
    {
        NAPI_ASSERT_BASE(env, exports != nullptr, "Illegal export params", NAPI_ERR);
        napi_value enumrator;
        NAPI_CALL_BASE(env, napi_create_object(env, &enumrator), NAPI_ERR);
        for (size_t idx = 0; idx < enumDef.valueCount_; idx++) {
            const auto &def = enumDef.values_[idx];
            napi_value prop = nullptr;
            NAPI_CALL_BASE(env, napi_create_string_utf8(env, def.valueJson_.data(), NAPI_AUTO_LENGTH, &prop), NAPI_ERR);
            NAPI_CALL_BASE(env, ValueStringConvert(env, prop, &prop, false), NAPI_ERR);
            NAPI_CALL_BASE(env, napi_set_named_property(env, enumrator, def.name_.data(), prop), NAPI_ERR);
        }
        NAPI_CALL_BASE(env, napi_set_named_property(env, exports, enumDef.name_.data(), enumrator), NAPI_ERR);
        return napi_ok;
    }

    /**Function used for statistics, return an array of uncalled js-api names.*/
    static napi_value GetUnCalledJsApis(napi_env env, napi_callback_info info)
    {
        napi_value nameArray;
        NAPI_CALL(env, napi_create_array_with_length(env, g_unCalledJsFuncNames.size(), &nameArray));
        size_t idx = 0;
        for (auto &name : g_unCalledJsFuncNames) {
            napi_value nameItem = nullptr;
            NAPI_CALL(env, napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &nameItem));
            NAPI_CALL(env, napi_set_element(env, nameArray, idx, nameItem));
            idx++;
        }
        return nameArray;
    }

    napi_value Export(napi_env env, napi_value exports)
    {
        LOG_I("Begin export uitest apis");
        // export transaction-environment-lifecycle callbacks and dfx functions
        napi_property_descriptor props[] = {
            DECLARE_NAPI_STATIC_FUNCTION("setup", EnvironmentSetup),
            DECLARE_NAPI_STATIC_FUNCTION("teardown", EnvironmentTeardown),
            DECLARE_NAPI_STATIC_FUNCTION("getUnCalledJsApis", GetUnCalledJsApis),
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(props) / sizeof(props[0]), props));
        NAPI_CALL(env, ExportClass(env, exports, BY_DEF));
        NAPI_CALL(env, ExportClass(env, exports, UI_DRIVER_DEF));
        NAPI_CALL(env, ExportClass(env, exports, UI_COMPONENT_DEF));
        NAPI_CALL(env, ExportEnumrator(env, exports, MATCH_PATTERN_DEF));
        LOG_I("End export uitest apis");
        return exports;
    }

    static napi_module module = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Export,
        .nm_modname = "uitest",
        .nm_priv = ((void *)0),
        .reserved = {0},
    };

    extern "C" __attribute__((constructor)) void RegisterModule(void)
    {
        napi_module_register(&module);
    }
} // namespace OHOS::uitest

// put register functions out of namespace to ensure C-linkage
extern const char _binary_uitest_exporter_js_start[];
extern const char _binary_uitest_exporter_js_end[];
extern const char _binary_uitest_exporter_abc_start[];
extern const char _binary_uitest_exporter_abc_end[];

extern "C" __attribute__((visibility("default"))) void NAPI_uitest_GetJSCode(const char **buf, int *bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_uitest_exporter_js_start;
    }
    if (bufLen != nullptr) {
        *bufLen = _binary_uitest_exporter_js_end - _binary_uitest_exporter_js_start;
    }
}

extern "C" __attribute__((visibility("default"))) void NAPI_uitest_GetABCCode(const char **buf, int *bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_uitest_exporter_abc_start;
    }
    if (bufLen != nullptr) {
        *bufLen = _binary_uitest_exporter_abc_end - _binary_uitest_exporter_abc_start;
    }
}
