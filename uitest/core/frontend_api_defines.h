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

#include <initializer_list>
#include <string_view>
#include "json.hpp"

namespace OHOS::uitest {
    enum ErrCode : uint8_t {
        NO_ERROR = 0,
        /**Internal error, not expected to happen.*/
        INTERNAL_ERROR = 1,
        /**Widget that is expected to be exist lost.*/
        WIDGET_LOST = 2,
        /**Window that is expected to be exist lost.*/
        WINDOW_LOST = 3,
        /**The user assertion failure.*/
        ASSERTION_FAILURE = 4,
        USAGE_ERROR = 5,
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

    /**Structure wraps the api-call data.*/
    struct ApiCallInfo {
        std::string apiId_;
        std::string callerObjRef_;
        nlohmann::json paramList_ = nlohmann::json::array();
    };

    /**Structure wraps the api-call reply.*/
    struct ApiReplyInfo {
        nlohmann::json resultValue_ = nullptr;
        ApiCallErr exception_ = ApiCallErr(ErrCode::NO_ERROR);
    };

    /** Specifications of a frontend enumrator value.*/
    struct FrontendEnumValueDef {
        std::string_view name_;
        std::string_view valueJson_;
    };

    /** Specifications of a frontend enumrator.*/
    struct FrontendEnumratorDef {
        std::string_view name_;
        const FrontendEnumValueDef *values_;
        size_t valueCount_;
    };

    /** Specifications of a frontend json data property.*/
    struct FrontEndJsonPropDef {
        std::string_view name_;
        std::string_view type_;
        bool required_;
    };
    /** Specifications of a frontend json object.*/
    struct FrontEndJsonDef {
        std::string_view name_;
        const FrontEndJsonPropDef *props_;
        size_t propCount_;
    };

    /** Specifications of a frontend class method.*/
    struct FrontendMethodDef {
        std::string_view name_;
        std::string_view signature_;
        bool static_;
        bool fast_;
    };

    /** Specifications of a frontend class.*/
    struct FrontEndClassDef {
        std::string_view name_;
        const FrontendMethodDef *methods_;
        size_t methodCount_;
        bool bindUiDriver_;
    };

