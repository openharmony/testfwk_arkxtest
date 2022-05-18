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
#include "frontend_api_handler.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

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
        } catch (exception &ex) {
            out.exception_ = ApiCallErr(ErrCode::INTERNAL_ERROR, "Preprocessor failed: " + string(ex.what()));
        }
        try {
            find->second(in, out);
        } catch (exception &ex) {
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
    static map<string, pair<vector<nlohmann::detail::value_t>, bool>> sApiArgTypesMap;

    static void ParseMethodSignature(string_view signature, vector<nlohmann::detail::value_t> &types, bool &defArg)
    {
        static const map<string, nlohmann::detail::value_t> typeMap = {
            {"int", nlohmann::detail::value_t::number_unsigned}, {"float", nlohmann::detail::value_t::number_float},
            {"bool", nlohmann::detail::value_t::boolean},        {"string", nlohmann::detail::value_t::string},
            {"By", nlohmann::detail::value_t::string},           {"UiComponent", nlohmann::detail::value_t::string},
            {"UiDriver", nlohmann::detail::value_t::string},
        }; // complex-object should be passed with objRef (string type)
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
                    token = string(buf, tokenLen); // consume token and reset buffer
                    auto find = typeMap.find(token);
                    DCHECK(find != typeMap.end()); // illegal type
                    types.emplace_back(find->second);
                }
                tokenLen = 0;
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
                auto paramTypes = vector<nlohmann::detail::value_t>();
                auto hasDefaultArg = false;
                ParseMethodSignature(methodDef.signature_, paramTypes, hasDefaultArg);
                sApiArgTypesMap.insert(make_pair(string(methodDef.name_), make_pair(paramTypes, hasDefaultArg)));
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
        DCHECK(in.paramList_.type() == nlohmann::detail::value_t::array);
        for (const auto &item : in.paramList_) {
            DCHECK(item.type() == nlohmann::detail::value_t::string); // must be objRef
            const auto ref = item.get<string>();
            auto find = sBackendObjects.find(ref);
            if (find == sBackendObjects.end()) {
                LOG_W("No such object living: %{public}s", ref.c_str());
                continue;
            }
            sBackendObjects.erase(find);
            ss << ref << ",";
        }
        ss << "]";
        LOG_D("%{public}s", ss.str().c_str());
    }

    /** Checks ApiCallInfo data, and reports exception if any.*/
    static void CallDataChecker(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        // return nullptr by degfault
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
        const auto inArgSize = in.paramList_.size();
        if (inArgSize > maxArgc || inArgSize < minArgc) {
            out.exception_ = ApiCallErr(USAGE_ERROR, "Illegal argument count");
            return;
        }
        // check argument type
        for (size_t idx = 0; idx < inArgSize; idx++) {
            if (in.paramList_.at(idx).type() != types.at(idx)) {
                out.exception_ = ApiCallErr(USAGE_ERROR, "Illegal argument type");
                return;
            }
        }
    }

    template <typename T> static T ReadCallArg(const ApiCallInfo &in, size_t index)
    {
        DCHECK(in.paramList_.type() == nlohmann::detail::value_t::array);
        DCHECK(index <= in.paramList_.size());
        return in.paramList_.at(index).get<T>();
    }

    template <typename T> static T ReadCallArg(const ApiCallInfo &in, size_t index, T defValue)
    {
        DCHECK(in.paramList_.type() == nlohmann::detail::value_t::array);
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
                out.resultValue_ =  StoreBackendObject(move(*(recv.begin())), driverRef);
            }
        };
        server.AddHandler("UiDriver.findComponent", genricFindWidgetHandler);
        server.AddHandler("UiDriver.findComponents", genricFindWidgetHandler);
        server.AddHandler("UiDriver.waitForComponent", genricFindWidgetHandler);
        server.AddHandler("UiDriver.assertComponentExist", genricFindWidgetHandler);
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
            out.resultValue_ = (out.exception_.code_ == ErrCode::NO_ERROR) ;
        };
        server.AddHandler("UiDriver.screenCap", screenCap);

        auto pressBack = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            driver.TriggerKey(Back(), uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.pressBack", pressBack);

        auto triggerKey = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            auto key = AnonymousSingleKey(ReadCallArg<int32_t>(in, INDEX_ZERO));
            driver.TriggerKey(key, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiDriver.triggerKey", triggerKey);
    }

    static void RegisterUiDriverTocuchOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericClick = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &driver = GetBackendObject<UiDriver>(in.callerObjRef_);
            const auto point0 = Point(ReadCallArg<int32_t>(in, INDEX_ZERO), ReadCallArg<int32_t>(in, INDEX_ONE));
            auto point1 = Point(0, 0);
            UiOpArgs uiOpArgs;
            auto op = PointerOp::CLICK_P;
            if (in.apiId_ == "UiDriver.longClick") {
                op = PointerOp::LONG_CLICK_P;
            } else if (in.apiId_ == "UiDriver.doubleClick") {
                op = PointerOp::DOUBLE_CLICK_P;
            } else if (in.apiId_ == "UiDriver.swipe") {
                op = PointerOp::SWIPE_P;
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<int32_t>(in, INDEX_FOUR, uiOpArgs.swipeVelocityPps_);
                point1 = Point(ReadCallArg<int32_t>(in, INDEX_TWO), ReadCallArg<int32_t>(in, INDEX_THREE));
            } else if (in.apiId_ == "UiDriver.drag") {
                op = PointerOp::DRAG_P;
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<int32_t>(in, INDEX_FOUR, uiOpArgs.swipeVelocityPps_);
                point1 = Point(ReadCallArg<int32_t>(in, INDEX_TWO), ReadCallArg<int32_t>(in, INDEX_THREE));
            }
            if (op == PointerOp::SWIPE_P || op == PointerOp::DRAG_P) {
                driver.PerformSwipe(op, point0, point1, uiOpArgs, out.exception_);
            } else {
                driver.PerformClick(op, point0, uiOpArgs, out.exception_);
            }
        };
        server.AddHandler("UiDriver.click", genericClick);
        server.AddHandler("UiDriver.longClick", genericClick);
        server.AddHandler("UiDriver.doubleClick", genericClick);
        server.AddHandler("UiDriver.swipe", genericClick);
        server.AddHandler("UiDriver.drag", genericClick);
    }

    template <UiAttr kAttr, bool kString = false>
    static void GenericComponentAttrGetter(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        constexpr auto attrName = ATTR_NAMES[kAttr];
        auto &image = GetBackendObject<Widget>(in.callerObjRef_);
        auto &driver = GetBoundUiDriver(in.callerObjRef_);
        auto snapshot = driver.GetWidgetSnapshot(image, out.exception_);
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

    static void RegisterUiComponentGestures()
    {
        auto &server = FrontendApiServer::Get();
        auto genericGestureHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            WidgetOp op = WidgetOp::CLICK;
            UiOpArgs uiOpArgs;
            // these methods accept one option integer argument
            if (in.apiId_ == "UiComponent.click") {
                op = WidgetOp::CLICK;
            } else if (in.apiId_ == "UiComponent.longClick") {
                op = WidgetOp::LONG_CLICK;
            } else if (in.apiId_ == "UiComponent.doubleClick") {
                op = WidgetOp::DOUBLE_CLICK;
            } else if (in.apiId_ == "UiComponent.scrollToTop") {
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<int32_t>(in, INDEX_ZERO, uiOpArgs.swipeVelocityPps_);
                op = WidgetOp::SCROLL_TO_TOP;
            } else if (in.apiId_ == "UiComponent.scrollToBottom") {
                uiOpArgs.swipeVelocityPps_ = ReadCallArg<int32_t>(in, INDEX_ZERO, uiOpArgs.swipeVelocityPps_);
                op = WidgetOp::SCROLL_TO_BOTTOM;
            }
            driver.OperateWidget(widget, op, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiComponent.click", genericGestureHandler);
        server.AddHandler("UiComponent.longClick", genericGestureHandler);
        server.AddHandler("UiComponent.doubleClick", genericGestureHandler);
        server.AddHandler("UiComponent.scrollToTop", genericGestureHandler);
        server.AddHandler("UiComponent.scrollToBottom", genericGestureHandler);
    }

    static void RegisterUiComponentMiscOperators()
    {
        auto &server = FrontendApiServer::Get();
        auto genericInputHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            UiOpArgs uiOpArgs;
            string text = "";
            if (in.apiId_ == "UiComponent.inputText") {
                text = ReadCallArg<string>(in, INDEX_ZERO);
            }
            driver.InputText(widget, text, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiComponent.inputText", genericInputHandler);
        server.AddHandler("UiComponent.clearText", genericInputHandler);

        auto scrollSearchHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            auto &selector = GetBackendObject<WidgetSelector>(ReadCallArg<string>(in, INDEX_ZERO));
            UiOpArgs uiOpArgs;
            driver.ScrollSearch(widget, selector, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiComponent.scrollSearch", scrollSearchHandler);

        auto dragToHandler = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto &widget = GetBackendObject<Widget>(in.callerObjRef_);
            auto &driver = GetBoundUiDriver(in.callerObjRef_);
            auto &widgetTo = GetBackendObject<Widget>(ReadCallArg<string>(in, INDEX_ZERO));
            UiOpArgs uiOpArgs;
            driver.DragIntoWidget(widget, widgetTo, uiOpArgs, out.exception_);
        };
        server.AddHandler("UiComponent.dragTo", dragToHandler);
    }

    /** Register fronendApiHandlers and preprocessors on startup.*/
    __attribute__((constructor)) static void RegisterApiHandlers()
    {
        auto &server = FrontendApiServer::Get();
        server.AddCommonPreprocessor("CallDataChecker", CallDataChecker);
        server.AddHandler("BackendObjectsCleaner", BackendObjectsCleaner);
        RegisterByBuilders();
        RegisterUiDriverComponentFinders();
        RegisterUiDriverMiscMethods();
        RegisterUiDriverTocuchOperators();
        RegisterUiComponentAttrGetters();
        RegisterUiComponentGestures();
        RegisterUiComponentMiscOperators();
    }
} // namespace OHOS::uitest
