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

#include <vector>
#include "ui_model.h"

namespace OHOS::uitest {
    // frequently used keys.
    constexpr int32_t KEYCODE_NONE = 0;
    constexpr int32_t KEYCODE_BACK = 2;
    constexpr int32_t KEYCODE_DPAD_RIGHT = 2015;
    constexpr int32_t KEYCODE_DEL = 2055;
    constexpr int32_t KEYCODE_CTRL = 2072;
    constexpr int32_t KEYCODE_V = 2038;
    constexpr int32_t KEYCODE_POWER = 18;
    constexpr int32_t KEYCODE_HOME = 1;
    constexpr int32_t KEYCODE_D = 2020;
    constexpr int32_t KEYCODE_WIN = 2076;
    constexpr int32_t KEYCODE_MOVETOEND = 2082;

    /**Enumerates all the supported coordinate-based touch operations.*/
    enum TouchOp : uint8_t { CLICK, LONG_CLICK, DOUBLE_CLICK_P, SWIPE, DRAG, FLING};

    /**Enumerates the supported Key actions.*/
    enum UiKey : uint8_t { BACK, GENERIC };

    enum ActionStage : uint8_t {
        NONE = 0,
        DOWN = 1,
        MOVE = 2,
        UP = 3,
        AXIS_UP = 4,
        AXIS_DOWN = 5,
        AXIS_STOP = 6,
        PROXIMITY_IN = 7,
        PROXIMITY_OUT = 8
    };

    enum TouchToolType : uint8_t { FINGER = 0, PEN = 1, KNUCKLE = 8 };

    enum ResizeDirection : uint8_t {
        LEFT,
        RIGHT,
        D_UP,
        D_DOWN,
        LEFT_UP,
        LEFT_DOWN,
        RIGHT_UP,
        RIGHT_DOWN
    };

    enum DisplayRotation : uint32_t {
        ROTATION_0,
        ROTATION_90,
        ROTATION_180,
        ROTATION_270
    };

    enum MouseButton : int32_t {
        BUTTON_NONE = -1,
        BUTTON_LEFT = 0,
        BUTTON_RIGHT = 1,
        BUTTON_MIDDLE = 2
    };

    enum Direction : uint32_t {
        TO_LEFT,
        TO_RIGHT,
        TO_UP,
        TO_DOWN
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

    struct MouseEvent {
        ActionStage stage_;
        Point point_;
        MouseButton button_;
        vector<KeyEvent> keyEvents_;
        uint32_t holdMs_;
    };

    struct TouchPadEvent {
        ActionStage stage;
        Point point;
        uint32_t fingerCount;
        uint32_t holdMs;
    };

    class PointerMatrix : public BackendClass {
    public:
        PointerMatrix();

        PointerMatrix(uint32_t fingersNum, uint32_t stepsNum);

        PointerMatrix(PointerMatrix&& other);

        PointerMatrix& operator=(PointerMatrix&& other);

        ~PointerMatrix() override;

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return POINTER_MATRIX_DEF;
        }

        void PushAction(const TouchEvent& ptr);

        bool Empty() const;

        TouchEvent& At(uint32_t fingerIndex, uint32_t stepIndex) const;

        uint32_t GetCapacity() const;

        uint32_t GetSize() const;

        uint32_t GetSteps() const;

        uint32_t GetFingers() const;

        void SetToolType(const TouchToolType type);

        TouchToolType GetToolType() const;

        void SetTouchPressure(const float pressure);

        float GetTouchPressure() const;

        void ConvertToMouseEvents(vector<MouseEvent> &recv) const;

        void ConvertToPenEvents(PointerMatrix &recv) const;

        bool IsSyncInject() const;

        void SetSyncInject();
    private:
        std::unique_ptr<TouchEvent[]> data_ = nullptr;
        uint32_t capacity_ = 0;
        uint32_t stepNum_ = 0;
        uint32_t fingerNum_ = 0;
        uint32_t size_ = 0;
        TouchToolType touchToolType_ = TouchToolType::FINGER;
        float touchPressure_ = 1.0;
        bool syncInject_ = false;
    };

