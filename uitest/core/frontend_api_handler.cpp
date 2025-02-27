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
#include <unistd.h>
#include <regex.h>
#include <cstdio>
#include <cstdlib>
#include "ui_driver.h"
#include "widget_operator.h"
#include "window_operator.h"
#include "ui_controller.h"
#include "frontend_api_handler.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    using namespace nlohmann::detail;

    class UIEventObserver : public BackendClass {
    public:
        UIEventObserver() {};
        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return UI_EVENT_OBSERVER_DEF;
        };
    };

    class UiEventFowarder : public UiEventListener {
    public:
        UiEventFowarder() {};

        void IncRef(const string &ref)
        {
            auto find = refCountMap_.find(ref);
            if (find != refCountMap_.end()) {
                find->second++;
            } else {
                refCountMap_.insert(make_pair(ref, 1));
            }
        }

        uint32_t DecAndGetRef(const string &ref)
        {
            auto find = refCountMap_.find(ref);
            if (find != refCountMap_.end()) {
                find->second--;
                if (find->second == 0) {
                    refCountMap_.erase(find);
                    return 0;
                }
                return find->second;
            }
            return 0;
        }

        void OnEvent(const std::string &event, const UiEventSourceInfo &source) override
        {
            const auto &server = FrontendApiServer::Get();
            json uiElementInfo;
            uiElementInfo["bundleName"] = source.bundleName;
            uiElementInfo["type"] = source.type;
            uiElementInfo["text"] = source.text;
            auto count = callBackInfos_.count(event);
            size_t index = 0;
            while (index < count) {
                auto find = callBackInfos_.find(event);
                if (find == callBackInfos_.end()) {
                    return;
                }
                const auto &observerRef = find->second.first;
                const auto &callbackRef = find->second.second;
                ApiCallInfo in;
                ApiReplyInfo out;
                in.apiId_ = "UIEventObserver.once";
                in.callerObjRef_ = observerRef;
                in.paramList_.push_back(uiElementInfo);
                in.paramList_.push_back(callbackRef);
                in.paramList_.push_back(DecAndGetRef(observerRef) == 0);
                in.paramList_.push_back(DecAndGetRef(callbackRef) == 0);
                server.Callback(in, out);
                callBackInfos_.erase(find);
                index++;
            }
        }

        void AddCallbackInfo(const string &&event, const string &observerRef, const string &&cbRef)
        {
            auto count = callBackInfos_.count(event);
            auto find = callBackInfos_.find(event);
            for (size_t index = 0; index < count; index++) {
                if (find != callBackInfos_.end()) {
                    if (find->second.first == observerRef && find->second.second == cbRef) {
                        return;
                    }
                    find++;
                }
            }
            callBackInfos_.insert(make_pair(event, make_pair(observerRef, cbRef)));
            IncRef(observerRef);
            IncRef(cbRef);
        }

    private:
        multimap<string, pair<string, string>> callBackInfos_;
        map<string, int> refCountMap_;
    };

    /** API argument type list map.*/
    static multimap<string, pair<vector<string>, size_t>> sApiArgTypesMap;

    static void ParseMethodSignature(string_view signature, vector<string> &types, size_t &defArg)
    {
        int charIndex = 0;
        constexpr size_t BUF_LEN = 32;
        char buf[BUF_LEN];
        size_t tokenLen = 0;
        size_t defArgCount = 0;
        string token;
        for (char ch : signature) {
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                buf[tokenLen++] = ch;
            } else if (ch == '?') {
                defArgCount++;
            } else if (ch == ',' || ch == '?' || ch == ')') {
                if (tokenLen > 0) {
                    token = string_view(buf, tokenLen);
                    DCHECK(find(DATA_TYPE_SCOPE.begin(), DATA_TYPE_SCOPE.end(), token) != DATA_TYPE_SCOPE.end());
                    types.emplace_back(token);
                }
                tokenLen = 0; // consume token and reset buffer
                if (ch == ')') {
                    // add return value type to the end of types.
                    string retType = string(signature.substr(charIndex + 2));
                    types.emplace_back(retType);
                    break; // end parsing
                }
            }
            charIndex++;
        }
        defArg = defArgCount;
    }

    /** Parse frontend method definitions to collect type information.*/
    static void ParseFrontendMethodsSignature()
    {
        for (auto classDef : FRONTEND_CLASS_DEFS) {
            LOG_D("parse class %{public}s", string(classDef->name_).c_str());
            if (classDef->methods_ == nullptr || classDef->methodCount_ <= 0) {
                continue;
            }
            for (size_t idx = 0; idx < classDef->methodCount_; idx++) {
                auto methodDef = classDef->methods_[idx];
                auto paramTypes = vector<string>();
                size_t hasDefaultArg = 0;
                ParseMethodSignature(methodDef.signature_, paramTypes, hasDefaultArg);
                sApiArgTypesMap.insert(make_pair(string(methodDef.name_), make_pair(paramTypes, hasDefaultArg)));
            }
        }
    }

    static void ParseExtensionMethodsSignature()
    {
        auto paramTypesForGetAllProperties = vector<string>();
        paramTypesForGetAllProperties.push_back("");
        string methodForGetAllProperties = "Component.getAllProperties";
        sApiArgTypesMap.insert(make_pair(methodForGetAllProperties, make_pair(paramTypesForGetAllProperties, 0)));

        auto paramTypesForAamsWorkMode = vector<string>();
        paramTypesForAamsWorkMode.push_back("int");
        paramTypesForAamsWorkMode.push_back("");
        string methodForAamsWorkMode = "Driver.SetAamsWorkMode";
        sApiArgTypesMap.insert(make_pair(methodForAamsWorkMode, make_pair(paramTypesForAamsWorkMode, 0)));
    }

    static string GetClassName(const string &apiName, char splitter)
    {
        auto classNameLen = apiName.find(splitter);
        if (classNameLen == std::string::npos) {
            return "";
        }
        return apiName.substr(0, classNameLen);
    }

    static string CheckAndDoApiMapping(string_view apiName, char splitter, const map<string, string> &apiMap)
    {
        string output = string(apiName);
        auto classNameLen = output.find(splitter);
        if (classNameLen == std::string::npos) {
            return output;
        }
        string className = output.substr(0, classNameLen);
        auto result = apiMap.find(className);
        if (result == apiMap.end()) {
            return output;
        }
        auto specialMapItem = apiMap.find(output);
        if (specialMapItem != apiMap.end()) {
            output = specialMapItem->second;
        } else {
            output.replace(0, classNameLen, result->second);
        }
        LOG_D("original className: %{public}s, modified to: %{public}s", className.c_str(), output.c_str());
        return output;
    }

    FrontendApiServer::FrontendApiServer()
    {
        old2NewApiMap_["By"] = "On";
        old2NewApiMap_["UiDriver"] = "Driver";
        old2NewApiMap_["UiComponent"] = "Component";
        old2NewApiMap_["By.id"] = "On.accessibilityId";
        old2NewApiMap_["By.key"] = "On.id";
        old2NewApiMap_["UiComponent.getId"] = "Component.getAccessibilityId";
        old2NewApiMap_["UiComponent.getKey"] = "Component.getId";
        old2NewApiMap_["UiWindow.isActived"] = "UiWindow.isActive";
        new2OldApiMap_["On"] = "By" ;
        new2OldApiMap_["Driver"] = "UiDriver" ;
        new2OldApiMap_["Component"] = "UiComponent" ;
    }

    /**
     * map api8 old api call to new api.
     * return old api name if it's an old api call.
     * return empty string if it's a new api call.
     */
    static const std::map<ErrCode, std::vector<ErrCode>> ErrCodeMap = {
        /**Correspondence between old and new error codes*/
        {NO_ERROR, {NO_ERROR}},
        {ERR_COMPONENT_LOST, {WIDGET_LOST, WINDOW_LOST}},
        {ERR_NO_SYSTEM_CAPABILITY, {USAGE_ERROR}},
        {ERR_INTERNAL, {INTERNAL_ERROR}},
        {ERR_INITIALIZE_FAILED, {USAGE_ERROR}},
        {ERR_INVALID_INPUT, {USAGE_ERROR}},
        {ERR_ASSERTION_FAILED, {ASSERTION_FAILURE}},
        {ERR_OPERATION_UNSUPPORTED, {INTERNAL_ERROR}},
        {ERR_API_USAGE, {INTERNAL_ERROR}},
    };
    
    string FrontendApiServer::ApiMapPre(ApiCallInfo &inModifier) const
    {
        // 1. map method name
        const string &className = GetClassName(inModifier.apiId_, '.');
        const auto result = old2NewApiMap_.find(className);
        if (result == old2NewApiMap_.end()) {
            auto iter = old2NewApiMap_.find(inModifier.apiId_);
            if (iter != old2NewApiMap_.end()) {
                LOG_D("original api:%{public}s, modified to:%{public}s", inModifier.apiId_.c_str(),
                    iter->second.c_str());
                inModifier.apiId_ = iter->second;
            }
            return "";
        }
        string oldApiName = inModifier.apiId_;
        inModifier.apiId_ = CheckAndDoApiMapping(inModifier.apiId_, '.', old2NewApiMap_);
        LOG_D("Modify call name to %{public}s", inModifier.apiId_.c_str());
        // 2. map callerObjRef
        if (inModifier.callerObjRef_.find(className) == 0) {
            inModifier.callerObjRef_.replace(0, className.length(), result->second);
            LOG_D("Modify callobjref to %{public}s", inModifier.callerObjRef_.c_str());
        }
        // 3. map parameters
        // find method signature of old api
        const auto find = sApiArgTypesMap.find(oldApiName);
        if (find == sApiArgTypesMap.end()) {
            return oldApiName;
        }
        const auto &argTypes = find->second.first;
        size_t argCount = inModifier.paramList_.size();
        if (argTypes.size() - 1 < argCount) {
            // parameter number invalid
            return oldApiName;
        }
        for (size_t i = 0; i < argCount; i++) {
            auto &argItem = inModifier.paramList_.at(i);
            const string &argType = argTypes.at(i);
            if (argType != "string" && argItem.type() == value_t::string) {
                argItem = CheckAndDoApiMapping(argItem.get<string>(), '#', old2NewApiMap_);
            }
        }
        return oldApiName;
    }

    static void ErrCodeMapping(ApiCallErr &apiErr)
    {
        ErrCode ec = apiErr.code_;
        auto msg = apiErr.message_;
        auto findCode = ErrCodeMap.find(ec);
        if (findCode != ErrCodeMap.end() && !findCode->second.empty()) {
            apiErr = ApiCallErr(findCode->second.at(0), msg);
        }
    }

    /** map new error code and api Object to old error code and api Object. */
    void FrontendApiServer::ApiMapPost(const string &oldApiName, ApiReplyInfo &out) const
    {
        json &value = out.resultValue_;
        const auto type = value.type();
        // 1. error code conversion
        ErrCodeMapping(out.exception_);
        // 2. ret value conversion
        const auto find = sApiArgTypesMap.find(oldApiName);
        if (find == sApiArgTypesMap.end()) {
            return;
        }
        string &retType = find->second.first.back();
        if ((retType == "string") || (retType == "[string]")) {
            return;
        }
        if (type == value_t::string) {
            value = CheckAndDoApiMapping(value.get<string>(), '#', new2OldApiMap_);
        } else if (type == value_t::array) {
            for (auto &item : value) {
                if (item.type() == value_t::string) {
                    item = CheckAndDoApiMapping(item.get<string>(), '#', new2OldApiMap_);
                }
            }
        }
    }
 
    FrontendApiServer &FrontendApiServer::Get()
    {
        static FrontendApiServer singleton;
        return singleton;
    }

    void FrontendApiServer::AddHandler(string_view apiId, ApiInvokeHandler handler)
    {
        if (handler == nullptr) {
            return;
        }
        handlers_.insert(make_pair(apiId, handler));
    }

    void FrontendApiServer::SetCallbackHandler(ApiInvokeHandler handler)
    {
        callbackHandler_ = handler;
    }

    void FrontendApiServer::Callback(const ApiCallInfo& in, ApiReplyInfo& out) const
    {
        if (callbackHandler_ == nullptr) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "No callback handler set!");
            return;
        }
        callbackHandler_(in, out);
    }

    bool FrontendApiServer::HasHandlerFor(std::string_view apiId) const
    {
        string apiIdstr = CheckAndDoApiMapping(apiId, '.', old2NewApiMap_);
        return handlers_.find(apiIdstr) != handlers_.end();
    }

    void FrontendApiServer::RemoveHandler(string_view apiId)
    {
        handlers_.erase(string(apiId));
    }

    void FrontendApiServer::AddCommonPreprocessor(string_view name, ApiInvokeHandler processor)
    {
        if (processor == nullptr) {
            return;
        }
        commonPreprocessors_.insert(make_pair(name, processor));
    }

    void FrontendApiServer::RemoveCommonPreprocessor(string_view name)
    {
        commonPreprocessors_.erase(string(name));
    }

    void FrontendApiServer::Call(const ApiCallInfo &in, ApiReplyInfo &out) const
    {
        LOG_I("Begin to invoke api '%{public}s', '%{public}s'", in.apiId_.data(), in.paramList_.dump().data());
        auto call = in;
        // initialize method signature
        if (sApiArgTypesMap.empty()) {
            ParseFrontendMethodsSignature();
            ParseExtensionMethodsSignature();
        }
        string oldApiName = ApiMapPre(call);
        auto find = handlers_.find(call.apiId_);
        if (find == handlers_.end()) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "No handler found for api '" + call.apiId_ + "'");
            return;
        }
        try {
            for (auto &[name, processor] : commonPreprocessors_) {
                processor(call, out);
                if (out.exception_.code_ != NO_ERROR) {
                    out.exception_.message_ = "(PreProcessing: " + name + ")" + out.exception_.message_;
                    if (oldApiName.length() > 0) {
                        ApiMapPost(oldApiName, out);
                    }
                    return; // error during pre-processing, abort
                }
            }
        } catch (std::exception &ex) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Preprocessor failed: " + string(ex.what()));
        }
        try {
            find->second(call, out);
        } catch (std::exception &ex) {
            // catch possible json-parsing error
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Handler failed: " + string(ex.what()));
        }
        if (oldApiName.length() > 0) {
            ApiMapPost(oldApiName, out);
        }
    }

    void ApiTransact(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        LOG_I("Begin to invoke api '%{public}s', '%{public}s'", in.apiId_.data(), in.paramList_.dump().data());
        FrontendApiServer::Get().Call(in, out);
    }

    /** Backend objects cache.*/
    static map<string, unique_ptr<BackendClass>> sBackendObjects;
    /** UiDriver binding map.*/
    static map<string, string> sDriverBindingMap;


