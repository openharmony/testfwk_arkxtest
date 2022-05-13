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
#ifndef UI_ACTION_H
#define UI_ACTION_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "ui_model.h"

namespace OHOS::uitest {
    // frequently used keys.
    constexpr int32_t KEYCODE_NONE = 0;
    constexpr int32_t KEYCODE_BACK = 2;
    constexpr int32_t KEYCODE_CTRL = 2072;
    constexpr int32_t KEYCODE_V = 2038;
    constexpr char KEYNAME_BACK[] = "Back";
    constexpr char KEYNAME_PASTE[] = "Paste";

    /**Enumerates all the supported widget operations.*/
    enum WidgetOp : uint8_t { CLICK, LONG_CLICK, DOUBLE_CLICK, SCROLL_TO_TOP, SCROLL_TO_BOTTOM };

    /**Enumerates all the supported coordinate-based operations.*/
    enum PointerOp : uint8_t { CLICK_P, LONG_CLICK_P, DOUBLE_CLICK_P, SWIPE_P, DRAG_P };

    /**Enumerates the supported Key actions.*/
    enum UiKey : uint8_t { BACK, GENERIC };

    enum ActionStage : uint8_t {
        DOWN = 0, MOVE = 1, UP = 2
    };

    struct TouchEvent {
        ActionStage stage_;
        Point point_;
        uint32_t downTimeOffsetMs_;
        uint32_t holdMs_;
        uint32_t flags_;
    };

    struct KeyEvent {
        ActionStage stage_;
        int32_t code_;
        uint32_t holdMs_;
    };

    /**
     * Options of the UI operations, initialized with system default values.
     **/
    class UiOpArgs {
    public:
        uint32_t clickHoldMs_ = 200;
        uint32_t longClickHoldMs_ = 1500;
        uint32_t doubleClickIntervalMs_ = 200;
        uint32_t keyHoldMs_ = 100;
        uint32_t swipeVelocityPps_ = 600;
        uint32_t uiSteadyThresholdMs_ = 1000;
        uint32_t waitUiSteadyMaxMs_ = 3000;
        uint32_t waitWidgetMaxMs_ = 5000;
        int32_t scrollWidgetDeadZone_ = 20;
    };

    /**
     * Base type of all raw pointer click actions.
     **/
    class GenericClick {
    public:
        explicit GenericClick(PointerOp type) : type_(type) {};

        /**Compute the touch event sequence that are needed to implement this action.
         * @param point: the click location.
         * */
        void Decompose(std::vector<TouchEvent> &recv, const Point &point, const UiOpArgs &options) const;

        ~GenericClick() = default;

    private:
        const PointerOp type_;
    };

    /**
     * Base type of all raw pointer swipe actions.
     **/
    class GenericSwipe {
    public:
        explicit GenericSwipe(PointerOp type) : type_(type) {};

        /**Compute the touch event sequence that are needed to implement this action.
         * @param fromPoint: the swipe start point.
         * @param toPoint: the swipe end point.
         * */
        void Decompose(std::vector<TouchEvent> &recv, const Point &fromPoint, const Point &toPoint,
                       const UiOpArgs &options) const;

        ~GenericSwipe() = default;

    private:
        const PointerOp type_;
    };

    /**
     * Base type of all key actions.
     * */
    class KeyAction {
    public:
        /**Compute the key event sequence that are needed to implement this action.*/
        virtual void ComputeEvents(std::vector<KeyEvent> &recv, const UiOpArgs &options) const = 0;

        virtual ~KeyAction() = default;

        /**Describes this key action.*/
        virtual std::string Describe() const = 0;
    };

    /**Base type of named single-key actions with at most 1 ctrl key.*/
    template<CStr kName, uint32_t kCode, uint32_t kCtrlCode = KEYCODE_NONE>
    class NamedPlainKey : public KeyAction {
    public:
        explicit NamedPlainKey() = default;

        void ComputeEvents(std::vector<KeyEvent> &recv, const UiOpArgs &opt) const override
        {
            if (kCtrlCode != KEYCODE_NONE) {
                recv.push_back(KeyEvent {ActionStage::DOWN, kCtrlCode,  0});
            }
            recv.push_back(KeyEvent {ActionStage::DOWN, kCode, opt.keyHoldMs_});
            recv.push_back(KeyEvent {ActionStage::UP, kCode, 0});
            if (kCtrlCode != KEYCODE_NONE) {
                recv.push_back(KeyEvent {ActionStage::UP, kCtrlCode, 0});
            }
        }

        std::string Describe() const override
        {
            if constexpr (kName != nullptr) {
                return kName;
            }
            std::string desc = std::string("key_") + std::to_string(kCode);
            if constexpr (kCtrlCode != KEYCODE_NONE) {
                desc = desc + "(ctrlKey=" + std::to_string(kCtrlCode) + ")";
            }
            return desc;
        }
    };

    /**Generic key actions without name and ctrl key.*/
    class AnonymousSingleKey final : public KeyAction {
    public:
        explicit AnonymousSingleKey(int32_t code) : code_(code) {};

        void ComputeEvents(std::vector<KeyEvent> &recv, const UiOpArgs &opt) const override
        {
            recv.push_back(KeyEvent {ActionStage::DOWN, code_, opt.keyHoldMs_});
            recv.push_back(KeyEvent {ActionStage::UP, code_, 0});
        }

        std::string Describe() const override
        {
            return std::string("key_") + std::to_string(code_);
        }

    private:
        const int32_t code_;
    };

    using Back = NamedPlainKey<KEYNAME_BACK, KEYCODE_BACK>;
    using Paste = NamedPlainKey<KEYNAME_PASTE, KEYCODE_V, KEYCODE_CTRL>;
}

#endif