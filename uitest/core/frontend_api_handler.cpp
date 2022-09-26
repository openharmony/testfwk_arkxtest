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
#include "ui_driver.h"
#include "widget_operator.h"
#include "window_operator.h"
#include "frontend_api_handler.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    using namespace nlohmann::detail;
 
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

    bool FrontendApiServer::HasHandlerFor(std::string_view apiId) const
    {
        return handlers_.find(string(apiId)) != handlers_.end();
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
        auto find = handlers_.find(in.apiId_);
        if (find == handlers_.end()) {
            out.exception_ = ApiCallErr(ErrCode::INTERNAL_ERROR, "No handler found for api '" + in.apiId_ + "'");
            return;
        }
        try {
            for (auto &[name, processor] : commonPreprocessors_) {
                processor(in, out);
                if (out.exception_.code_ != ErrCode::NO_ERROR) {
                    out.exception_.message_ = out.exception_.message_ + "(PreProsessing: " + name + ")";
                    return; // error during pre-proccessing, abort
                }
            }
        } catch (std::exception &ex) {
            out.exception_ = ApiCallErr(ErrCode::INTERNAL_ERROR, "Preprocessor failed: " + string(ex.what()));
        }
        try {
            find->second(in, out);
        } catch (std::exception &ex) {
            // catch possible json-parsing error
            out.exception_ = ApiCallErr(ErrCode::INTERNAL_ERROR, "Handler failed: " + string(ex.what()));
        }
    }

    void ApiTransact(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        LOG_D("Begin to invoke api '%{public}s'", in.apiId_.data());
        FrontendApiServer::Get().Call(in, out);
    }

    /** Backend objects cache.*/
    static map<string, unique_ptr<BackendClass>> sBackendObjects;
    /** UiDriver binding map.*/
    static map<string, string> sDriverBindingMap;
    /** API argument type list map.*/
    static map<string, pair<vector<string>, bool>> sApiArgTypesMap;

    static void ParseMethodSignature(string_view signature, vector<string> &types, bool &defArg)
    {
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
                DCHECK(defArgCount == 1); // only one defaultArg allowed
            } else if (ch == ',' || ch == '?' || ch == ')') {
                if (tokenLen > 0) {
                    token = string_view(buf, tokenLen);
                    DCHECK(find(DATA_TYPE_SCOPE.begin(), DATA_TYPE_SCOPE.end(), token) != DATA_TYPE_SCOPE.end());
                    types.emplace_back(token);
                }
                tokenLen = 0; // consume token and reset buffer
                if (ch == ')') {
                    break; // end parsing
                }
            }
        }
        defArg = defArgCount > 0;
    }

    /** Parse frontend method definitions to collect type information.*/
    static void ParseFrontendMethodsSignature()
    {
        for (auto classDef : FRONTEND_CLASS_DEFS) {
            if (classDef->methods_ == nullptr || classDef->methodCount_ <= 0) {
                continue;
            }
            for (size_t idx = 0; idx < classDef->methodCount_; idx++) {
                auto methodDef = classDef->methods_[idx];
                auto paramTypes = vector<string>();
                auto hasDefaultArg = false;
                ParseMethodSignature(methodDef.signature_, paramTypes, hasDefaultArg);
                sApiArgTypesMap.insert(make_pair(string(methodDef.name_), make_pair(paramTypes, hasDefaultArg)));
            }
        }
    }

