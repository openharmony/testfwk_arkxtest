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
#include <map>
#include "nlohmann/json.hpp"

namespace OHOS::uitest {
    enum ErrCode : uint32_t {
        /**Old ErrorCode*/
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
        /**New ErrorCode*/
        /**Initialize failed.*/
        ERR_INITIALIZE_FAILED = 17000001,
        /**API does not allow calling concurrently.*/
        ERR_API_USAGE = 17000002,
        /**Component existence assertion failed.*/
        ERR_ASSERTION_FAILED = 17000003,
        /**Component lost/UiWindow lost.*/
        ERR_COMPONENT_LOST = 17000004,
        /**This operation is not supported.*/
        ERR_OPERATION_UNSUPPORTED = 17000005,
        /**Internal error.*/
        ERR_INTERNAL = 17000006,
        /**Invalid input parameter.*/
        ERR_INVALID_INPUT = 401,
        /**The specified SystemCapability name was not found.*/
        ERR_NO_SYSTEM_CAPABILITY = 801,
        /**Invalid input parameter.*/
        ERR_INVALID_PARAM = 17000007,
    };

    const std::map<ErrCode, std::string> ErrDescMap = {
        /**Correspondence between error codes and descriptions*/
        {NO_ERROR, "No Error"},
        {ERR_INITIALIZE_FAILED, "Initialization failed."},
        {ERR_API_USAGE, "The async function is not called with await."},
        {ERR_ASSERTION_FAILED, "Assertion failed."},
        {ERR_COMPONENT_LOST, "The window or component is invisible or destroyed."},
        {ERR_OPERATION_UNSUPPORTED, "This operation is not supported."},
        {ERR_INTERNAL, "Internal error."},
        {ERR_NO_SYSTEM_CAPABILITY, "The specified SystemCapability name was not found."},
        {ERR_INVALID_INPUT, "Invalid input parameter."},
    };

    /**API invocation error detail wrapper.*/
    struct ApiCallErr {
    public:
        ErrCode code_;
        std::string message_ = "";

        ApiCallErr() = delete;

        explicit ApiCallErr(ErrCode ec)
        {
            code_ = ec;
            message_ = ErrDescMap.find(ec)->second;
        }

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
        int32_t fdParamIndex_ = -1; // support fd as param
        bool convertError_ = false;
    };

    /**Structure wraps the api-call reply.*/
    struct ApiReplyInfo {
        nlohmann::json resultValue_ = nullptr;
        ApiCallErr exception_ = ApiCallErr(NO_ERROR);
        bool convertError_ = false;
    };

    /** Specifications of a frontend enumerator value.*/
    struct FrontendEnumValueDef {
        std::string_view name_;
        std::string_view valueJson_;
    };

    /** Specifications of a frontend enumerator.*/
    struct FrontendEnumeratorDef {
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
        bool convertError_ = false;
    };

    /** Specifications of a frontend class.*/
    struct FrontEndClassDef {
        std::string_view name_;
        const FrontendMethodDef *methods_;
        size_t methodCount_;
        bool bindUiDriver_;
    };