    /**
     * Options of the UI operations, initialized with system default values.
     **/
    class UiOpArgs {
    public:
        const uint32_t maxSwipeVelocityPps_ = 40000;
        const uint32_t minSwipeVelocityPps_ = 200;
        const uint32_t defaultSwipeVelocityPps_ = 600;
        const uint32_t maxMultiTouchFingers = 10;
        const uint32_t maxMultiTouchSteps = 1000;
        const uint32_t defaultTouchPadSwipeVelocityPps_ = 2000;
        uint32_t clickHoldMs_ = 100;
        uint32_t longClickHoldMs_ = 1500;
        uint32_t doubleClickIntervalMs_ = 200;
        uint32_t keyHoldMs_ = 100;
        uint32_t swipeVelocityPps_ = defaultSwipeVelocityPps_;
        uint32_t uiSteadyThresholdMs_ = 1000;
        uint32_t waitUiSteadyMaxMs_ = 3000;
        uint32_t waitWidgetMaxMs_ = 5000;
        int32_t scrollWidgetDeadZone_ = 80; // make sure the scrollWidget does not slide more than one page.
        int32_t pinchWidgetDeadZone_ = 40;  // pinching at the edges of the widget has no effect.
        uint16_t swipeStepsCounts_ = 50;
        float touchPressure_ = 1.0;
        bool inputByPasteBoard_ = false;
        bool inputAdditional_ = false;
    };

    class TouchAction {
    public:
        /**Compute the touch event sequence that are needed to implement this action.
         * @param recv: the event seqence receiver.
         * @param options the ui operation agruments.
         * */
        virtual void Decompose(PointerMatrix &recv, const UiOpArgs &options) const = 0;
    };

    /**
     * Base type of all raw pointer click actions.
     **/
    class GenericClick : public TouchAction {
    public:
        GenericClick(TouchOp type, const Point &point) : type_(type), point_(point) {};

        void Decompose(PointerMatrix &recv, const UiOpArgs &options) const override;

        ~GenericClick() = default;

    private:
        const TouchOp type_;
        const Point point_;
    };

    class GenericMultiClick : public TouchAction {
    public:
        GenericMultiClick(const vector<Point> points, const int32_t times) : points_(points), times_(times) {};

        void Decompose(PointerMatrix &recv, const UiOpArgs &options) const override;

        ~GenericMultiClick() = default;

    private:
        const vector<Point> points_;
        const int32_t times_;
    };

    /**
     * Base type of all raw pointer swipe actions.
     **/
    class GenericSwipe : public TouchAction {
    public:
        explicit GenericSwipe(TouchOp type, const Point &from, const Point &to) : type_(type), from_(from), to_(to) {};

        void Decompose(PointerMatrix &recv, const UiOpArgs &options) const override;

        ~GenericSwipe() = default;

    private:
        const TouchOp type_;
        const Point from_;
        const Point to_;
    };

    /**
     * Base type of all raw pointer pinch actions.
     **/
    class GenericPinch : public TouchAction {
    public:
        explicit GenericPinch(const Rect &rect, float_t scale) : rect_(rect), scale_(scale) {};

        void Decompose(PointerMatrix &recv, const UiOpArgs &options) const override;

        ~GenericPinch() = default;

    private:
        const Rect rect_;
        const float_t scale_;
    };

    /**
     * Base type of multi pointer actions.
     **/
    class MultiPointerAction : public TouchAction {
    public:
        explicit MultiPointerAction(const PointerMatrix &pointer) : pointers_(pointer) {};

        void Decompose(PointerMatrix &recv, const UiOpArgs &options) const override;

        ~MultiPointerAction() = default;

    private:
        const PointerMatrix& pointers_;
    };

    /**
     * Base type of touchpad gesture actions.
     **/
    class TouchPadAction {
    public:
        explicit TouchPadAction(const int32_t fingers, const Direction direction, const bool stay)
            : fingers_(fingers), direction_(direction), stay_(stay) {};

        void Decompose(std::vector<TouchPadEvent> &recv, const UiOpArgs &options, const Point displaySize) const;

        ~TouchPadAction() = default;

    private:
        const int32_t fingers_;
        const Direction direction_;
        const bool stay_;
    };

    /**
     * Base type of all key actions.
     * */
    class KeyAction {
    public:
        /**Compute the key event sequence that are needed to implement this action.*/
        virtual void ComputeEvents(std::vector<KeyEvent> &recv, const UiOpArgs &options) const = 0;

        virtual ~KeyAction() = default;
    };

    class KeysForwarder : public KeyAction {
    public:
        explicit KeysForwarder(const vector<KeyEvent> &evetns) : events_(evetns) {};

        void ComputeEvents(vector<KeyEvent> &recv, const UiOpArgs &opt) const override
        {
            recv = events_;
        }

    private:
        const vector<KeyEvent> &events_;
    };

    /**Base type of named single-key actions with at most 1 ctrl key.*/
    template<uint32_t kCode, uint32_t kCtrlCode = KEYCODE_NONE>
    class NamedPlainKey : public KeyAction {
    public:
        explicit NamedPlainKey() = default;