#define CHECK_CALL_ARG(condition, code, message, error) \
    if (!(condition)) {                                 \
        error = ApiCallErr((code), (message));          \
        return;                                         \
    }

    /** Check if the json value represents and illegal data of expected type.*/
    static void CheckCallArgType(string_view expect, const json &value, bool isDefAgc, ApiCallErr &error)
    {
        const auto type = value.type();
        if (isDefAgc && type == value_t::null) {
            return;
        }
        const auto isInteger = type == value_t::number_integer || type == value_t::number_unsigned;
        auto begin0 = FRONTEND_CLASS_DEFS.begin();
        auto end0 = FRONTEND_CLASS_DEFS.end();
        auto begin1 = FRONTEND_JSON_DEFS.begin();
        auto end1 = FRONTEND_JSON_DEFS.end();
        auto find0 = find_if(begin0, end0, [&expect](const FrontEndClassDef *def) { return def->name_ == expect; });
        auto find1 = find_if(begin1, end1, [&expect](const FrontEndJsonDef *def) { return def->name_ == expect; });
        if (expect == "int") {
            CHECK_CALL_ARG(isInteger, ERR_INVALID_INPUT, "Expect integer", error);
            if (atoi(value.dump().c_str()) < 0) {
                error = ApiCallErr(ERR_INVALID_INPUT, "Expect integer which cannot be less than 0");
                return;
            }
        } else if (expect == "float") {
            CHECK_CALL_ARG(isInteger || type == value_t::number_float, ERR_INVALID_INPUT, "Expect float", error);
        } else if (expect == "bool") {
            CHECK_CALL_ARG(type == value_t::boolean, ERR_INVALID_INPUT, "Expect boolean", error);
        } else if (expect == "string") {
            CHECK_CALL_ARG(type == value_t::string, ERR_INVALID_INPUT, "Expect string", error);
        } else if (find0 != end0) {
            CHECK_CALL_ARG(type == value_t::string, ERR_INVALID_INPUT, "Expect " + string(expect), error);
            const auto findRef = sBackendObjects.find(value.get<string>());
            CHECK_CALL_ARG(findRef != sBackendObjects.end(), ERR_INTERNAL, "Bad object ref", error);
        } else if (find1 != end1) {
            CHECK_CALL_ARG(type == value_t::object, ERR_INVALID_INPUT, "Expect " + string(expect), error);
            auto copy = value;
            for (size_t idx = 0; idx < (*find1)->propCount_; idx++) {
                auto def = (*find1)->props_ + idx;
                const auto propName = string(def->name_);
                if (!value.contains(propName)) {
                    CHECK_CALL_ARG(!(def->required_), ERR_INVALID_INPUT, "Missing property " + propName, error);
                    continue;
                }
                copy.erase(propName);
                // check json property value type recursive
                CheckCallArgType(def->type_, value[propName], !def->required_, error);
                if (error.code_ != NO_ERROR) {
                    error.message_ = "Illegal value of property '" + propName + "': " + error.message_;
                    return;
                }
            }
            CHECK_CALL_ARG(copy.empty(), ERR_INVALID_INPUT, "Illegal property of " + string(expect), error);
        } else {
            CHECK_CALL_ARG(false, ERR_INTERNAL, "Unknown target type " + string(expect), error);
        }
    }

    /** Checks ApiCallInfo data, deliver exception and abort invocation if check fails.*/
    static void APiCallInfoChecker(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        auto count = sApiArgTypesMap.count(in.apiId_);
        // return nullptr by default
        out.resultValue_ = nullptr;
        auto find = sApiArgTypesMap.find(in.apiId_);
        size_t index = 0;
        while (index < count) {
            if (find == sApiArgTypesMap.end()) {
                return;
            }
            bool checkArgNum = false;
            bool checkArgType = true;
            out.exception_ = {NO_ERROR, "No Error"};
            auto &types = find->second.first;
            auto argSupportDefault = find->second.second;
            // check argument count.(last item of "types" is return value type)
            auto maxArgc = types.size() - 1;
            auto minArgc = maxArgc - argSupportDefault;
            auto argc = in.paramList_.size();
            checkArgNum = argc <= maxArgc && argc >= minArgc;
            if (!checkArgNum) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Illegal argument count");
                ++find;
                ++index;
                continue;
            }
            // check argument type
            for (size_t idx = 0; idx < argc; idx++) {
                auto isDefArg = (idx >= minArgc) ? true : false;
                CheckCallArgType(types.at(idx), in.paramList_.at(idx), isDefArg, out.exception_);
                if (out.exception_.code_ != NO_ERROR) {
                    out.exception_.message_ = "Check arg" + to_string(idx) + " failed: " + out.exception_.message_;
                    checkArgType = false;
                    break;
                }
            }
            if (checkArgType) {
                return;
            }
            ++find;
            ++index;
        }
    }

    /** Store the backend object and return the reference-id.*/
    static string StoreBackendObject(unique_ptr<BackendClass> ptr, string_view ownerRef = "")
    {
        static map<string, uint32_t> sObjectCounts;
        DCHECK(ptr != nullptr);
        const auto typeName = string(ptr->GetFrontendClassDef().name_);
        auto find = sObjectCounts.find(typeName);
        uint32_t index = 0;
        if (find != sObjectCounts.end()) {
            index = find->second;
        }
        auto ref = typeName + "#" + to_string(index);
        sObjectCounts[typeName] = index + 1;
        sBackendObjects[ref] = move(ptr);
        if (!ownerRef.empty()) {
            DCHECK(sBackendObjects.find(string(ownerRef)) != sBackendObjects.end());
            sDriverBindingMap[ref] = ownerRef;
        }
        return ref;
    }

    /** Retrieve the stored backend object by reference-id.*/
    template <typename T, typename = enable_if<is_base_of_v<BackendClass, T>>>
    static T &GetBackendObject(string_view ref)
    {
        auto find = sBackendObjects.find(string(ref));
        DCHECK(find != sBackendObjects.end() && find->second != nullptr);
        return *(reinterpret_cast<T *>(find->second.get()));
    }

    static UiDriver &GetBoundUiDriver(string_view ref)
    {
        auto find0 = sDriverBindingMap.find(string(ref));
        DCHECK(find0 != sDriverBindingMap.end());
        auto find1 = sBackendObjects.find(find0->second);
        DCHECK(find1 != sBackendObjects.end() && find1->second != nullptr);
        return *(reinterpret_cast<UiDriver *>(find1->second.get()));
    }

    /** Delete stored backend objects.*/
    static void BackendObjectsCleaner(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        stringstream ss("Deleted objects[");
        DCHECK(in.paramList_.type() == value_t::array);
        for (const auto &item : in.paramList_) {
            DCHECK(item.type() == value_t::string); // must be objRef
            const auto ref = item.get<string>();
            auto findBinding = sDriverBindingMap.find(ref);
            if (findBinding != sDriverBindingMap.end()) {
                sDriverBindingMap.erase(findBinding);
            }
            auto findObject = sBackendObjects.find(ref);
            if (findObject == sBackendObjects.end()) {
                LOG_W("No such object living: %{public}s", ref.c_str());
                continue;
            }
            sBackendObjects.erase(findObject);
            ss << ref << ",";
        }
        ss << "]";
        LOG_D("%{public}s", ss.str().c_str());
    }

    template <typename T> static T ReadCallArg(const ApiCallInfo &in, size_t index)
    {
        DCHECK(in.paramList_.type() == value_t::array);
        DCHECK(index <= in.paramList_.size());
        return in.paramList_.at(index).get<T>();
    }

    template <typename T> static T ReadCallArg(const ApiCallInfo &in, size_t index, T defValue)
    {
        DCHECK(in.paramList_.type() == value_t::array);
        if (index >= in.paramList_.size()) {
            return defValue;
        }
        auto type = in.paramList_.at(index).type();
        if (type == value_t::null) {
            return defValue;
        } else {
            return in.paramList_.at(index).get<T>();
        }
    }

    ApiCallErr CheckRegExp(string regex)
    {
        regex_t preg;
        int rc;
        const int errorLength = 100;
        char errorBuffer[errorLength] = "";
        ApiCallErr error(NO_ERROR);
        char *patternValue = const_cast<char*>(regex.data());
        if ((rc = regcomp(&preg, patternValue, REG_EXTENDED)) != 0) {
            regerror(rc, &preg, errorBuffer, errorLength);
            LOG_E("Regcomp error : %{public}s", errorBuffer);
            error.code_ = ERR_INVALID_INPUT;
            error.message_ = errorBuffer;
        }
        return error;
    }
    template <UiAttr kAttr, typename T> static void GenericOnAttrBuilder(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        // always create and return a new selector
        auto selector = make_unique<WidgetSelector>();
        if (in.callerObjRef_ != REF_SEED_ON) { // copy-construct from the caller if it's not seed
            *selector = GetBackendObject<WidgetSelector>(in.callerObjRef_);
        }
        string testValue = "";
        if constexpr (std::is_same<string, T>::value) {
            testValue = ReadCallArg<string>(in, INDEX_ZERO);
        } else if constexpr (std::is_same<bool, T>::value) { // bool testValue can be defaulted to true
            testValue = ReadCallArg<bool>(in, INDEX_ZERO, true) ? "true" : "false";
        } else {
            testValue = to_string(ReadCallArg<T>(in, INDEX_ZERO));
        }
        auto matchPattern = ReadCallArg<uint8_t>(in, INDEX_ONE, ValueMatchPattern::EQ); // match pattern argument
        if (matchPattern == ValueMatchPattern::REG_EXP || matchPattern == ValueMatchPattern::REG_EXP_ICASE) {
            out.exception_.code_ = NO_ERROR;
            out.exception_ = CheckRegExp(testValue);
        }
        if (out.exception_.code_ != NO_ERROR) {
            return;
        }
        auto matcher = WidgetMatchModel(kAttr, testValue, static_cast<ValueMatchPattern>(matchPattern));
        selector->AddMatcher(matcher);
        out.resultValue_ = StoreBackendObject(move(selector));
    }

    static void RegisterOnBuilders()
    {
        auto &server = FrontendApiServer::Get();
        server.AddHandler("On.accessibilityId", GenericOnAttrBuilder<UiAttr::ACCESSIBILITY_ID, int32_t>);
        server.AddHandler("On.text", GenericOnAttrBuilder<UiAttr::TEXT, string>);
        server.AddHandler("On.id", GenericOnAttrBuilder<UiAttr::ID, string>);
        server.AddHandler("On.description", GenericOnAttrBuilder<UiAttr::DESCRIPTION, string>);
        server.AddHandler("On.type", GenericOnAttrBuilder<UiAttr::TYPE, string>);
        server.AddHandler("On.hint", GenericOnAttrBuilder<UiAttr::HINT, string>);
        server.AddHandler("On.enabled", GenericOnAttrBuilder<UiAttr::ENABLED, bool>);
        server.AddHandler("On.focused", GenericOnAttrBuilder<UiAttr::FOCUSED, bool>);
        server.AddHandler("On.selected", GenericOnAttrBuilder<UiAttr::SELECTED, bool>);
        server.AddHandler("On.clickable", GenericOnAttrBuilder<UiAttr::CLICKABLE, bool>);
        server.AddHandler("On.longClickable", GenericOnAttrBuilder<UiAttr::LONG_CLICKABLE, bool>);
        server.AddHandler("On.scrollable", GenericOnAttrBuilder<UiAttr::SCROLLABLE, bool>);
        server.AddHandler("On.checkable", GenericOnAttrBuilder<UiAttr::CHECKABLE, bool>);
        server.AddHandler("On.checked", GenericOnAttrBuilder<UiAttr::CHECKED, bool>);

        auto genericRelativeBuilder = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            const auto attrName = in.apiId_.substr(ON_DEF.name_.length() + 1); // On.xxx()->xxx
            // always create and return a new selector
            auto selector = make_unique<WidgetSelector>();
            if (in.callerObjRef_ != REF_SEED_ON) { // copy-construct from the caller if it's not seed
                *selector = GetBackendObject<WidgetSelector>(in.callerObjRef_);
            }
            if (attrName == "isBefore") {
                auto &that = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                selector->AddRearLocator(that, out.exception_);
            } else if (attrName == "isAfter") {
                auto &that = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                selector->AddFrontLocator(that, out.exception_);
            } else if (attrName == "within") {
                auto &that = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                selector->AddParentLocator(that, out.exception_);
            } else if (attrName == "inWindow") {
                auto hostApp = ReadCallArg<string>(in, INDEX_ZERO);
                selector->AddAppLocator(hostApp);
            } else if (attrName == "inDisplay") {
                auto displayId = ReadCallArg<int32_t>(in, INDEX_ZERO);
                selector->AddDisplayLocator(displayId);
            }
            out.resultValue_ = StoreBackendObject(move(selector));
        };
        server.AddHandler("On.isBefore", genericRelativeBuilder);
        server.AddHandler("On.isAfter", genericRelativeBuilder);
        server.AddHandler("On.within", genericRelativeBuilder);
        server.AddHandler("On.inWindow", genericRelativeBuilder);
        server.AddHandler("On.inDisplay", genericRelativeBuilder);
    }

    static void RegisterUiDriverComponentFinders()
    {
        auto &server = FrontendApiServer::Get();
        auto genericFindWidgetHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            const auto driverRef = in.callerObjRef_;
            auto &driver = GetBackendObject<UiDriver>(driverRef);
            auto &selector = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
            UiOpArgs uiOpArgs;
            vector<unique_ptr<Widget>> recv;
            if (in.apiId_ == "Driver.waitForComponent") {
                uiOpArgs.waitWidgetMaxMs_ = ReadCallArg<int32_t>(in, INDEX_ONE);
                selector.SetWantMulti(false);
                auto result = driver.WaitForWidget(selector, uiOpArgs, out.exception_);
                if (result != nullptr) {
                    recv.emplace_back(move(result));
                }
            } else {
                selector.SetWantMulti(in.apiId_ == "Driver.findComponents");
                driver.FindWidgets(selector, recv, out.exception_);
            }
            if (out.exception_.code_ != NO_ERROR) {
                LOG_W("genericFindWidgetHandler has error: %{public}s", out.exception_.message_.c_str());
                return;
            }
            if (in.apiId_ == "Driver.assertComponentExist") {
                if (recv.empty()) { // widget-exist assertion failure, deliver exception
                    out.exception_.code_ = ERR_ASSERTION_FAILED;
                    out.exception_.message_ = "Component not exist matching: " + selector.Describe();
                }
            } else if (in.apiId_ == "Driver.findComponents") { // return widget array, maybe empty
                for (auto &ptr : recv) {
                    out.resultValue_.emplace_back(StoreBackendObject(move(ptr), driverRef));
                }
            } else if (recv.empty()) { // return first one or null
                out.resultValue_ = nullptr;
            } else {
                out.resultValue_ = StoreBackendObject(move(*(recv.begin())), driverRef);
            }
        };
        server.AddHandler("Driver.findComponent", genericFindWidgetHandler);
        server.AddHandler("Driver.findComponents", genericFindWidgetHandler);
        server.AddHandler("Driver.waitForComponent", genericFindWidgetHandler);
        server.AddHandler("Driver.assertComponentExist", genericFindWidgetHandler);
    }

    static void RegisterUiDriverWindowFinder()
    {
        auto findWindowHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            const auto driverRef = in.callerObjRef_;
            auto &driver = GetBackendObject<UiDriver>(driverRef);
            auto filterJson = ReadCallArg<json>(in, INDEX_ZERO);
            if (filterJson.empty()) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "WindowFilter cannot be empty");
                return;
            }
            auto matcher = [&filterJson](const Window &window) -> bool {
                bool match = true;
                if (filterJson.contains("bundleName")) {
                    match = match && (filterJson["bundleName"].get<string>() == window.bundleName_);
                }
                if (filterJson.contains("title")) {
                    match = match && (filterJson["title"].get<string>() == window.title_);
                }
                if (filterJson.contains("focused")) {
                    match = match && (filterJson["focused"].get<bool>() == window.focused_);
                }
                if (filterJson.contains("actived")) {
                    match = match && (filterJson["actived"].get<bool>() == window.actived_);
                }
                if (filterJson.contains("active")) {
                    match = match && (filterJson["active"].get<bool>() == window.actived_);
                }
                if (filterJson.contains("displayId")) {
                    match = match && (filterJson["displayId"].get<int32_t>() == window.displayId_);
                }
                return match;
            };
            auto window = driver.FindWindow(matcher, out.exception_);
            if (window == nullptr) {
                LOG_W("There is no match window by %{public}s", filterJson.dump().data());
                out.resultValue_ = nullptr;
            } else {
                out.resultValue_ = StoreBackendObject(move(window), driverRef);
            }
        };
        FrontendApiServer::Get().AddHandler("Driver.findWindow", findWindowHandler);
    }

    static void RegisterUiDriverMiscMethods1()
    {
        auto &server = FrontendApiServer::Get();
        auto create = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto driver = make_unique<UiDriver>();
            if (driver->CheckStatus(true, out.exception_)) {
                out.resultValue_ = StoreBackendObject(move(driver));
            }
        };
        server.AddHandler("Driver.create", create);

        auto createUIEventObserver = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto observer = make_unique<UIEventObserver>();
            out.resultValue_ = StoreBackendObject(move(observer), in.callerObjRef_);
        };
        server.AddHandler("Driver.createUIEventObserver", createUIEventObserver);

        auto delay = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto time = ReadCallArg<int32_t>(in, INDEX_ZERO);
            driver.DelayMs(time);
        };
        server.AddHandler("Driver.delayMs", delay);

        auto screenCap = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto null = json();
            auto rectJson = ReadCallArg<json>(in, INDEX_ONE, null);
            Rect rect = {0, 0, 0, 0};
            if (!rectJson.empty()) {
                rect = Rect(rectJson["left"], rectJson["right"], rectJson["top"], rectJson["bottom"]);
            }
            auto fd = ReadCallArg<uint32_t>(in, INDEX_ZERO);
            auto displayId = ReadCallArg<uint32_t>(in, INDEX_TWO, 0);
            driver.TakeScreenCap(fd, out.exception_, rect, displayId);
            out.resultValue_ = (out.exception_.code_ == NO_ERROR);
            (void) close(fd);
        };
        server.AddHandler("Driver.screenCap", screenCap);
        server.AddHandler("Driver.screenCapture", screenCap);
    }

    static void RegisterUiDriverMiscMethods2()
    {
        auto &server = FrontendApiServer::Get();
        auto pressBack = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            driver.TriggerKey(Back(), uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.pressBack", pressBack);
        auto pressHome = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto keyAction = CombinedKeys(KEYCODE_WIN, KEYCODE_D, KEYCODE_NONE);
            driver.TriggerKey(keyAction, uiOpArgs, out.exception_);
            driver.TriggerKey(Home(), uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.pressHome", pressHome);
        auto triggerKey = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto key = AnonymousSingleKey(ReadCallArg<int32_t>(in, INDEX_ZERO));
            driver.TriggerKey(key, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.triggerKey", triggerKey);
        auto triggerCombineKeys = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto key0 = ReadCallArg<int32_t>(in, INDEX_ZERO);
            auto key1 = ReadCallArg<int32_t>(in, INDEX_ONE);
            auto key2 = ReadCallArg<int32_t>(in, INDEX_TWO, KEYCODE_NONE);
            auto keyAction = CombinedKeys(key0, key1, key2);
            driver.TriggerKey(keyAction, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.triggerCombineKeys", triggerCombineKeys);
        auto inputText = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto pointJson = ReadCallArg<json>(in, INDEX_ZERO);
            auto displayId = ReadArgFromJson<int32_t>(pointJson, "displayId", 0);
            auto point = Point(pointJson["x"], pointJson["y"], displayId);
            auto text = ReadCallArg<string>(in, INDEX_ONE);
            auto touch = GenericClick(TouchOp::CLICK, point);
            driver.PerformTouch(touch, uiOpArgs, out.exception_);
            static constexpr uint32_t focusTimeMs = 500;
            driver.DelayMs(focusTimeMs);
            driver.InputText(text, out.exception_);
        };
        server.AddHandler("Driver.inputText", inputText);
    }

    static void RegisterUiEventObserverMethods()
    {
        static bool observerDelegateRegistered = false;
        static auto fowarder = std::make_shared<UiEventFowarder>();
        auto &server = FrontendApiServer::Get();
        auto once = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            auto event = ReadCallArg<string>(in, INDEX_ZERO);
            auto cbRef = ReadCallArg<string>(in, INDEX_ONE);
            fowarder->AddCallbackInfo(move(event), in.callerObjRef_, move(cbRef));
            if (!observerDelegateRegistered) {
                driver.RegisterUiEventListener(fowarder);
                observerDelegateRegistered = true;
            }
        };
        server.AddHandler("UIEventObserver.once", once);
    }

    static void CheckSwipeVelocityPps(UiOpArgs& args)
    {
        if (args.swipeVelocityPps_ < args.minSwipeVelocityPps_ || args.swipeVelocityPps_ > args.maxSwipeVelocityPps_) {
            LOG_W("The swipe velocity out of range");
            args.swipeVelocityPps_ = args.defaultSwipeVelocityPps_;
        }
    }

    static void TouchParamsConverts(const ApiCallInfo &in, Point &point0, Point &point1, UiOpArgs &uiOpArgs)
    {
        auto params = in.paramList_;
        if (params.size() == 1) {
            auto pointJson = ReadCallArg<json>(in, INDEX_ZERO);
            auto displayId = ReadArgFromJson<int32_t>(pointJson, "displayId", 0);
            point0 = Point(pointJson["x"], pointJson["y"], displayId);
            return;
        }
        if (params.at(1).type() != value_t::object) {
            if (params.size() == TWO) {
                point0 = Point(ReadCallArg<int32_t>(in, INDEX_ZERO), ReadCallArg<int32_t>(in, INDEX_ONE));
                return;
            } else {
                point0 = Point(ReadCallArg<int32_t>(in, INDEX_ZERO), ReadCallArg<int32_t>(in, INDEX_ONE));
                point1 = Point(ReadCallArg<int32_t>(in, INDEX_TWO), ReadCallArg<int32_t>(in, INDEX_THREE));
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_FOUR, uiOpArgs.swipeVelocityPps_);
                return;
            }
        } else {
            auto pointJson0 = ReadCallArg<json>(in, INDEX_ZERO);
            auto displayId0 = ReadArgFromJson<int32_t>(pointJson0, "displayId", 0);
            point0 = Point(pointJson0["x"], pointJson0["y"], displayId0);

            auto pointJson1 = ReadCallArg<json>(in, INDEX_ONE);
            auto displayId1 = ReadArgFromJson<int32_t>(pointJson1, "displayId", 0);
            point1 = Point(pointJson1["x"], pointJson1["y"], displayId1);
            uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_TWO, uiOpArgs.swipeVelocityPps_);
            return;
        }
    }

    static void RegisterUiDriverTouchOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericClick = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto point0 = Point(0, 0);
            auto point1 = Point(0, 0);
            UiOpArgs uiOpArgs;
            TouchParamsConverts(in, point0, point1, uiOpArgs);
            auto op = TouchOp::CLICK;
            if (in.apiId_ == "Driver.longClick") {
                op = TouchOp::LONG_CLICK;
            } else if (in.apiId_ == "Driver.doubleClick") {
                op = TouchOp::DOUBLE_CLICK_P;
            } else if (in.apiId_ == "Driver.swipe") {
                op = TouchOp::SWIPE;
            } else if (in.apiId_ == "Driver.drag") {
                op = TouchOp::DRAG;
            }
            CheckSwipeVelocityPps(uiOpArgs);
            if (op == TouchOp::SWIPE || op == TouchOp::DRAG) {
                auto touch = GenericSwipe(op, point0, point1);
                driver.PerformTouch(touch, uiOpArgs, out.exception_);
            } else {
                auto touch = GenericClick(op, point0);
                driver.PerformTouch(touch, uiOpArgs, out.exception_);
            }
        };
        server.AddHandler("Driver.click", genericClick);
        server.AddHandler("Driver.longClick", genericClick);
        server.AddHandler("Driver.doubleClick", genericClick);
        server.AddHandler("Driver.swipe", genericClick);
        server.AddHandler("Driver.drag", genericClick);
    }

    static bool CheckMultiPointerOperatorsPoint(const PointerMatrix& pointer)
    {
        for (uint32_t fingerIndex = 0; fingerIndex < pointer.GetFingers(); fingerIndex++) {
            for (uint32_t stepIndex = 0; stepIndex < pointer.GetSteps(); stepIndex++) {
                if (pointer.At(fingerIndex, stepIndex).flags_ != 1) {
                    return false;
                }
            }
        }
        return true;
    }

    static void CreateFlingPoint(Point &to, Point &from, Point screenSize, Direction direction)
    {
        to = Point(screenSize.px_ / INDEX_TWO, screenSize.py_ / INDEX_TWO);
        switch (direction) {
            case TO_LEFT:
                from.px_ = to.px_ - screenSize.px_ / INDEX_FOUR;
                from.py_ = to.py_;
                break;
            case TO_RIGHT:
                from.px_ = to.px_ + screenSize.px_ / INDEX_FOUR;
                from.py_ = to.py_;
                break;
            case TO_UP:
                from.px_ = to.px_;
                from.py_ = to.py_ - screenSize.py_ / INDEX_FOUR;
                break;
            case TO_DOWN:
                from.px_ = to.px_;
                from.py_ = to.py_ + screenSize.py_ / INDEX_FOUR;
                break;
            default:
                break;
        }
    }

    static void RegisterUiDriverFlingOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericFling = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto op = TouchOp::FLING;
            auto params = in.paramList_;
            Point from;
            Point to;
            if (params.size() != INDEX_FOUR) {
                auto displayId = ReadCallArg<int32_t>(in, INDEX_TWO, 0);
                auto screenSize = driver.GetDisplaySize(out.exception_, displayId);
                auto direction = ReadCallArg<Direction>(in, INDEX_ZERO);
                CreateFlingPoint(to, from, screenSize, direction);
                to.displayId_ = displayId;
                from.displayId_ = displayId;
                uiOpArgs.swipeStepsCounts_ = INDEX_TWO;
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_ONE);
            } else {
                auto pointJson0 = ReadCallArg<json>(in, INDEX_ZERO);
                auto pointJson1 = ReadCallArg<json>(in, INDEX_ONE);
                if (pointJson0.empty() || pointJson1.empty()) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Point cannot be empty");
                    return;
                }
                auto displayId0 = ReadArgFromJson<int32_t>(pointJson0, "displayId", 0);
                auto displayId1 = ReadArgFromJson<int32_t>(pointJson1, "displayId", 0);
                from = Point(pointJson0["x"], pointJson0["y"], displayId0);
                to = Point(pointJson1["x"], pointJson1["y"], displayId1);
                auto stepLength = ReadCallArg<uint32_t>(in, INDEX_TWO);
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_THREE);
                const int32_t distanceX = to.px_ - from.px_;
                const int32_t distanceY = to.py_ - from.py_;
                const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
                if (stepLength <= 0 || stepLength > distance) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "The stepLen is out of range");
                    return;
                }
                uiOpArgs.swipeStepsCounts_ = distance / stepLength;
            }
            CheckSwipeVelocityPps(uiOpArgs);
            auto touch = GenericSwipe(op, from, to);
            driver.PerformTouch(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.fling", genericFling);
    }

    static void RegisterUiDriverMultiPointerOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto multiPointerAction = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto &pointer = GetBackendObject<PointerMatrix>(ReadCallArg<string>(in, INDEX_ZERO));
            if (!CheckMultiPointerOperatorsPoint(pointer)) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "There is not all coordinate points are set");
                return;
            }
            UiOpArgs uiOpArgs;
            uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_ONE, uiOpArgs.swipeVelocityPps_);
            auto touch = MultiPointerAction(pointer);
            CheckSwipeVelocityPps(uiOpArgs);
            if (in.apiId_ == "Driver.injectMultiPointerAction") {
                driver.PerformTouch(touch, uiOpArgs, out.exception_);
                out.resultValue_ = (out.exception_.code_ == NO_ERROR);
            } else if (in.apiId_ == "Driver.injectPenPointerAction") {
                if (pointer.GetFingers() != 1) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Finger number must be 1 when injecting pen action");
                    return;
                }
                uiOpArgs.touchPressure_ = ReadCallArg<float>(in, INDEX_TWO, uiOpArgs.touchPressure_);
                driver.PerformPenTouch(touch, uiOpArgs, out.exception_);
            }
        };
        server.AddHandler("Driver.injectMultiPointerAction", multiPointerAction);
        server.AddHandler("Driver.injectPenPointerAction", multiPointerAction);
    }

    static void RegisterUiDriverMouseOperators1()
    {
        auto &server = FrontendApiServer::Get();
        auto mouseClick = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto pointJson = ReadCallArg<json>(in, INDEX_ZERO);
            auto displayId = ReadArgFromJson<int32_t>(pointJson, "displayId", 0);

            auto point = Point(pointJson["x"], pointJson["y"], displayId);
            auto button = ReadCallArg<MouseButton>(in, INDEX_ONE);
            auto key1 = ReadCallArg<int32_t>(in, INDEX_TWO, KEYCODE_NONE);
            auto key2 = ReadCallArg<int32_t>(in, INDEX_THREE, KEYCODE_NONE);
            auto op = TouchOp::CLICK;
            if (in.apiId_ == "Driver.mouseDoubleClick") {
                op = TouchOp::DOUBLE_CLICK_P;
            } else if (in.apiId_ == "Driver.mouseLongClick") {
                op = TouchOp::LONG_CLICK;
            }
            auto touch = MouseClick(op, point, button, key1, key2);
            driver.PerformMouseAction(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.mouseClick", mouseClick);
        server.AddHandler("Driver.mouseDoubleClick", mouseClick);
        server.AddHandler("Driver.mouseLongClick", mouseClick);
        auto mouseMoveTo = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto pointJson = ReadCallArg<json>(in, INDEX_ZERO);
            UiOpArgs uiOpArgs;
            auto displayId = ReadArgFromJson<int32_t>(pointJson, "displayId", 0);
            auto point = Point(pointJson["x"], pointJson["y"], displayId);
            auto touch = MouseMoveTo(point);
            driver.PerformMouseAction(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.mouseMoveTo", mouseMoveTo);
    }

    static void RegisterUiDriverMouseOperators2()
    {
        auto &server = FrontendApiServer::Get();
        auto mouseSwipe = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto pointJson1 = ReadCallArg<json>(in, INDEX_ZERO);
            auto pointJson2 = ReadCallArg<json>(in, INDEX_ONE);
            auto displayId1 = ReadArgFromJson<int32_t>(pointJson1, "displayId", 0);
            auto displayId2 = ReadArgFromJson<int32_t>(pointJson2, "displayId", 0);
            auto from = Point(pointJson1["x"], pointJson1["y"], displayId1);
            auto to = Point(pointJson2["x"], pointJson2["y"], displayId2);
            UiOpArgs uiOpArgs;
            uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_TWO, uiOpArgs.swipeVelocityPps_);
            CheckSwipeVelocityPps(uiOpArgs);
            auto op = TouchOp::SWIPE;
            if (in.apiId_ == "Driver.mouseDrag") {
                op = TouchOp::DRAG;
            }
            auto touch = MouseSwipe(op, from, to);
            driver.PerformMouseAction(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.mouseMoveWithTrack", mouseSwipe);
        server.AddHandler("Driver.mouseDrag", mouseSwipe);
        auto mouseScroll = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto pointJson = ReadCallArg<json>(in, INDEX_ZERO);
            auto displayId = ReadArgFromJson<int32_t>(pointJson, "displayId", 0);
            auto point = Point(pointJson["x"], pointJson["y"], displayId);
            auto scrollValue = ReadCallArg<int32_t>(in, INDEX_TWO);
            auto adown = ReadCallArg<bool>(in, INDEX_ONE);
            scrollValue = adown ? scrollValue : -scrollValue;
            auto key1 = ReadCallArg<int32_t>(in, INDEX_THREE, KEYCODE_NONE);
            auto key2 = ReadCallArg<int32_t>(in, INDEX_FOUR, KEYCODE_NONE);
            const uint32_t maxScrollSpeed = 500;
            const uint32_t defaultScrollSpeed = 20;
            auto speed = ReadCallArg<uint32_t>(in, INDEX_FIVE, defaultScrollSpeed);
            if (speed < 1 || speed > maxScrollSpeed) {
                speed = defaultScrollSpeed;
            }
            auto touch = MouseScroll(point, scrollValue, key1, key2, speed);
            driver.PerformMouseAction(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.mouseScroll", mouseScroll);
    }

    static void RegisterUiDriverTouchPadOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto touchPadMultiFingerSwipe = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto fingers = ReadCallArg<int32_t>(in, INDEX_ZERO);
            if (fingers < 3 || fingers > 4) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Number of illegal fingers");
                return;
            }
            auto direction = ReadCallArg<Direction>(in, INDEX_ONE);
            if (direction < TO_LEFT || direction > TO_DOWN) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Illegal direction");
                return;
            }
            auto stay = false;
            auto speed = uiOpArgs.defaultTouchPadSwipeVelocityPps_;
            auto touchPadSwipeOptions = ReadCallArg<json>(in, INDEX_TWO, json());
            if (!touchPadSwipeOptions.empty()) {
                if (touchPadSwipeOptions.contains("stay") && touchPadSwipeOptions["stay"].is_boolean()) {
                    stay = touchPadSwipeOptions["stay"];
                }
                if (touchPadSwipeOptions.contains("speed") && touchPadSwipeOptions["speed"].is_number()) {
                    speed = touchPadSwipeOptions["speed"];
                }
                if (speed < uiOpArgs.minSwipeVelocityPps_ || speed > uiOpArgs.maxSwipeVelocityPps_) {
                    LOG_W("The swipe velocity out of range");
                    speed = uiOpArgs.defaultTouchPadSwipeVelocityPps_;
                }
            }
            uiOpArgs.swipeVelocityPps_ = speed;
            auto touch = TouchPadAction(fingers, direction, stay);
            driver.PerformTouchPadAction(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("Driver.touchPadMultiFingerSwipe", touchPadMultiFingerSwipe);
    }

    static void RegisterUiDriverPenOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericPenAction = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto point0Json = ReadCallArg<json>(in, INDEX_ZERO);
            auto displayId = ReadArgFromJson<int32_t>(point0Json, "displayId", 0);
            const auto point0 = Point(point0Json["x"], point0Json["y"], displayId);
            const auto screenSize = driver.GetDisplaySize(out.exception_, displayId);
            const auto screen = Rect(0, screenSize.px_, 0, screenSize.py_);
            if (!RectAlgorithm::IsInnerPoint(screen, point0)) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "The point is not on the screen");
                return;
            }
            auto point1 = Point(0, 0);
            auto op = TouchOp::CLICK;
            if (in.apiId_ == "Driver.penLongClick") {
                op = TouchOp::LONG_CLICK;
                uiOpArgs.touchPressure_ = ReadCallArg<float>(in, INDEX_ONE, uiOpArgs.touchPressure_);
            } else if (in.apiId_ == "Driver.penDoubleClick") {
                op = TouchOp::DOUBLE_CLICK_P;
            } else if (in.apiId_ == "Driver.penSwipe") {
                op = TouchOp::SWIPE;
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_TWO, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                auto point1Json = ReadCallArg<json>(in, INDEX_ONE);
                auto displayId1 = ReadArgFromJson<int32_t>(point1Json, "displayId", 0);
                point1 = Point(point1Json["x"], point1Json["y"], displayId1);
                if (!RectAlgorithm::IsInnerPoint(screen, point1)) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "The point is not on the screen");
                    return;
                }
                uiOpArgs.touchPressure_ = ReadCallArg<float>(in, INDEX_THREE, uiOpArgs.touchPressure_);
            }
            if (op == TouchOp::SWIPE) {
                auto touch = GenericSwipe(op, point0, point1);
                driver.PerformPenTouch(touch, uiOpArgs, out.exception_);
            } else {
                auto touch = GenericClick(op, point0);
                driver.PerformPenTouch(touch, uiOpArgs, out.exception_);
            }
        };
        server.AddHandler("Driver.penClick", genericPenAction);
        server.AddHandler("Driver.penLongClick", genericPenAction);
        server.AddHandler("Driver.penDoubleClick", genericPenAction);
        server.AddHandler("Driver.penSwipe", genericPenAction);
    }

    static void RegisterUiDriverDisplayOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericDisplayOperator = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            if (in.apiId_ == "Driver.setDisplayRotation") {
                auto rotation = ReadCallArg<DisplayRotation>(in, INDEX_ZERO);
                driver.SetDisplayRotation(rotation, out.exception_);
            } else if (in.apiId_ == "Driver.getDisplayRotation") {
                auto displayId = ReadCallArg<int32_t>(in, INDEX_ZERO, 0);
                out.resultValue_ = driver.GetDisplayRotation(out.exception_, displayId);
            } else if (in.apiId_ == "Driver.setDisplayRotationEnabled") {
                auto enabled = ReadCallArg<bool>(in, INDEX_ZERO);
                driver.SetDisplayRotationEnabled(enabled, out.exception_);
            } else if (in.apiId_ == "Driver.waitForIdle") {
                auto idleTime = ReadCallArg<int32_t>(in, INDEX_ZERO);
                auto timeout = ReadCallArg<int32_t>(in, INDEX_ONE);
                out.resultValue_ = driver.WaitForUiSteady(idleTime, timeout, out.exception_);
            } else if (in.apiId_ == "Driver.wakeUpDisplay") {
                driver.WakeUpDisplay(out.exception_);
            } else if (in.apiId_ == "Driver.getDisplaySize") {
                auto displayId = ReadCallArg<int32_t>(in, INDEX_ZERO, 0);
                auto result = driver.GetDisplaySize(out.exception_, displayId);
                json data;
                data["x"] = result.px_;
                data["y"] = result.py_;
                out.resultValue_ = data;
            } else if (in.apiId_ == "Driver.getDisplayDensity") {
                auto displayId = ReadCallArg<int32_t>(in, INDEX_ZERO, 0);
                auto result = driver.GetDisplayDensity(out.exception_, displayId);
                json data;
                data["x"] = result.px_;
                data["y"] = result.py_;
                out.resultValue_ = data;
            }
        };
        server.AddHandler("Driver.setDisplayRotation", genericDisplayOperator);
        server.AddHandler("Driver.getDisplayRotation", genericDisplayOperator);
        server.AddHandler("Driver.setDisplayRotationEnabled", genericDisplayOperator);
        server.AddHandler("Driver.waitForIdle", genericDisplayOperator);
        server.AddHandler("Driver.wakeUpDisplay", genericDisplayOperator);
        server.AddHandler("Driver.getDisplaySize", genericDisplayOperator);
        server.AddHandler("Driver.getDisplayDensity", genericDisplayOperator);
    }

    template <UiAttr kAttr, bool kString = false>
    static void GenericComponentAttrGetter(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        constexpr auto attrName = ATTR_NAMES[kAttr];
        auto &image = GetBackendObject<Widget>(in.callerObjRef_);
        auto &driver = GetBoundUiDriver(in.callerObjRef_);
        auto snapshot = driver.RetrieveWidget(image, out.exception_);
        if (out.exception_.code_ != NO_ERROR) {
            out.resultValue_ = nullptr; // exception, return null
            return;
        }
        if (attrName == ATTR_NAMES[UiAttr::BOUNDSCENTER]) { // getBoundsCenter
            const auto bounds = snapshot->GetBounds();
            json data;
            data["x"] = bounds.GetCenterX();
            data["y"] = bounds.GetCenterY();
            data["displayId"] = snapshot->GetDisplayId();
            out.resultValue_ = data;
            return;
        }
        if (attrName == ATTR_NAMES[UiAttr::BOUNDS]) { // getBounds
            const auto bounds = snapshot->GetBounds();
            json data;
            data["left"] = bounds.left_;
            data["top"] = bounds.top_;
            data["right"] = bounds.right_;
            data["bottom"] = bounds.bottom_;
            data["displayId"] = snapshot->GetDisplayId();
            out.resultValue_ = data;
            return;
        }
        if (attrName == ATTR_NAMES[UiAttr::DISPLAY_ID]) {
            out.resultValue_ = snapshot->GetDisplayId();
            return;
        }
        // convert value-string to json value of target type
        auto attrValue = snapshot->GetAttr(kAttr);
        if (attrValue == "NA") {
            out.resultValue_ = nullptr; // no such attribute, return null
        } else if (kString) {
            out.resultValue_ = attrValue;
        } else {
            out.resultValue_ = nlohmann::json::parse(attrValue);
        }
    }

