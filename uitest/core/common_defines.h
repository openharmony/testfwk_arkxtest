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

#ifndef COMMON_DEFINES_H
#define COMMON_DEFINES_H

#include <string_view>

namespace OHOS::uitest {
    constexpr size_t INDEX_ZERO = 0;
    constexpr size_t INDEX_ONE = 1;
    constexpr size_t INDEX_TWO = 2;
    constexpr size_t INDEX_THREE = 3;
    constexpr size_t INDEX_FOUR = 4;

    using CStr = const char *;

    /**Enumerates the supported data types in the uitest API transactions.*/
    enum TypeId : uint8_t { NONE, INT, FLOAT, BOOL, STRING, BY, COMPONENT, DRIVER, RECT_JSON };

    /**Enumerates the supported string value match rules.*/
    enum ValueMatchRule : uint8_t { EQ, CONTAINS, STARTS_WITH, ENDS_WITH };

    /**Enumerates the supported UiComponent attributes.*/
    enum UiAttr : uint8_t {
        ID,
        TEXT,
        KEY,
        TYPE,
        BOUNDS,
        ENABLED,
        FOCUSED,
        SELECTED,
        CLICKABLE,
        LONG_CLICKABLE,
        SCROLLABLE,
        CHECKABLE,
        CHECKED
    };

    /**Supported UiComponent attribute names. Ordered by <code>UiAttr</code> definition.*/
    constexpr CStr ATTR_NAMES[] = {
        "id",            // ID
        "text",          // TEXT
        "key",           // KEY
        "type",          // TYPE
        "bounds",        // BOUNDS
        "enabled",       // ENABLED
        "focused",       // FOCUSED
        "selected",      // SELECTED
        "clickable",     // CLICKABLE
        "longClickable", // LONG_CLICKABLE
        "scrollable",    // SCROLLABLE
        "checkable",     // CHECKABLE
        "checked",       // CHECKED
    };

    /**Supported UiComponent attribute types. Ordered by <code>UiAttr</code> definition.*/
    constexpr TypeId ATTR_TYPES[] = {
        INT,       // id
        STRING,    // text
        STRING,    // key
        STRING,    // type
        RECT_JSON, // bounds
        BOOL,      // enabled
        BOOL,      // focused
        BOOL,      // selected
        BOOL,      // clickable
        BOOL,      // longClickable
        BOOL,      // scrollable
        BOOL,      // checkable
        BOOL       // checked
    };

    /**Enumerates all the supported widget operations.*/
    enum WidgetOp : uint8_t { CLICK, LONG_CLICK, DOUBLE_CLICK, SCROLL_TO_TOP, SCROLL_TO_BOTTOM };

    /**Enumerates all the supported coordinate-based operations.*/
    enum PointerOp : uint8_t { CLICK_P, LONG_CLICK_P, DOUBLE_CLICK_P, SWIPE_P, DRAG_P };

    /**Enumerates the supported Key actions.*/
    enum UiKey : uint8_t { BACK, GENERIC };

    /**Message keys used in the api transactions.*/
    constexpr auto KEY_UPDATED_CALLER = "updated_caller";
    constexpr auto KEY_RESULT_VALUES = "result_values";
    constexpr auto KEY_EXCEPTION = "exception";
    constexpr auto KEY_CODE = "code";
    constexpr auto KEY_MESSAGE = "message";
    constexpr auto KEY_DATA_TYPE = "type";
    constexpr auto KEY_DATA_VALUE = "value";

    /** Specifications of a json-property.*/
    struct JsonPropSpec {
        std::string_view name_;
        TypeId type_;
        bool required_;
    };

    /**Rectangle json-properties.*/
    constexpr auto RECT_ATTR_LT_X = "leftX";
    constexpr auto RECT_ATTR_LT_Y = "topY";
    constexpr auto RECT_ATTR_RB_X = "rightX";
    constexpr auto RECT_ATTR_RB_Y = "bottomY";

    constexpr JsonPropSpec RECT_JSON_PROP_SPECS[4] = {
        {RECT_ATTR_LT_X, TypeId::INT, true}, // leftX
        {RECT_ATTR_LT_Y, TypeId::INT, true}, // topY
        {RECT_ATTR_RB_X, TypeId::INT, true}, // rightX
        {RECT_ATTR_RB_Y, TypeId::INT, true}  // bottomY
    };
} // namespace OHOS::uitest

#endif