        void ComputeEvents(std::vector<KeyEvent> &recv, const UiOpArgs &opt) const override
        {
            if (kCtrlCode != KEYCODE_NONE) {
                recv.push_back(KeyEvent {ActionStage::DOWN, kCtrlCode,  0});
            }
            recv.push_back(KeyEvent {ActionStage::DOWN, kCode, opt.keyHoldMs_});
            if (kCtrlCode != KEYCODE_NONE) {
                recv.push_back(KeyEvent {ActionStage::UP, kCtrlCode, 0});
            }
            recv.push_back(KeyEvent {ActionStage::UP, kCode, 0});
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

    private:
        const int32_t code_;
    };

    /**Generic Combinedkeys actions.*/
    class CombinedKeys final : public KeyAction {
    public:
        CombinedKeys(int32_t codeZero, int32_t codeOne, int32_t codeTwo)
            : codeZero_(codeZero), codeOne_(codeOne), codeTwo_(codeTwo) {};

        void ComputeEvents(std::vector<KeyEvent> &recv, const UiOpArgs &opt) const override
        {
            recv.push_back(KeyEvent {ActionStage::DOWN, codeZero_, 0});
            recv.push_back(KeyEvent {ActionStage::DOWN, codeOne_, 0});
            if (codeTwo_ != KEYCODE_NONE) {
                recv.push_back(KeyEvent {ActionStage::DOWN, codeTwo_, opt.keyHoldMs_});
            } else {
                recv.at(INDEX_ONE).holdMs_ = opt.keyHoldMs_;
            }
            recv.push_back(KeyEvent {ActionStage::UP, codeZero_, 0});
            recv.push_back(KeyEvent {ActionStage::UP, codeOne_, 0});
            if (codeTwo_ != KEYCODE_NONE) {
                recv.push_back(KeyEvent {ActionStage::UP, codeTwo_, 0});
            }
        }

    private:
        const int32_t codeZero_;
        const int32_t codeOne_;
        const int32_t codeTwo_;
    };

    using Back = NamedPlainKey<KEYCODE_BACK>;
    using Power = NamedPlainKey<KEYCODE_POWER>;
    using Home = NamedPlainKey<KEYCODE_HOME>;
    using Paste = NamedPlainKey<KEYCODE_V, KEYCODE_CTRL>;
    using MoveToEnd = NamedPlainKey<KEYCODE_MOVETOEND>;

    class MouseAction {
    public:
        /**Compute the mouse event sequence that are needed to implement this action.
         * @param recv: the event seqence receiver.
         * @param options the ui operation agruments.
         * */
        virtual void Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const = 0;
    };

    class MouseMoveTo : public MouseAction {
    public:
        explicit MouseMoveTo(const Point &point) : point_(point) {};

        void Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const override;

        ~MouseMoveTo() = default;

    private:
        const Point point_;
    };

    class MouseSwipe : public MouseAction {
    public:
        explicit MouseSwipe(TouchOp type, const Point &from, const Point &to) : type_(type), from_(from), to_(to) {};

        void Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const override;

        ~MouseSwipe() = default;

    private:
        const TouchOp type_;
        const Point from_;
        const Point to_;
    };

    class MouseClick : public MouseAction {
    public:
        explicit MouseClick(TouchOp type, const Point &point, const MouseButton &button, int32_t &key1, int32_t &key2)
            : type_(type), point_(point), button_(button), key1_(key1), key2_(key2) {};

        void Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const override;

        ~MouseClick() = default;

    private:
        const TouchOp type_;
        const Point point_;
        const MouseButton button_;
        const int32_t key1_;
        const int32_t key2_;
    };

    class MouseScroll : public MouseAction {
    public:
        explicit MouseScroll(const Point &point, int32_t scrollValue, int32_t key1, int32_t key2, uint32_t speed)
            : point_(point),  scrollValue_(scrollValue), key1_(key1), key2_(key2),  speed_(speed) {};

        void Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const override;

        ~MouseScroll() = default;

    private:
        const Point point_;
        const int32_t scrollValue_;
        const int32_t key1_;
        const int32_t key2_;
        const uint32_t speed_;
    };

    /**
     * Base type of all atomic actions.
     * */
    class GenericAtomicAction : public TouchAction {
    public:
        explicit GenericAtomicAction(const ActionStage stage, const Point point) : stage_(stage), point_(point) {};

        void Decompose(PointerMatrix &recv, const UiOpArgs &options) const override;

        ~GenericAtomicAction() = default;

    private:
        const ActionStage stage_;
        const Point point_;
    };

    /**
     * Base type of all atomic mouse actions.
     * */
    class GenericAtomicMouseAction : public MouseAction {
    public:
        explicit GenericAtomicMouseAction(const ActionStage stage, const Point point, const MouseButton btn)
            : stage_(stage), point_(point), btn_(btn) {};

        void Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &options) const override;

        ~GenericAtomicMouseAction() = default;

    private:
        const ActionStage stage_;
        const Point point_;
        const MouseButton btn_;
    };
}

#endif