    /** MatchPattern enumrator definition.*/
    constexpr FrontendEnumValueDef PATTERN_VALUES[] = {
        {"EQUALS", "0"},
        {"CONTAINS", "1"},
        {"STARTS_WITH", "2"},
        {"ENDS_WITH", "3"},
    };
    constexpr FrontendEnumratorDef MATCH_PATTERN_DEF = {
        "MatchPattern",
        PATTERN_VALUES,
        sizeof(PATTERN_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Window ResizeDirection enumrator definition.*/
    constexpr FrontendEnumValueDef RESIZE_DIRECTION_VALUES[] = {
        {"LEFT", "0"},    {"RIGHT", "1"},     {"UP", "2"},       {"DOWN", "3"},
        {"LEFT_UP", "4"}, {"LEFT_DOWN", "5"}, {"RIGHT_UP", "6"}, {"RIGHT_DOWN", "7"},
    };
    constexpr FrontendEnumratorDef RESIZE_DIRECTION_DEF = {
        "ResizeDirection",
        RESIZE_DIRECTION_VALUES,
        sizeof(RESIZE_DIRECTION_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** WindowMode enumrator definition.*/
    constexpr FrontendEnumValueDef WINDOW_MODE_VALUES[] = {
        {"FULLSCREEN", "0"},
        {"PRIMARY", "1"},
        {"SECONDARY", "2"},
        {"FLOATING", "3"},
    };
    constexpr FrontendEnumratorDef WINDOW_MODE_DEF = {
        "WindowMode",
        WINDOW_MODE_VALUES,
        sizeof(WINDOW_MODE_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Describes the rotation of the device display.*/
    constexpr FrontendEnumValueDef DISPLAY_ROTATION_VALUES[] = {
        {"ROTATION_0", "0"},
        {"ROTATION_90", "1"},
        {"ROTATION_180", "2"},
        {"ROTATION_270", "3"},
    };
    constexpr FrontendEnumratorDef DISPLAY_ROTATION_DEF = {
        "DisplayRotation",
        DISPLAY_ROTATION_VALUES,
        sizeof(DISPLAY_ROTATION_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Rect jsonObject definition.*/
    constexpr FrontEndJsonPropDef RECT_PROPERTIES[] = {
        {"leftX", "int", true},
        {"topY", "int", true},
        {"rightX", "int", true},
        {"bottomY", "int", true},
    };
    constexpr FrontEndJsonDef RECT_DEF = {
        "Rect",
        RECT_PROPERTIES,
        sizeof(RECT_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** Point jsonObject definition.*/
    constexpr FrontEndJsonPropDef POINT_PROPERTIES[] = {
        {"X", "int", true},
        {"Y", "int", true},
    };
    constexpr FrontEndJsonDef POINT_DEF = {
        "Point",
        POINT_PROPERTIES,
        sizeof(POINT_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** WindowFilter jsonObject definition.*/
    constexpr FrontEndJsonPropDef WINDOW_FILETR_PROPERTIES[] = {
        {"bundleName", "string", false},
        {"title", "string", false},
        {"focused", "bool", false},
        {"actived", "bool", false},
    };
    constexpr FrontEndJsonDef WINDOW_FILTER_DEF = {
        "WindowFilter",
        WINDOW_FILETR_PROPERTIES,
        sizeof(WINDOW_FILETR_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** By class definition.*/
    constexpr FrontendMethodDef BY_METHODS[] = {
        {"By.id", "(int):By", false, true},
        {"By.text", "(string,int?):By", false, true}, //  MatchPattern enum as int value
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
        sizeof(BY_METHODS) / sizeof(FrontendMethodDef),
    };

    /** UiDriver class definition.*/
    constexpr FrontendMethodDef UI_DRIVER_METHODS[] = {
        {"UiDriver.create", "():UiDriver", true, true},
        {"UiDriver.delayMs", "(int):void", false, false},
        {"UiDriver.findComponent", "(By):UiComponent", false, false},
        {"UiDriver.findWindow", "(WindowFilter):UiWindow", false, false},
        {"UiDriver.findComponents", "(By):[UiComponent]", false, false},
        {"UiDriver.waitForComponent", "(By,int):UiComponent", false, false},
        {"UiDriver.screenCap", "(string):bool", false, false},
        {"UiDriver.assertComponentExist", "(By):void", false, false},
        {"UiDriver.pressBack", "():void", false, false},
        {"UiDriver.triggerKey", "(int):void", false, false},
        {"UiDriver.triggerCombineKeys", "(int,int,int?):void", false, false},
        {"UiDriver.click", "(int,int):void", false, false},
        {"UiDriver.longClick", "(int,int):void", false, false},
        {"UiDriver.doubleClick", "(int,int):void", false, false},
        {"UiDriver.swipe", "(int,int,int,int,int?):void", false, false},
        {"UiDriver.drag", "(int,int,int,int,int?):void", false, false},
        {"UiDriver.setDisplayRotation", "(int):void", false, false},  // DisplayRotation enum as int value
        {"UiDriver.getDisplayRotation", "():int", false, false},  // DisplayRotation enum as int value
        {"UiDriver.setDisplayRotationEnabled", "(bool):void", false, false},
        {"UiDriver.getDisplaySize", "():Point", false, false},
        {"UiDriver.getDisplayDensity", "():Point", false, false},
        {"UiDriver.wakeUpDisplay", "():void", false, false},
        {"UiDriver.pressHome", "():void", false, false},
        {"UiDriver.waitForIdle", "(int,int):bool", false, false},
    };
    constexpr FrontEndClassDef UI_DRIVER_DEF = {
        "UiDriver",
        UI_DRIVER_METHODS,
        sizeof(UI_DRIVER_METHODS) / sizeof(FrontendMethodDef),
    };

    /** UiComponent class definition.*/
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
        {"UiComponent.pinchOut", "(float):void", false, false},
        {"UiComponent.pinchIn", "(float):void", false, false},
    };
    constexpr FrontEndClassDef UI_COMPONENT_DEF = {
        "UiComponent",
        UI_COMPONENT_METHODS,
        sizeof(UI_COMPONENT_METHODS) / sizeof(FrontendMethodDef),
    };

    /** UiWindow class definition.*/
    constexpr FrontendMethodDef UI_WINDOW_METHODS[] = {
        {"UiWindow.getBundleName", "():string", false, false},
        {"UiWindow.getBounds", "():Rect", false, false},
        {"UiWindow.getTitle", "():string", false, false},
        {"UiWindow.getWindowMode", "():int", false, false}, // WindowMode enum as int value
        {"UiWindow.isFocused", "():bool", false, false},
        {"UiWindow.isActived", "():bool", false, false},
        {"UiWindow.focus", "():bool", false, false},
        {"UiWindow.moveTo", "(int,int):bool", false, false},
        {"UiWindow.resize", "(int,int,int):bool", false, false}, // ResizeDirection enum as int value
        {"UiWindow.split", "():bool", false, false},
        {"UiWindow.maximize", "():bool", false, false},
        {"UiWindow.resume", "():bool", false, false},
        {"UiWindow.minimize", "():bool", false, false},
        {"UiWindow.close", "():bool", false, false},
    };
    constexpr FrontEndClassDef UI_WINDOW_DEF = {
        "UiWindow",
        UI_WINDOW_METHODS,
        sizeof(UI_WINDOW_METHODS) / sizeof(FrontendMethodDef),
    };

    /** List all the frontend data-type definitions.*/
    const auto FRONTEND_CLASS_DEFS = {&BY_DEF, &UI_DRIVER_DEF, &UI_COMPONENT_DEF, &UI_WINDOW_DEF};
    const auto FRONTEND_ENUMERATOR_DEFS = {&MATCH_PATTERN_DEF, &WINDOW_MODE_DEF, &RESIZE_DIRECTION_DEF,
                                           &DISPLAY_ROTATION_DEF};
    const auto FRONTEND_JSON_DEFS = {&RECT_DEF, &POINT_DEF, &WINDOW_FILTER_DEF};
    /** The allowed in/out data type scope of frontend apis.*/
    const std::initializer_list<std::string_view> DATA_TYPE_SCOPE = {
        "int",
        "float",
        "bool",
        "string",
        RECT_DEF.name_,
        POINT_DEF.name_,
        WINDOW_FILTER_DEF.name_,
        BY_DEF.name_,
        UI_DRIVER_DEF.name_,
        UI_COMPONENT_DEF.name_,
        UI_WINDOW_DEF.name_,
    };
} // namespace OHOS::uitest

#endif