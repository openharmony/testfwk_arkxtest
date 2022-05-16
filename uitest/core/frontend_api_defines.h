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

#ifndef FRONTEND_API_DEFINES_H
#define FRONTEND_API_DEFINES_H

#include <string_view>
#include "json.hpp"

namespace OHOS::uitest {
    enum ErrCode : uint8_t {
        NO_ERROR = 0,
        /**Internal error, not expected to happen.*/
        INTERNAL_ERROR = 1,
        /**Widget that is expected to be exist lost.*/
        WIDGET_LOST = 2,
        /**The user assertion failure.*/
        ASSERTION_FAILURE = 3,
        USAGE_ERROR = 4,
    };

    /**API invocation error detail wrapper.*/
    struct ApiCallErr {
    public:
        ErrCode code_;
        std::string message_;

        ApiCallErr() = delete;

        explicit ApiCallErr(ErrCode ec) : ApiCallErr(ec, "") {}

        ApiCallErr(ErrCode ec, std::string_view msg)
        {
            code_ = ec;
            message_ = std::string(msg);
        }
    };

    /**Wraps the api-call data.*/
    struct ApiCallInfo {
        std::string apiId_;
        std::string callerObjRef_;
        nlohmann::json paramList_ = nlohmann::json::array();
    };

    /**Wraps the api-call reply.*/
    struct ApiReplyInfo {
        nlohmann::json resultValue_ = nullptr;
        ApiCallErr exception_ = ApiCallErr(ErrCode::NO_ERROR);
    };

    /** Specifications of a frontend enumrator value.*/
    struct FrontendEnumValueDef {
        std::string_view name_;
        std::string_view valueJson_;
    };
    constexpr size_t SIZE_EDEF = sizeof(FrontendEnumValueDef);
    /** Specifications of a frontend enumrator.*/
    struct FrontendEnumratorDef {
        std::string_view name_;
        const FrontendEnumValueDef *values_;
        size_t valueCount_;
    };

    /** List all the MatchPattern enumrator values..*/
    constexpr FrontendEnumValueDef PATTERN_VALUES[] = {
        {"EQUALS", "0"},
        {"CONTAINS", "1"},
        {"STARTS_WITH", "2"},
        {"ENDS_WITH", "3"},
    };
    constexpr FrontendEnumratorDef MATCH_PATTERN_DEF = {
        "MatchPattern",
        PATTERN_VALUES,
        sizeof(PATTERN_VALUES) / SIZE_EDEF,
    };

    /** Specifications of a frontend class method.*/
    struct FrontendMethodDef {
        std::string_view name_;
        std::string_view signature_;
        bool static_;
        bool fast_;
    };
    constexpr size_t SIZE_MDEF = sizeof(FrontendMethodDef);

    /** Specifications of a frontend class.*/
    struct FrontEndClassDef {
        std::string_view name_;
        const FrontendMethodDef *methods_;
        size_t methodCount_;
        bool bindUiDriver_;
    };

    /** List all the By methods.*/
    constexpr FrontendMethodDef BY_METHODS[] = {
        {"By.id", "(int):By", false, true},
        {"By.text", "(string,int?):By", false, true}, // default int arg: MatchPattern::EQ
        {"By.key", "(string):By", false, true},
        {"By.type", "(string):By", false, true},
        {"By.enabled", "(bool?):By", false, true}, // default bool arg: true
        {"By.focused", "(bool?):By", false, true},
        {"By.selected", "(bool?):By", false, true},
        {"By.clickable", "(bool?):By", false, true},
        {"By.longClickable", "(bool?):By", false, true},
        {"By.scrollable", "(bool?):By", false, true},
        {"By.checkable", "(bool?):By", false, true},
        {"By.checked", "(bool?):By", false, true},
        {"By.isBefore", "(By):By", false, true},
        {"By.isAfter", "(By):By", false, true},
    };
    constexpr std::string_view REF_SEED_BY = "By#seed";
    constexpr FrontEndClassDef BY_DEF = {
        "By",
        BY_METHODS,
        sizeof(BY_METHODS) / SIZE_MDEF,
    };

    /** List all the UiDriver methods.*/
    constexpr FrontendMethodDef UI_DRIVER_METHODS[] = {
        {"UiDriver.create", "():UiDriver", true, true},
        {"UiDriver.delayMs", "(int):void", false, false},
        {"UiDriver.findComponent", "(By):UiComponent", false, false},
        {"UiDriver.findComponents", "(By):[UiComponent]", false, false},
        {"UiDriver.waitForComponent", "(By,int):UiComponent", false, false},
        {"UiDriver.screenCap", "(string):bool", false, false},
        {"UiDriver.assertComponentExist", "(By):void", false, false},
        {"UiDriver.pressBack", "():void", false, false},
        {"UiDriver.triggerKey", "(int):void", false, false},
        {"UiDriver.click", "(int,int):void", false, false},
        {"UiDriver.longClick", "(int,int):void", false, false},
        {"UiDriver.doubleClick", "(int,int):void", false, false},
        {"UiDriver.swipe", "(int,int,int,int,int?):void", false, false},
        {"UiDriver.drag", "(int,int,int,int,int?):void", false, false},
    };
    constexpr FrontEndClassDef UI_DRIVER_DEF = {
        "UiDriver",
        UI_DRIVER_METHODS,
        sizeof(UI_DRIVER_METHODS) / SIZE_MDEF,
    };

    /** List all the UiComponent methods.*/
    constexpr FrontendMethodDef UI_COMPONENT_METHODS[] = {
        {"UiComponent.getId", "():int", false, false},
        {"UiComponent.getText", "():string", false, false},
        {"UiComponent.getKey", "():string", false, false},
        {"UiComponent.getType", "():string", false, false},
        {"UiComponent.isEnabled", "():bool", false, false},
        {"UiComponent.isFocused", "():bool", false, false},
        {"UiComponent.isSelected", "():bool", false, false},
        {"UiComponent.isClickable", "():bool", false, false},
        {"UiComponent.isLongClickable", "():bool", false, false},
        {"UiComponent.isScrollable", "():bool", false, false},
        {"UiComponent.isCheckable", "():bool", false, false},
        {"UiComponent.isChecked", "():bool", false, false},
        {"UiComponent.getBounds", "():Rect", false, false},
        {"UiComponent.getBoundsCenter", "():Point", false, false},
        {"UiComponent.click", "():void", false, false},
        {"UiComponent.longClick", "():void", false, false},
        {"UiComponent.doubleClick", "():void", false, false},
        {"UiComponent.scrollToTop", "(int?):void", false, false},
        {"UiComponent.scrollToBottom", "(int?):void", false, false},
        {"UiComponent.inputText", "(string):void", false, false},
        {"UiComponent.clearText", "():void", false, false},
        {"UiComponent.scrollSearch", "(By):UiComponent", false, false},
        {"UiComponent.dragTo", "(UiComponent):void", false, false},
    };
    constexpr FrontEndClassDef UI_COMPONENT_DEF = {
        "UiComponent",
        UI_COMPONENT_METHODS,
        sizeof(UI_COMPONENT_METHODS) / SIZE_MDEF,
    };

    /** List all the frontend class definitions.*/
    const auto FRONTEND_CLASS_DEFS = {&BY_DEF, &UI_DRIVER_DEF, &UI_COMPONENT_DEF};
} // namespace OHOS::uitest

#endif