    /** MatchPattern enumerator definition.*/
    constexpr FrontendEnumValueDef PATTERN_VALUES[] = {
        {"EQUALS", "0"},
        {"CONTAINS", "1"},
        {"STARTS_WITH", "2"},
        {"ENDS_WITH", "3"},
        {"REG_EXP", "4"},
        {"REG_EXP_ICASE", "5"},
    };
    constexpr FrontendEnumeratorDef MATCH_PATTERN_DEF = {
        "MatchPattern",
        PATTERN_VALUES,
        sizeof(PATTERN_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Window ResizeDirection enumerator definition.*/
    constexpr FrontendEnumValueDef RESIZE_DIRECTION_VALUES[] = {
        {"LEFT", "0"},    {"RIGHT", "1"},     {"UP", "2"},       {"DOWN", "3"},
        {"LEFT_UP", "4"}, {"LEFT_DOWN", "5"}, {"RIGHT_UP", "6"}, {"RIGHT_DOWN", "7"},
    };
    constexpr FrontendEnumeratorDef RESIZE_DIRECTION_DEF = {
        "ResizeDirection",
        RESIZE_DIRECTION_VALUES,
        sizeof(RESIZE_DIRECTION_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** WindowMode enumerator definition.*/
    constexpr FrontendEnumValueDef WINDOW_MODE_VALUES[] = {
        {"FULLSCREEN", "0"},
        {"PRIMARY", "1"},
        {"SECONDARY", "2"},
        {"FLOATING", "3"},
    };
    constexpr FrontendEnumeratorDef WINDOW_MODE_DEF = {
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
    constexpr FrontendEnumeratorDef DISPLAY_ROTATION_DEF = {
        "DisplayRotation",
        DISPLAY_ROTATION_VALUES,
        sizeof(DISPLAY_ROTATION_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Describes the Button of the mouse.*/
    constexpr FrontendEnumValueDef MOUSE_BUTTON_VALUES[] = {
        {"MOUSE_BUTTON_LEFT", "0"},
        {"MOUSE_BUTTON_RIGHT", "1"},
        {"MOUSE_BUTTON_MIDDLE", "2"},
    };
    constexpr FrontendEnumeratorDef MOUSE_BUTTON_DEF = {
        "MouseButton",
        MOUSE_BUTTON_VALUES,
        sizeof(MOUSE_BUTTON_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Describes the direction of the UI operation.*/
    constexpr FrontendEnumValueDef UI_DIRECTION_VALUES[] = {
        {"LEFT", "0"},
        {"RIGHT", "1"},
        {"UP", "2"},
        {"DOWN", "3"},
    };
    constexpr FrontendEnumeratorDef UI_DIRECTION_DEF = {
        "UiDirection",
        UI_DIRECTION_VALUES,
        sizeof(UI_DIRECTION_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** Rect jsonObject definition.*/
    constexpr FrontEndJsonPropDef RECT_PROPERTIES[] = {
        {"left", "int", true},
        {"top", "int", true},
        {"right", "int", true},
        {"bottom", "int", true},
        {"displayId", "int", false},
    };
    constexpr FrontEndJsonDef RECT_DEF = {
        "Rect",
        RECT_PROPERTIES,
        sizeof(RECT_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** Point jsonObject definition.*/
    constexpr FrontEndJsonPropDef POINT_PROPERTIES[] = {
        {"x", "int", true},
        {"y", "int", true},
        {"displayId", "int", false},
    };
    constexpr FrontEndJsonDef POINT_DEF = {
        "Point",
        POINT_PROPERTIES,
        sizeof(POINT_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** WindowFilter jsonObject definition.*/
    constexpr FrontEndJsonPropDef WINDOW_FILTER_PROPERTIES[] = {
        {"bundleName", "string", false},
        {"title", "string", false},
        {"focused", "bool", false},
        {"actived", "bool", false}, // Deprecated from API 11
        {"active", "bool", false},
        {"displayId", "int", false},
    };
    constexpr FrontEndJsonDef WINDOW_FILTER_DEF = {
        "WindowFilter",
        WINDOW_FILTER_PROPERTIES,
        sizeof(WINDOW_FILTER_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** InputTextMode jsonObject definition.*/
    constexpr FrontEndJsonPropDef INPUTTEXT_MODE_PROPERTIES[] = {
        {"paste", "bool", false},
        {"addition", "bool", false},
    };
    constexpr FrontEndJsonDef INPUTTEXT_MODE_DEF = {
        "InputTextMode",
        INPUTTEXT_MODE_PROPERTIES,
        sizeof(INPUTTEXT_MODE_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** UIElementInfo jsonObject definition.*/
    constexpr FrontEndJsonPropDef UI_ELEMENT_INFO_PROPERTIES[] = {
        {"bundleName", "string", false},
        {"text", "string", false},
        {"type", "string", false},
    };
    constexpr FrontEndJsonDef UI_ELEMENT_INFO_DEF = {
        "UIElementInfo",
        UI_ELEMENT_INFO_PROPERTIES,
        sizeof(UI_ELEMENT_INFO_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** TouchPadSwipeOptions definition.*/
    constexpr FrontEndJsonPropDef TOUCH_PAD_SWIPE_OPTIONS[] = {
        {"stay", "bool", false},
        {"speed", "int", false},
    };
    constexpr FrontEndJsonDef TOUCH_PAD_SWIPE_OPTIONS_DEF = {
        "TouchPadSwipeOptions",
        TOUCH_PAD_SWIPE_OPTIONS,
        sizeof(TOUCH_PAD_SWIPE_OPTIONS) / sizeof(FrontEndJsonPropDef),
    };

    /** By class definition. deprecated since api 9*/
    constexpr FrontendMethodDef BY_METHODS[] = {
        {"By.id", "(int):By", false, true},
        {"By.text", "(string,int?):By", false, true}, //  MatchPattern enum as int value
        {"By.key", "(string):By", false, true},
        {"By.type", "(string):By", false, true},
        {"By.enabled", "(bool?):By", false, true}, // default bool arg: true
        {"By.focused", "(bool?):By", false, true},
        {"By.selected", "(bool?):By", false, true},
        {"By.clickable", "(bool?):By", false, true},
        {"By.scrollable", "(bool?):By", false, true},
        {"By.isBefore", "(By):By", false, true},
        {"By.isAfter", "(By):By", false, true},
    };
    constexpr std::string_view REF_SEED_BY = "On#seed";
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
        {"UiDriver.findComponents", "(By):[UiComponent]", false, false},
        {"UiDriver.screenCap", "(int):bool", false, false}, // fliePath as fileDescription.
        {"UiDriver.assertComponentExist", "(By):void", false, false},
        {"UiDriver.pressBack", "():void", false, false},
        {"UiDriver.triggerKey", "(int):void", false, false},
        {"UiDriver.swipe", "(int,int,int,int,int?):void", false, false},
        {"UiDriver.click", "(int,int):void", false, false},
        {"UiDriver.longClick", "(int,int):void", false, false},
        {"UiDriver.doubleClick", "(int,int):void", false, false},
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
        {"UiComponent.isScrollable", "():bool", false, false},
        {"UiComponent.click", "():void", false, false},
        {"UiComponent.longClick", "():void", false, false},
        {"UiComponent.doubleClick", "():void", false, false},
        {"UiComponent.inputText", "(string):void", false, false},
        {"UiComponent.scrollSearch", "(By):UiComponent", false, false},
    };
    constexpr FrontEndClassDef UI_COMPONENT_DEF = {
        "UiComponent",
        UI_COMPONENT_METHODS,
        sizeof(UI_COMPONENT_METHODS) / sizeof(FrontendMethodDef),
    };

    /** On class definition(since api 9, outdates By class).*/
    constexpr FrontendMethodDef ON_METHODS[] = {
        {"On.text", "(string,int?):On", false, true}, //  MatchPattern enum as int value
        {"On.id", "(string,int?):On", false, true},
        {"On.type", "(string,int?):On", false, true},
        {"On.description", "(string,int?):On", false, true},
        {"On.hint", "(string,int?):On", false, true},
        {"On.enabled", "(bool?):On", false, true}, // default bool arg: true
        {"On.focused", "(bool?):On", false, true},
        {"On.selected", "(bool?):On", false, true},
        {"On.clickable", "(bool?):On", false, true},
        {"On.longClickable", "(bool?):On", false, true},
        {"On.scrollable", "(bool?):On", false, true},
        {"On.checkable", "(bool?):On", false, true},
        {"On.checked", "(bool?):On", false, true},
        {"On.isBefore", "(On):On", false, true},
        {"On.isAfter", "(On):On", false, true},
        {"On.within", "(On):On", false, true},
        {"On.inWindow", "(string):On", false, true},
        {"On.inDisplay", "(int):On", false, true},
        {"On.belongingDisplay", "(int):On", false, true, true},
        {"On.originalText", "(string,int?):On", false, true, true}, //  MatchPattern enum as int value
    };

    constexpr std::string_view REF_SEED_ON = "On#seed";
    constexpr FrontEndClassDef ON_DEF = {
        "On",
        ON_METHODS,
        sizeof(ON_METHODS) / sizeof(FrontendMethodDef),
    };
    
    /** Driver class definition. (since api 9, outdates UiDriver)*/
    constexpr FrontendMethodDef DRIVER_METHODS[] = {
        {"Driver.create", "():Driver", true, true},
        {"Driver.delayMs", "(int):void", false, false},
        {"Driver.findComponent", "(On):Component", false, false},
        {"Driver.findWindow", "(WindowFilter):UiWindow", false, false},
        {"Driver.findComponents", "(On):[Component]", false, false},
        {"Driver.waitForComponent", "(On,int):Component", false, false},
        {"Driver.screenCap", "(int,int?):bool", false, false},            // fliePath as fileDescription.
        {"Driver.screenCapture", "(int, Rect?):bool", false, false}, // fliePath as fileDescription.
        {"Driver.assertComponentExist", "(On):void", false, false},
        {"Driver.pressBack", "(int?):void", false, false, true},
        {"Driver.triggerKey", "(int,int?):void", false, false},
        {"Driver.triggerCombineKeys", "(int,int,int?,int?):void", false, false},
        {"Driver.click", "(int,int):void", false, false},
        {"Driver.click", "(Point):void", false, false},
        {"Driver.clickAt", "(Point):void", false, false, true},
        {"Driver.longClick", "(int,int):void", false, false},
        {"Driver.longClick", "(Point):void", false, false},
        {"Driver.longClickAt", "(Point, int?):void", false, false, true},
        {"Driver.doubleClick", "(int,int):void", false, false},
        {"Driver.doubleClick", "(Point):void", false, false},
        {"Driver.doubleClickAt", "(Point):void", false, false, true},
        {"Driver.swipe", "(int,int,int,int,int?):void", false, false},
        {"Driver.swipe", "(Point,Point,int?):void", false, false},
        {"Driver.swipeBetween", "(Point,Point,int?):void", false, false, true},
        {"Driver.drag", "(int,int,int,int,int?):void", false, false},
        {"Driver.drag", "(Point,Point,int?):void", false, false},
        {"Driver.dragBetween", "(Point,Point,int?,int?):void", false, false, true},
        {"Driver.setDisplayRotation", "(int):void", false, false},  // DisplayRotation enum as int value
        {"Driver.getDisplayRotation", "(int?):int", false, false, true},     // DisplayRotation enum as int value
        {"Driver.setDisplayRotationEnabled", "(bool):void", false, false},
        {"Driver.getDisplaySize", "(int?):Point", false, false, true},
        {"Driver.getDisplayDensity", "(int?):Point", false, false, true},
        {"Driver.wakeUpDisplay", "():void", false, false},
        {"Driver.pressHome", "(int?):void", false, false, true},
        {"Driver.waitForIdle", "(int,int):bool", false, false},
        {"Driver.fling", "(Point,Point,int,int):void", false, false},
        {"Driver.fling", "(int,int,int?):void", false, false},
        {"Driver.injectMultiPointerAction", "(PointerMatrix, int?):bool", false, false},
        {"Driver.mouseClick", "(Point,int,int?,int?):void", false, false},
        {"Driver.mouseDoubleClick", "(Point,int,int?,int?):void", false, false},
        {"Driver.mouseLongClick", "(Point,int,int?,int?,int?):void", false, false},
        {"Driver.mouseMoveTo", "(Point):void", false, false},
        {"Driver.mouseMoveWithTrack", "(Point,Point,int?):void", false, false},
        {"Driver.mouseDrag", "(Point,Point,int?,int?):void", false, false},
        {"Driver.mouseScroll", "(Point,bool,int,int?,int?,int?):void", false, false},
        {"Driver.createUIEventObserver", "():UIEventObserver", false, true},
        {"Driver.inputText", "(Point,string,InputTextMode?):void", false, false},
        {"Driver.touchPadMultiFingerSwipe", "(int,int,TouchPadSwipeOptions?):void", false, false},
        {"Driver.penClick", "(Point):void", false, false},
        {"Driver.penLongClick", "(Point,float?):void", false, false},
        {"Driver.penDoubleClick", "(Point):void", false, false},
        {"Driver.penSwipe", "(Point,Point,int?,float?):void", false, false},
        {"Driver.injectPenPointerAction", "(PointerMatrix,int?,float?):void", false, false},
        {"Driver.crownRotate", "(signedInt,int?):void", false, false, true},
    };
    constexpr FrontEndClassDef DRIVER_DEF = {
        "Driver",
        DRIVER_METHODS,
        sizeof(DRIVER_METHODS) / sizeof(FrontendMethodDef),
    };

    /** Component class definition.(since api 9, outdates UiComponent)*/
    constexpr FrontendMethodDef COMPONENT_METHODS[] = {
        {"Component.getText", "():string", false, false},
        {"Component.getId", "():string", false, false},
        {"Component.getType", "():string", false, false},
        {"Component.getDescription", "():string", false, false},
        {"Component.getHint", "():string", false, false},
        {"Component.getDisplayId", "():int", false, false},
        {"Component.isEnabled", "():bool", false, false},
        {"Component.isFocused", "():bool", false, false},
        {"Component.isSelected", "():bool", false, false},
        {"Component.isClickable", "():bool", false, false},
        {"Component.isLongClickable", "():bool", false, false},
        {"Component.isScrollable", "():bool", false, false},
        {"Component.isCheckable", "():bool", false, false},
        {"Component.isChecked", "():bool", false, false},
        {"Component.getBounds", "():Rect", false, false},
        {"Component.getBoundsCenter", "():Point", false, false},
        {"Component.click", "():void", false, false},
        {"Component.longClick", "():void", false, false},
        {"Component.doubleClick", "():void", false, false},
        {"Component.scrollToTop", "(int?):void", false, false},
        {"Component.scrollToBottom", "(int?):void", false, false},
        {"Component.inputText", "(string,InputTextMode?):void", false, false},
        {"Component.clearText", "():void", false, false},
        {"Component.scrollSearch", "(On, bool?, int?):Component", false, false},
        {"Component.dragTo", "(Component):void", false, false},
        {"Component.pinchOut", "(float):void", false, false},
        {"Component.pinchIn", "(float):void", false, false},
        {"Component.getOriginalText", "():string", false, false, true},
    };
    constexpr FrontEndClassDef COMPONENT_DEF = {
        "Component",
        COMPONENT_METHODS,
        sizeof(COMPONENT_METHODS) / sizeof(FrontendMethodDef),
    };

    /** UiWindow class definition.*/
    constexpr FrontendMethodDef UI_WINDOW_METHODS[] = {
        {"UiWindow.getBundleName", "():string", false, false},
        {"UiWindow.getBounds", "():Rect", false, false},
        {"UiWindow.getTitle", "():string", false, false},
        {"UiWindow.getWindowMode", "():int", false, false}, // WindowMode enum as int value
        {"UiWindow.getDisplayId", "():int", false, false},
        {"UiWindow.isFocused", "():bool", false, false},
        {"UiWindow.isActived", "():bool", false, false}, // Deprecated from API 11
        {"UiWindow.isActive", "():bool", false, false},
        {"UiWindow.focus", "():void", false, false},
        {"UiWindow.moveTo", "(int,int):void", false, false},
        {"UiWindow.resize", "(int,int,int):void", false, false}, // ResizeDirection enum as int value
        {"UiWindow.split", "():void", false, false},
        {"UiWindow.maximize", "():void", false, false},
        {"UiWindow.resume", "():void", false, false},
        {"UiWindow.minimize", "():void", false, false},
        {"UiWindow.close", "():void", false, false},
    };
    constexpr FrontEndClassDef UI_WINDOW_DEF = {
        "UiWindow",
        UI_WINDOW_METHODS,
        sizeof(UI_WINDOW_METHODS) / sizeof(FrontendMethodDef),
    };

    /** PointerMatrix class definition.*/
    constexpr FrontendMethodDef POINTER_MATRIX_METHODS[] = {
        {"PointerMatrix.create", "(int,int):PointerMatrix", true, true},
        {"PointerMatrix.setPoint", "(int,int,Point):void", false, true},
    };
    constexpr FrontEndClassDef POINTER_MATRIX_DEF = {
        "PointerMatrix",
        POINTER_MATRIX_METHODS,
        sizeof(POINTER_MATRIX_METHODS) / sizeof(FrontendMethodDef),
    };

    /** UIEventObserver class definition.*/
    constexpr FrontendMethodDef UI_EVENT_OBSERVER_METHODS[] = {
	    // callback<UIElementInfo> saved in js, works as callbackRef in c++.
        {"UIEventObserver.once", "(string, string):void", false, true},
    };
    constexpr FrontEndClassDef UI_EVENT_OBSERVER_DEF = {
        "UIEventObserver",
        UI_EVENT_OBSERVER_METHODS,
        sizeof(UI_EVENT_OBSERVER_METHODS) / sizeof(FrontendMethodDef),
    };

    /** List all the frontend data-type definitions.*/
    const auto FRONTEND_CLASS_DEFS = {&BY_DEF, &UI_DRIVER_DEF, &UI_COMPONENT_DEF, &ON_DEF,
                                      &DRIVER_DEF, &COMPONENT_DEF, &UI_WINDOW_DEF, &POINTER_MATRIX_DEF,
                                      &UI_EVENT_OBSERVER_DEF};
    const auto FRONTEND_ENUMERATOR_DEFS = {&MATCH_PATTERN_DEF, &WINDOW_MODE_DEF, &RESIZE_DIRECTION_DEF,
                                           &DISPLAY_ROTATION_DEF, &MOUSE_BUTTON_DEF, &UI_DIRECTION_DEF};
    const auto FRONTEND_JSON_DEFS = {&RECT_DEF, &POINT_DEF, &WINDOW_FILTER_DEF, &UI_ELEMENT_INFO_DEF,
                                     &TOUCH_PAD_SWIPE_OPTIONS_DEF, &INPUTTEXT_MODE_DEF};
    /** The allowed in/out data type scope of frontend apis.*/
    const std::initializer_list<std::string_view> DATA_TYPE_SCOPE = {
        "int",
        "signedInt",
        "float",
        "bool",
        "string",
        RECT_DEF.name_,
        POINT_DEF.name_,
        WINDOW_FILTER_DEF.name_,
        BY_DEF.name_,
        UI_DRIVER_DEF.name_,
        UI_COMPONENT_DEF.name_,
        ON_DEF.name_,
        DRIVER_DEF.name_,
        COMPONENT_DEF.name_,
        UI_WINDOW_DEF.name_,
        POINTER_MATRIX_DEF.name_,
        UI_EVENT_OBSERVER_DEF.name_,
        TOUCH_PAD_SWIPE_OPTIONS_DEF.name_,
        INPUTTEXT_MODE_DEF.name_,
    };
} // namespace OHOS::uitest

#endif