#define CHECK_CALL_ARG(condition, code, message, error) \
    if (!(condition)) {                                 \
        error = ApiCallErr((code), (message));          \
        return;                                         \
    }

    /** Check if the json value represents and illegal data of expected type.*/
    static void CheckCallArgType(string_view expect, const json &value, ApiCallErr &error)
    {
        const auto type = value.type();
        const auto isInteger = type == value_t::number_integer || type == value_t::number_unsigned;
        auto begin0 = FRONTEND_CLASS_DEFS.begin();
        auto end0 = FRONTEND_CLASS_DEFS.end();
        auto begin1 = FRONTEND_JSON_DEFS.begin();
        auto end1 = FRONTEND_JSON_DEFS.end();
        auto find0 = find_if(begin0, end0, [&expect](const FrontEndClassDef *def) { return def->name_ == expect; });
        auto find1 = find_if(begin1, end1, [&expect](const FrontEndJsonDef *def) { return def->name_ == expect; });
        if (expect == "int") {
            CHECK_CALL_ARG(isInteger, USAGE_ERROR, "Expect interger", error);
        } else if (expect == "float") {
            CHECK_CALL_ARG(isInteger || type == value_t::number_float, USAGE_ERROR, "Expect float", error);
        } else if (expect == "bool") {
            CHECK_CALL_ARG(type == value_t::boolean, USAGE_ERROR, "Expect boolean", error);
        } else if (expect == "string") {
            CHECK_CALL_ARG(type == value_t::string, USAGE_ERROR, "Expect string", error);
        } else if (find0 != end0) {
            CHECK_CALL_ARG(type == value_t::string, USAGE_ERROR, "Expect " + string(expect), error);
            const auto findRef = sBackendObjects.find(value.get<string>());
            CHECK_CALL_ARG(findRef != sBackendObjects.end(), INTERNAL_ERROR, "Bad object ref", error);
        } else if (find1 != end1) {
            CHECK_CALL_ARG(type == value_t::object, USAGE_ERROR, "Expect " + string(expect), error);
            auto copy = value;
            for (size_t idx = 0; idx < (*find1)->propCount_; idx++) {
                auto def = (*find1)->props_ + idx;
                const auto propName = string(def->name_);
                if (!value.contains(propName)) {
                    CHECK_CALL_ARG(!(def->required_), USAGE_ERROR, "Missing property " + propName, error);
                    continue;
                }
                copy.erase(propName);
                // check json property value type recursive
                CheckCallArgType(def->type_, value[propName], error);
                if (error.code_ != NO_ERROR) {
                    error.message_ = "Illegal value of property '" + propName + "': " + error.message_;
                    return;
                }
            }
            CHECK_CALL_ARG(copy.empty(), USAGE_ERROR, "Illegal property of " + string(expect), error);
        } else {
            CHECK_CALL_ARG(false, INTERNAL_ERROR, "Unknown target type " + string(expect), error);
        }
    }

    /** Checks ApiCallInfo data, deliver exception and abort invocation if check fails.*/
    static void APiCallInfoChecker(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        // return nullptr by default
        out.resultValue_ = nullptr;
        if (sApiArgTypesMap.empty()) {
            ParseFrontendMethodsSignature();
        }
        const auto find = sApiArgTypesMap.find(in.apiId_);
        if (find == sApiArgTypesMap.end()) {
            return;
        }
        const auto &types = find->second.first;
        const auto argSupportDefault = find->second.second;
        // check argument count
        const auto maxArgc = types.size();
        const auto minArgc = argSupportDefault ? maxArgc - 1 : maxArgc;
        const auto argc = in.paramList_.size();
        CHECK_CALL_ARG(argc <= maxArgc && argc >= minArgc, USAGE_ERROR, "Illegal argument count", out.exception_);
        // check argument type
        for (size_t idx = 0; idx < argc; idx++) {
            CheckCallArgType(types.at(idx), in.paramList_.at(idx), out.exception_);
            if (out.exception_.code_ != NO_ERROR) {
                out.exception_.message_ = "Check arg" + to_string(idx) + " failed: " + out.exception_.message_;
                return;
            }
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
        return in.paramList_.at(index).get<T>();
    }

    template <UiAttr kAttr, typename T> static void GenericByAttrBuilder(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        const auto attrName = ATTR_NAMES[kAttr];
        // always create and return a new selector
        auto selector = make_unique<WidgetSelector>();
        if (in.callerObjRef_ != REF_SEED_BY) { // copy-construct from the caller if it's not seed
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
        auto matcher = WidgetAttrMatcher(attrName, testValue, static_cast<ValueMatchPattern>(matchPattern));
        selector->AddMatcher(matcher);
        out.resultValue_ = StoreBackendObject(move(selector));
    }

    static void RegisterByBuilders()
    {
        auto &server = FrontendApiServer::Get();
        server.AddHandler("By.id", GenericByAttrBuilder<UiAttr::ID, int32_t>);
        server.AddHandler("By.text", GenericByAttrBuilder<UiAttr::TEXT, string>);
        server.AddHandler("By.key", GenericByAttrBuilder<UiAttr::KEY, string>);
        server.AddHandler("By.type", GenericByAttrBuilder<UiAttr::TYPE, string>);
        server.AddHandler("By.enabled", GenericByAttrBuilder<UiAttr::ENABLED, bool>);
        server.AddHandler("By.focused", GenericByAttrBuilder<UiAttr::FOCUSED, bool>);
        server.AddHandler("By.selected", GenericByAttrBuilder<UiAttr::SELECTED, bool>);
        server.AddHandler("By.clickable", GenericByAttrBuilder<UiAttr::CLICKABLE, bool>);
        server.AddHandler("By.longClickable", GenericByAttrBuilder<UiAttr::LONG_CLICKABLE, bool>);
        server.AddHandler("By.scrollable", GenericByAttrBuilder<UiAttr::SCROLLABLE, bool>);
        server.AddHandler("By.checkable", GenericByAttrBuilder<UiAttr::CHECKABLE, bool>);
        server.AddHandler("By.checked", GenericByAttrBuilder<UiAttr::CHECKED, bool>);

        auto genericRelativeBuilder = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            const auto attrName = in.apiId_.substr(BY_DEF.name_.length() + 1); // By.xxx()->xxx
            // always create and return a new selector
            auto selector = make_unique<WidgetSelector>();
            if (in.callerObjRef_ != REF_SEED_BY) { // copy-construct from the caller if it's not seed
                *selector = GetBackendObject<WidgetSelector>(in.callerObjRef_);
            }
            if (attrName == "isBefore") {
                auto &that = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                selector->AddRearLocator(that, out.exception_);
            } else {
                auto &that = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                selector->AddFrontLocator(that, out.exception_);
            }
            out.resultValue_ = StoreBackendObject(move(selector));
        };
        server.AddHandler("By.isBefore", genericRelativeBuilder);
        server.AddHandler("By.isAfter", genericRelativeBuilder);
    }

    static void RegisterUiDriverComponentFinders()
    {
        auto &server = FrontendApiServer::Get();
        auto genricFindWidgetHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            const auto driverRef = in.callerObjRef_;
            auto &driver = GetBackendObject<UiDriver>(driverRef);
            auto &selector = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
            UiOpArgs uiOpArgs;
            vector<unique_ptr<Widget>> recv;
            if (in.apiId_ == "UiDriver.waitForComponent") {
                uiOpArgs.waitWidgetMaxMs_ = ReadCallArg<uint32_t>(in, INDEX_ONE);
                auto result = driver.WaitForWidget(selector, uiOpArgs, out.exception_);
                if (result != nullptr) {
                    recv.emplace_back(move(result));
                }
            } else {
                driver.FindWidgets(selector, recv, out.exception_);
            }
            if (out.exception_.code_ != ErrCode::NO_ERROR) {
                return;
            }
            if (in.apiId_ == "UiDriver.assertComponentExist") {
                if (recv.empty()) { // widget-exist assertion faliure, delevier exception
                    out.exception_.code_ = ErrCode::ASSERTION_FAILURE;
                    out.exception_.message_ = "UiComponent not exist matching: " + selector.Describe();
                }
            } else if (in.apiId_ == "UiDriver.findComponents") { // return widget array, maybe empty
                for (auto &ptr : recv) {
                    out.resultValue_.emplace_back(StoreBackendObject(move(ptr), driverRef));
                }
            } else if (recv.empty()) { // return first one or null
                out.resultValue_ = nullptr;
            } else {
                out.resultValue_ = StoreBackendObject(move(*(recv.begin())), driverRef);
            }
        };
        server.AddHandler("UiDriver.findComponent", genricFindWidgetHandler);
        server.AddHandler("UiDriver.findComponents", genricFindWidgetHandler);
        server.AddHandler("UiDriver.waitForComponent", genricFindWidgetHandler);
        server.AddHandler("UiDriver.assertComponentExist", genricFindWidgetHandler);
    }

    static void RegisterUiDriverWindowFinder()
    {
        auto findWindowHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            const auto driverRef = in.callerObjRef_;
            auto &driver = GetBackendObject<UiDriver>(driverRef);
            auto filterJson = ReadCallArg<json>(in, INDEX_ZERO);
            if (filterJson.empty()) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "WindowFilter cannot be empty");
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
                return match;
            };
            auto window = driver.FindWindow(matcher, out.exception_);
            if (window == nullptr) {
                out.resultValue_ = nullptr;
            } else {
                out.resultValue_ = StoreBackendObject(move(window), driverRef);
            }
        };
        FrontendApiServer::Get().AddHandler("UiDriver.findWindow", findWindowHandler);
    }

    static void RegisterUiDriverMiscMethods()
    {
        auto &server = FrontendApiServer::Get();
        auto create = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            out.resultValue_ = StoreBackendObject(make_unique<UiDriver>());
        };
        server.AddHandler("UiDriver.create", create);
        auto delay = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            driver.DelayMs(ReadCallArg<uint32_t>(in, INDEX_ZERO));
        };
        server.AddHandler("UiDriver.delayMs", delay);
        auto screenCap = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            driver.TakeScreenCap(ReadCallArg<string>(in, INDEX_ZERO), out.exception_);
            out.resultValue_ = (out.exception_.code_ == ErrCode::NO_ERROR);
        };
        server.AddHandler("UiDriver.screenCap", screenCap);
        auto pressBack = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            driver.TriggerKey(Back(), uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.pressBack", pressBack);
        auto pressHome = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            driver.TriggerKey(Home(), uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.pressHome", pressHome);
        auto triggerKey = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto key = AnonymousSingleKey(ReadCallArg<int32_t>(in, INDEX_ZERO));
            driver.TriggerKey(key, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.triggerKey", triggerKey);
        auto triggerCombineKeys = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto key0 = ReadCallArg<int32_t>(in, INDEX_ZERO);
            auto key1 = ReadCallArg<int32_t>(in, INDEX_ONE);
            auto key2 = ReadCallArg<int32_t>(in, INDEX_TWO, KEYCODE_NONE);
            auto keyAction = CombinedKeys(key0, key1, key2);
            driver.TriggerKey(keyAction, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.triggerCombineKeys", triggerCombineKeys);
    }

    static void CheckSwipeVelocityPps(UiOpArgs& args)
    {
        if (args.swipeVelocityPps_ < args.minSwipeVelocityPps_ || args.swipeVelocityPps_ > args.maxSwipeVelocityPps_) {
            LOG_W("The swipe velocity out of range");
            args.swipeVelocityPps_ = args.defaultSwipeVelocityPps_;
        }
    }

    static void RegisterUiDriverTocuchOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericClick = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            const auto point0 = Point(ReadCallArg<int32_t>(in, INDEX_ZERO), ReadCallArg<int32_t>(in, INDEX_ONE));
            auto point1 = Point(0, 0);
            UiOpArgs uiOpArgs;
            auto op = TouchOp::CLICK;
            if (in.apiId_ == "UiDriver.longClick") {
                op = TouchOp::LONG_CLICK;
            } else if (in.apiId_ == "UiDriver.doubleClick") {
                op = TouchOp::DOUBLE_CLICK_P;
            } else if (in.apiId_ == "UiDriver.swipe") {
                op = TouchOp::SWIPE;
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_FOUR, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                point1 = Point(ReadCallArg<int32_t>(in, INDEX_TWO), ReadCallArg<int32_t>(in, INDEX_THREE));
            } else if (in.apiId_ == "UiDriver.drag") {
                op = TouchOp::DRAG;
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_FOUR, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                point1 = Point(ReadCallArg<int32_t>(in, INDEX_TWO), ReadCallArg<int32_t>(in, INDEX_THREE));
            }
            if (op == TouchOp::SWIPE || op == TouchOp::DRAG) {
                auto touch = GenericSwipe(op, point0, point1);
                driver.PerformTouch(touch, uiOpArgs, out.exception_);
            } else {
                auto touch = GenericClick(op, point0);
                driver.PerformTouch(touch, uiOpArgs, out.exception_);
            }
        };
        server.AddHandler("UiDriver.click", genericClick);
        server.AddHandler("UiDriver.longClick", genericClick);
        server.AddHandler("UiDriver.doubleClick", genericClick);
        server.AddHandler("UiDriver.swipe", genericClick);
        server.AddHandler("UiDriver.drag", genericClick);
    }

    static bool CheckMultiPointerOperatorsPoint(PointerMatrix& pointer)
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

    static void RegisterUiDriverMultiPointerOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericFling = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto op = TouchOp::SWIPE;
            auto pointJson0 = ReadCallArg<json>(in, INDEX_ZERO);
            auto pointJson1 = ReadCallArg<json>(in, INDEX_ONE);
            if (pointJson0.empty() || pointJson1.empty()) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "Point cannot be empty");
                return;
            }
            const auto point0 = Point(pointJson0["X"], pointJson0["Y"]);
            const auto point1 = Point(pointJson1["X"], pointJson1["Y"]);
            const auto stepLength = ReadCallArg<uint32_t>(in, INDEX_TWO);
            uiOpArgs.swipeVelocityPps_  = ReadCallArg<uint32_t>(in, INDEX_THREE);
            CheckSwipeVelocityPps(uiOpArgs);
            const int32_t distanceX = point1.px_ - point0.px_;
            const int32_t distanceY = point1.py_ - point0.py_;
            const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
            if (stepLength <= 0 || stepLength > distance) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "The stepLen is out of range");
                return;
            }
            uiOpArgs.swipeStepsCounts_ = distance / stepLength;
            auto touch = GenericSwipe(op, point0, point1);
            driver.PerformTouch(touch, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.fling", genericFling);

        auto multiPointerAction = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto &pointer = GetBackendObject<PointerMatrix>(ReadCallArg<string>(in, INDEX_ZERO));
            auto flag = CheckMultiPointerOperatorsPoint(pointer);
            if (!flag) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "There is not all coordinate points are set");
                return;
            }
            UiOpArgs uiOpArgs;
            uiOpArgs.swipeVelocityPps_  = ReadCallArg<uint32_t>(in, INDEX_ONE, uiOpArgs.swipeVelocityPps_);
            CheckSwipeVelocityPps(uiOpArgs);
            auto touch = MultiPointerAction(pointer);
            driver.PerformTouch(touch, uiOpArgs, out.exception_);
            out.resultValue_ = (out.exception_.code_ == ErrCode::NO_ERROR);
        };
        server.AddHandler("UiDriver.injectMultiPointerAction", multiPointerAction);
    }

    static void RegisterUiDriverDisplayOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericDisplayOperator = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            auto controller = driver.GetUiController(out.exception_);
            if (out.exception_.code_ != NO_ERROR) {
                return;
            }
            if (in.apiId_ == "UiDriver.setDisplayRotation") {
                auto rotation = ReadCallArg<DisplayRotation>(in, INDEX_ZERO);
                controller->SetDisplayRotation(rotation);
            } else if (in.apiId_ == "UiDriver.getDisplayRotation") {
                out.resultValue_ = controller->GetDisplayRotation();
            } else if (in.apiId_ == "UiDriver.setDisplayRotationEnabled") {
                auto enabled = ReadCallArg<bool>(in, INDEX_ZERO);
                controller->SetDisplayRotationEnabled(enabled);
            } else if (in.apiId_ == "UiDriver.waitForIdle") {
                auto idleTime = ReadCallArg<uint32_t>(in, INDEX_ZERO);
                auto timeout = ReadCallArg<uint32_t>(in, INDEX_ONE);
                out.resultValue_ = controller->WaitForUiSteady(idleTime, timeout);
            } else if (in.apiId_ == "UiDriver.wakeUpDisplay") {
                if (controller->IsScreenOn()) {
                    return;
                } else {
                    LOG_I("screen is off, turn it on");
                    UiOpArgs uiOpArgs;
                    driver.TriggerKey(Power(), uiOpArgs, out.exception_);
                }
            } else if (in.apiId_ == "UiDriver.getDisplaySize") {
                auto result = controller->GetDisplaySize();
                json data;
                data["X"] = result.px_;
                data["Y"] = result.py_;
                out.resultValue_ = data;
            } else if (in.apiId_ == "UiDriver.getDisplayDensity") {
                auto result = controller->GetDisplayDensity();
                json data;
                data["X"] = result.px_;
                data["Y"] = result.py_;
                out.resultValue_ = data;
            }
        };
        server.AddHandler("UiDriver.setDisplayRotation", genericDisplayOperator);
        server.AddHandler("UiDriver.getDisplayRotation", genericDisplayOperator);
        server.AddHandler("UiDriver.setDisplayRotationEnabled", genericDisplayOperator);
        server.AddHandler("UiDriver.waitForIdle", genericDisplayOperator);
        server.AddHandler("UiDriver.wakeUpDisplay", genericDisplayOperator);
        server.AddHandler("UiDriver.getDisplaySize", genericDisplayOperator);
        server.AddHandler("UiDriver.getDisplayDensity", genericDisplayOperator);
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
            data["X"] = bounds.GetCenterX();
            data["Y"] = bounds.GetCenterY();
            out.resultValue_ = data;
            return;
        }
        if (attrName == ATTR_NAMES[UiAttr::BOUNDS]) { // getBounds
            const auto bounds = snapshot->GetBounds();
            json data;
            data["leftX"] = bounds.left_;
            data["topY"] = bounds.top_;
            data["rightX"] = bounds.right_;
            data["bottomY"] = bounds.bottom_;
            out.resultValue_ = data;
            return;
        }
        // convert value-string to json value of target type
        auto attrValue = snapshot->GetAttr(attrName, "NA");
        if (attrValue == "NA") {
            out.resultValue_ = nullptr; // no such attribute, return null
        } else if (kString) {
            out.resultValue_ = attrValue;
        } else {
            out.resultValue_ = nlohmann::json::parse(attrValue);
        }
    }

    static void RegisterUiComponentAttrGetters()
    {
        auto &server = FrontendApiServer::Get();
        server.AddHandler("UiComponent.getId", GenericComponentAttrGetter<UiAttr::ID>);
        server.AddHandler("UiComponent.getText", GenericComponentAttrGetter<UiAttr::TEXT, true>);
        server.AddHandler("UiComponent.getKey", GenericComponentAttrGetter<UiAttr::KEY, true>);
        server.AddHandler("UiComponent.getType", GenericComponentAttrGetter<UiAttr::TYPE, true>);
        server.AddHandler("UiComponent.isEnabled", GenericComponentAttrGetter<UiAttr::ENABLED>);
        server.AddHandler("UiComponent.isFocused", GenericComponentAttrGetter<UiAttr::FOCUSED>);
        server.AddHandler("UiComponent.isSelected", GenericComponentAttrGetter<UiAttr::SELECTED>);
        server.AddHandler("UiComponent.isClickable", GenericComponentAttrGetter<UiAttr::CLICKABLE>);
        server.AddHandler("UiComponent.isLongClickable", GenericComponentAttrGetter<UiAttr::LONG_CLICKABLE>);
        server.AddHandler("UiComponent.isScrollable", GenericComponentAttrGetter<UiAttr::SCROLLABLE>);
        server.AddHandler("UiComponent.isCheckable", GenericComponentAttrGetter<UiAttr::CHECKABLE>);
        server.AddHandler("UiComponent.isChecked", GenericComponentAttrGetter<UiAttr::CHECKED>);
        server.AddHandler("UiComponent.getBounds", GenericComponentAttrGetter<UiAttr::BOUNDS>);
        server.AddHandler("UiComponent.getBoundsCenter", GenericComponentAttrGetter<UiAttr::BOUNDSCENTER>);
    }

    static void RegisterUiComponentOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto wOp = WidgetOperator(driver, widget, uiOpArgs);
            if (in.apiId_ == "UiComponent.click") {
                wOp.GenericClick(TouchOp::CLICK, out.exception_);
            } else if (in.apiId_ == "UiComponent.longClick") {
                wOp.GenericClick(TouchOp::LONG_CLICK, out.exception_);
            } else if (in.apiId_ == "UiComponent.doubleClick") {
                wOp.GenericClick(TouchOp::DOUBLE_CLICK_P, out.exception_);
            } else if (in.apiId_ == "UiComponent.scrollToTop") {
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_ZERO, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                wOp.ScrollToEnd(true, out.exception_);
            } else if (in.apiId_ == "UiComponent.scrollToBottom") {
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<uint32_t>(in, INDEX_ZERO, uiOpArgs.swipeVelocityPps_);
                CheckSwipeVelocityPps(uiOpArgs);
                wOp.ScrollToEnd(false, out.exception_);
            } else if (in.apiId_ == "UiComponent.dragTo") {
                auto &widgetTo = GetBackendObject<Widget>(ReadCallArg<string>(in, INDEX_ZERO));
                wOp.DragIntoWidget(widgetTo, out.exception_);
            } else if (in.apiId_ == "UiComponent.inputText") {
                wOp.InputText(ReadCallArg<string>(in, INDEX_ZERO), out.exception_);
            } else if (in.apiId_ == "UiComponent.clearText") {
                wOp.InputText("", out.exception_);
            } else if (in.apiId_ == "UiComponent.scrollSearch") {
                auto &selector = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
                auto res = wOp.ScrollFindWidget(selector, out.exception_);
                if (res != nullptr) {
                    out.resultValue_ = StoreBackendObject(move(res), sDriverBindingMap.find(in.callerObjRef_)->second);
                }
            } else if (in.apiId_ == "UiComponent.pinchOut" || in.apiId_ == "UiComponent.pinchIn") {
                auto pinchScale = ReadCallArg<float_t>(in, INDEX_ZERO);
                wOp.PinchWidget(pinchScale, out.exception_);
            }
        };
        server.AddHandler("UiComponent.click", genericOperationHandler);
        server.AddHandler("UiComponent.longClick", genericOperationHandler);
        server.AddHandler("UiComponent.doubleClick", genericOperationHandler);
        server.AddHandler("UiComponent.scrollToTop", genericOperationHandler);
        server.AddHandler("UiComponent.scrollToBottom", genericOperationHandler);
        server.AddHandler("UiComponent.dragTo", genericOperationHandler);
        server.AddHandler("UiComponent.inputText", genericOperationHandler);
        server.AddHandler("UiComponent.clearText", genericOperationHandler);
        server.AddHandler("UiComponent.scrollSearch", genericOperationHandler);
        server.AddHandler("UiComponent.pinchOut", genericOperationHandler);
        server.AddHandler("UiComponent.pinchIn", genericOperationHandler);
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
                data["leftX"] = snapshot->bounds_.left_;
                data["topY"] = snapshot->bounds_.top_;
                data["rightX"] = snapshot->bounds_.right_;
                data["bottomY"] = snapshot->bounds_.bottom_;
                out.resultValue_ = data;
            } else if (in.apiId_ == "UiWindow.getTitle") {
                out.resultValue_ = snapshot->title_;
            } else if (in.apiId_ == "UiWindow.getWindowMode") {
                out.resultValue_ = (uint8_t)(snapshot->mode_ - 1);
            } else if (in.apiId_ == "UiWindow.isFocused") {
                out.resultValue_ = snapshot->focused_;
            } else if (in.apiId_ == "UiWindow.isActived") {
                out.resultValue_ = snapshot->actived_;
            }
        };
        server.AddHandler("UiWindow.getBundleName", genericGetter);
        server.AddHandler("UiWindow.getBounds", genericGetter);
        server.AddHandler("UiWindow.getTitle", genericGetter);
        server.AddHandler("UiWindow.getWindowMode", genericGetter);
        server.AddHandler("UiWindow.isFocused", genericGetter);
        server.AddHandler("UiWindow.isActived", genericGetter);
    }

    static void RegisterUiWindowOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericWinOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &window = GetBackendObject<Window>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto wOp = WindowOperator(driver, window, uiOpArgs);
            auto action = in.apiId_;
            if (action == "UiWindow.resize") {
                auto width = ReadCallArg<uint32_t>(in, INDEX_ZERO);
                auto highth = ReadCallArg<uint32_t>(in, INDEX_ONE);
                auto direction = ReadCallArg<ResizeDirection>(in, INDEX_TWO);
                out.resultValue_ = wOp.Resize(width, highth, direction, out);
            } else if (action == "UiWindow.focus") {
                out.resultValue_ = wOp.Focuse(out);
            }
        };
        server.AddHandler("UiWindow.focus", genericWinOperationHandler);
        server.AddHandler("UiWindow.resize", genericWinOperationHandler);
    }

    static void RegisterUiWinBarOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericWinBarOperationHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &window = GetBackendObject<Window>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto wOp = WindowOperator(driver, window, uiOpArgs);
            auto action = in.apiId_;
            if (window.decoratorEnabled_) {
                if (action == "UiWindow.split") {
                    out.resultValue_ = wOp.Split(out);
                } else if (action == "UiWindow.maximize") {
                    out.resultValue_ = wOp.Maximize(out);
                } else if (action == "UiWindow.resume") {
                    out.resultValue_ = wOp.Resume(out);
                } else if (action == "UiWindow.minimize") {
                    out.resultValue_ = wOp.Minimize(out);
                } else if (action == "UiWindow.close") {
                    out.resultValue_ = wOp.Close(out);
                } else if (action == "UiWindow.moveTo") {
                    auto endX = ReadCallArg<uint32_t>(in, INDEX_ZERO);
                    auto endY = ReadCallArg<uint32_t>(in, INDEX_ONE);
                    out.resultValue_ = wOp.MoveTo(endX, endY, out);
                }
            } else {
                out.exception_ = ApiCallErr(USAGE_ERROR, "this device can not support this action");
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
                out.exception_ = ApiCallErr(USAGE_ERROR, "Number of illegal fingers");
                return;
            }
            auto step = ReadCallArg<uint32_t>(in, INDEX_ONE);
            if (step < 1 || step > uiOpArgs.maxMultiTouchSteps) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "Number of illegal steps");
                return;
            }
            out.resultValue_ = StoreBackendObject(make_unique<PointerMatrix>(finger, step));
        };
        server.AddHandler("PointerMatrix.create", create);

        auto setPoint = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &pointer = GetBackendObject<PointerMatrix>(in.callerObjRef_);
            auto finger = ReadCallArg<uint32_t>(in, INDEX_ZERO);
            if (finger < 0 || finger >= pointer.GetFingers()) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "Number of illegal fingers");
                return;
            }
            auto step = ReadCallArg<uint32_t>(in, INDEX_ONE);
            if (step < 0 || step >= pointer.GetSteps()) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "Number of illegal steps");
                return;
            }
            auto pointJson = ReadCallArg<json>(in, INDEX_TWO);
            if (pointJson.empty()) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "Point cannot be empty");
                return;
            }
            const auto point = Point(pointJson["X"], pointJson["Y"]);
            pointer.At(finger, step).point_ = point;
            pointer.At(finger, step).flags_ = 1;
        };
        server.AddHandler("PointerMatrix.setPoint", setPoint);
    }

    /** Register fronendApiHandlers and preprocessors on startup.*/
    __attribute__((constructor)) static void RegisterApiHandlers()
    {
        auto &server = FrontendApiServer::Get();
        server.AddCommonPreprocessor("APiCallInfoChecker", APiCallInfoChecker);
        server.AddHandler("BackendObjectsCleaner", BackendObjectsCleaner);
        RegisterByBuilders();
        RegisterUiDriverComponentFinders();
        RegisterUiDriverWindowFinder();
        RegisterUiDriverMiscMethods();
        RegisterUiDriverTocuchOperators();
        RegisterUiComponentAttrGetters();
        RegisterUiComponentOperators();
        RegisterUiWindowAttrGetters();
        RegisterUiWindowOperators();
        RegisterUiWinBarOperators();
        RegisterPointerMatrixOperators();
        RegisterUiDriverMultiPointerOperators();
        RegisterUiDriverDisplayOperators();
    }
} // namespace OHOS::uitest