static void RegisterExtensionHandler()
{
    auto &server = FrontendApiServer::Get();
    auto genericOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
        auto &image = GetBackendObject<Widget>(in.callerObjRef_);
        auto &driver = GetBoundUiDriver(in.callerObjRef_);
        auto snapshot = driver.RetrieveWidget(image, out.exception_);
        if (out.exception_.code_ != NO_ERROR || snapshot == nullptr) {
            out.resultValue_ = nullptr; // exception, return null
            return;
        }
        json data;
        for (auto i = 0; i < UiAttr::HIERARCHY + 1; ++i) {
            if (i == UiAttr::BOUNDS) {
                const auto bounds = snapshot->GetBounds();
                json rect;
                rect["left"] = bounds.left_;
                rect["top"] = bounds.top_;
                rect["right"] = bounds.right_;
                rect["bottom"] = bounds.bottom_;
                data["bounds"] = rect;
                continue;
            }
            data[ATTR_NAMES[i].data()] = snapshot->GetAttr(static_cast<UiAttr> (i));
        }
        out.resultValue_ = data;
    };
    server.AddHandler("Component.getAllProperties", genericOperationHandler);

    auto genericSetModeHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
        auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
        auto mode = ReadCallArg<uint32_t>(in, INDEX_ZERO);
        DCHECK(mode < AamsWorkMode::END);
        driver.SetAamsWorkMode(static_cast<AamsWorkMode>(mode));
    };
    server.AddHandler("Driver.SetAamsWorkMode", genericSetModeHandler);
}

    static void RegisterUiComponentAttrGetters()
    {
        auto &server = FrontendApiServer::Get();
        server.AddHandler("Component.getAccessibilityId", GenericComponentAttrGetter<UiAttr::ACCESSIBILITY_ID>);
        server.AddHandler("Component.getText", GenericComponentAttrGetter<UiAttr::TEXT, true>);
        server.AddHandler("Component.getHint", GenericComponentAttrGetter<UiAttr::HINT, true>);
        server.AddHandler("Component.getDescription", GenericComponentAttrGetter<UiAttr::DESCRIPTION, true>);
        server.AddHandler("Component.getId", GenericComponentAttrGetter<UiAttr::ID, true>);
        server.AddHandler("Component.getType", GenericComponentAttrGetter<UiAttr::TYPE, true>);
        server.AddHandler("Component.isEnabled", GenericComponentAttrGetter<UiAttr::ENABLED>);
        server.AddHandler("Component.isFocused", GenericComponentAttrGetter<UiAttr::FOCUSED>);
        server.AddHandler("Component.isSelected", GenericComponentAttrGetter<UiAttr::SELECTED>);
        server.AddHandler("Component.isClickable", GenericComponentAttrGetter<UiAttr::CLICKABLE>);
        server.AddHandler("Component.isLongClickable", GenericComponentAttrGetter<UiAttr::LONG_CLICKABLE>);
        server.AddHandler("Component.isScrollable", GenericComponentAttrGetter<UiAttr::SCROLLABLE>);
        server.AddHandler("Component.isCheckable", GenericComponentAttrGetter<UiAttr::CHECKABLE>);
        server.AddHandler("Component.isChecked", GenericComponentAttrGetter<UiAttr::CHECKED>);
        server.AddHandler("Component.getBounds", GenericComponentAttrGetter<UiAttr::BOUNDS>);
        server.AddHandler("Component.getBoundsCenter", GenericComponentAttrGetter<UiAttr::BOUNDSCENTER>);
        server.AddHandler("Component.getDisplayId", GenericComponentAttrGetter<UiAttr::DISPLAY_ID>);
    }

    static void RegisterUiComponentOperators1()
    {
        auto &server = FrontendApiServer::Get();
        auto genericOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto wOp = WidgetOperator(driver, widget, uiOpArgs);
            if (in.apiId_ == "Component.click") {
                wOp.GenericClick(TouchOp::CLICK, out.exception_);
            } else if (in.apiId_ == "Component.longClick") {
                wOp.GenericClick(TouchOp::LONG_CLICK, out.exception_);
            } else if (in.apiId_ == "Component.doubleClick") {
                wOp.GenericClick(TouchOp::DOUBLE_CLICK_P, out.exception_);
            } else if (in.apiId_ == "Component.scrollToTop") {
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_ZERO, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                wOp.ScrollToEnd(true, out.exception_);
            } else if (in.apiId_ == "Component.scrollToBottom") {
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_ZERO, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                wOp.ScrollToEnd(false, out.exception_);
            } else if (in.apiId_ == "Component.dragTo") {
                auto &widgetTo = GetBackendObject<Widget>(ReadCallArg<string>(in, INDEX_ZERO));
                wOp.DragIntoWidget(widgetTo, out.exception_);
            } else if (in.apiId_ == "Component.inputText") {
                wOp.InputText(ReadCallArg<string>(in, INDEX_ZERO), out.exception_);
            } else if (in.apiId_ == "Component.clearText") {
                wOp.InputText("", out.exception_);
            }
        };
        server.AddHandler("Component.click", genericOperationHandler);
        server.AddHandler("Component.longClick", genericOperationHandler);
        server.AddHandler("Component.doubleClick", genericOperationHandler);
        server.AddHandler("Component.scrollToTop", genericOperationHandler);
        server.AddHandler("Component.scrollToBottom", genericOperationHandler);
        server.AddHandler("Component.dragTo", genericOperationHandler);
        server.AddHandler("Component.inputText", genericOperationHandler);
        server.AddHandler("Component.clearText", genericOperationHandler);
    }

    static void RegisterUiComponentOperators2()
    {
        auto &server = FrontendApiServer::Get();
        auto genericOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto wOp = WidgetOperator(driver, widget, uiOpArgs);
            if (in.apiId_ == "Component.pinchOut") {
                auto pinchScale = ReadCallArg<float_t>(in, INDEX_ZERO);
                if (pinchScale < 1) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Expect integer which gerater 1");
                }
                wOp.PinchWidget(pinchScale, out.exception_);
            } else if (in.apiId_ == "Component.pinchIn") {
                auto pinchScale = ReadCallArg<float_t>(in, INDEX_ZERO);
                if (pinchScale > 1) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Expect integer which ranges from 0 to 1.");
                }
                wOp.PinchWidget(pinchScale, out.exception_);
            } else if (in.apiId_ == "Component.scrollSearch") {
                auto &selector = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                bool vertical = ReadCallArg<bool>(in, INDEX_ONE, true);
                uiOpArgs.scrollWidgetDeadZone_ = ReadCallArg<int32_t>(in, INDEX_TWO, 80);
                if (!wOp.CheckDeadZone(vertical, out.exception_)) {
                    return;
                }
                auto res = wOp.ScrollFindWidget(selector, vertical, out.exception_);
                if (res != nullptr) {
                    out.resultValue_ = StoreBackendObject(move(res), sDriverBindingMap.find(in.callerObjRef_)->second);
                }
            }
        };
        server.AddHandler("Component.pinchOut", genericOperationHandler);
        server.AddHandler("Component.pinchIn", genericOperationHandler);
        server.AddHandler("Component.scrollSearch", genericOperationHandler);
    }

    static void RegisterUiWindowAttrGetters()
    {
        auto &server = FrontendApiServer::Get();
        auto genericGetter = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &window = GetBackendObject<Window>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            auto snapshot = driver.RetrieveWindow(window, out.exception_);
            if (out.exception_.code_ != NO_ERROR) {
                out.resultValue_ = nullptr; // exception, return null
                return;
            }
            if (in.apiId_ == "UiWindow.getBundleName") {
                out.resultValue_ = snapshot->bundleName_;
            } else if (in.apiId_ == "UiWindow.getBounds") {
                json data;
                data["left"] = snapshot->bounds_.left_;
                data["top"] = snapshot->bounds_.top_;
                data["right"] = snapshot->bounds_.right_;
                data["bottom"] = snapshot->bounds_.bottom_;
                data["displayId"] = snapshot->displayId_;
                out.resultValue_ = data;
            } else if (in.apiId_ == "UiWindow.getTitle") {
                out.resultValue_ = snapshot->title_;
            } else if (in.apiId_ == "UiWindow.getWindowMode") {
                out.resultValue_ = (uint8_t)(snapshot->mode_ - 1);
            } else if (in.apiId_ == "UiWindow.isFocused") {
                out.resultValue_ = snapshot->focused_;
            } else if (in.apiId_ == "UiWindow.isActive") {
                out.resultValue_ = snapshot->actived_;
            } else if (in.apiId_ == "UiWindow.getDisplayId") {
                out.resultValue_ = snapshot->displayId_;
            }
        };
        server.AddHandler("UiWindow.getBundleName", genericGetter);
        server.AddHandler("UiWindow.getBounds", genericGetter);
        server.AddHandler("UiWindow.getTitle", genericGetter);
        server.AddHandler("UiWindow.getWindowMode", genericGetter);
        server.AddHandler("UiWindow.isFocused", genericGetter);
        server.AddHandler("UiWindow.isActive", genericGetter);
        server.AddHandler("UiWindow.getDisplayId", genericGetter);
    }

    static void RegisterUiWindowOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericWinOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &image = GetBackendObject<Window>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            auto window = driver.RetrieveWindow(image, out.exception_);
            if (out.exception_.code_ != NO_ERROR || window == nullptr) {
                return;
            }
            UiOpArgs uiOpArgs;
            auto wOp = WindowOperator(driver, *window, uiOpArgs);
            auto action = in.apiId_;
            if (action == "UiWindow.resize") {
                auto width = ReadCallArg<int32_t>(in, INDEX_ZERO);
                auto highth = ReadCallArg<int32_t>(in, INDEX_ONE);
                auto direction = ReadCallArg<ResizeDirection>(in, INDEX_TWO);
                if ((((direction == LEFT) || (direction == RIGHT)) && highth != window->bounds_.GetHeight()) ||
                    (((direction == D_UP) || (direction == D_DOWN)) && width != window->bounds_.GetWidth())) {
                    out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "Resize cannot be done in this direction");
                    return;
                }
                wOp.Resize(width, highth, direction, out);
            } else if (action == "UiWindow.focus") {
                wOp.Focus(out);
            }
        };
        server.AddHandler("UiWindow.focus", genericWinOperationHandler);
        server.AddHandler("UiWindow.resize", genericWinOperationHandler);
    }

    static void RegisterUiWinBarOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericWinBarOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &image = GetBackendObject<Window>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            auto window = driver.RetrieveWindow(image, out.exception_);
            if (out.exception_.code_ != NO_ERROR || window == nullptr) {
                return;
            }
            UiOpArgs uiOpArgs;
            auto wOp = WindowOperator(driver, *window, uiOpArgs);
            auto action = in.apiId_;
            if (window->decoratorEnabled_) {
                if (action == "UiWindow.split") {
                    wOp.Split(out);
                } else if (action == "UiWindow.maximize") {
                    wOp.Maximize(out);
                } else if (action == "UiWindow.resume") {
                    wOp.Resume(out);
                } else if (action == "UiWindow.minimize") {
                    wOp.Minimize(out);
                } else if (action == "UiWindow.close") {
                    wOp.Close(out);
                } else if (action == "UiWindow.moveTo") {
                    auto endX = ReadCallArg<uint32_t>(in, INDEX_ZERO);
                    auto endY = ReadCallArg<uint32_t>(in, INDEX_ONE);
                    wOp.MoveTo(endX, endY, out);
                }
            } else {
                out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            }
        };
        server.AddHandler("UiWindow.split", genericWinBarOperationHandler);
        server.AddHandler("UiWindow.maximize", genericWinBarOperationHandler);
        server.AddHandler("UiWindow.resume", genericWinBarOperationHandler);
        server.AddHandler("UiWindow.minimize", genericWinBarOperationHandler);
        server.AddHandler("UiWindow.close", genericWinBarOperationHandler);
        server.AddHandler("UiWindow.moveTo", genericWinBarOperationHandler);
    }

    static void RegisterPointerMatrixOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto create = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            UiOpArgs uiOpArgs;
            auto finger = ReadCallArg<uint32_t>(in, INDEX_ZERO);
            if (finger < 1 || finger > uiOpArgs.maxMultiTouchFingers) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Number of illegal fingers");
                return;
            }
            auto step = ReadCallArg<uint32_t>(in, INDEX_ONE);
            if (step < 1 || step > uiOpArgs.maxMultiTouchSteps) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Number of illegal steps");
                return;
            }
            out.resultValue_ = StoreBackendObject(make_unique<PointerMatrix>(finger, step));
        };
        server.AddHandler("PointerMatrix.create", create);

        auto setPoint = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &pointer = GetBackendObject<PointerMatrix>(in.callerObjRef_);
            auto finger = ReadCallArg<uint32_t>(in, INDEX_ZERO);
            if (finger < 0 || finger >= pointer.GetFingers()) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Number of illegal fingers");
                return;
            }
            auto step = ReadCallArg<uint32_t>(in, INDEX_ONE);
            if (step < 0 || step >= pointer.GetSteps()) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Number of illegal steps");
                return;
            }
            auto pointJson = ReadCallArg<json>(in, INDEX_TWO);
            if (pointJson.empty()) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Point cannot be empty");
                return;
            }
            auto displayId = ReadArgFromJson<int32_t>(pointJson, "displayId", 0);
            const auto point = Point(pointJson["x"], pointJson["y"], displayId);
            pointer.At(finger, step).point_ = point;
            pointer.At(finger, step).flags_ = 1;
        };
        server.AddHandler("PointerMatrix.setPoint", setPoint);
    }

    /** Register frontendApiHandlers and preprocessors on startup.*/
    __attribute__((constructor)) static void RegisterApiHandlers()
    {
        auto &server = FrontendApiServer::Get();
        server.AddCommonPreprocessor("APiCallInfoChecker", APiCallInfoChecker);
        server.AddHandler("BackendObjectsCleaner", BackendObjectsCleaner);
        RegisterOnBuilders();
        RegisterUiDriverComponentFinders();
        RegisterUiDriverWindowFinder();
        RegisterUiDriverMiscMethods1();
        RegisterUiDriverMiscMethods2();
        RegisterUiDriverTouchOperators();
        RegisterUiComponentAttrGetters();
        RegisterUiComponentOperators1();
        RegisterUiComponentOperators2();
        RegisterUiWindowAttrGetters();
        RegisterUiWindowOperators();
        RegisterUiWinBarOperators();
        RegisterPointerMatrixOperators();
        RegisterUiDriverFlingOperators();
        RegisterUiDriverMultiPointerOperators();
        RegisterUiDriverDisplayOperators();
        RegisterUiDriverMouseOperators1();
        RegisterUiDriverMouseOperators2();
        RegisterUiEventObserverMethods();
        RegisterExtensionHandler();
        RegisterUiDriverTouchPadOperators();
        RegisterUiDriverPenOperators();
    }
} // namespace OHOS::uitest
