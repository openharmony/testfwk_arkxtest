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

#include <cmath>
#include "ui_action.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static void DecomposeClick(PointerMatrix &recv, const Point &point, const UiOpArgs &options)
    {
        constexpr uint32_t fingers = 1;
        constexpr uint32_t steps = 2;
        PointerMatrix pointer(fingers, steps);
        pointer.PushAction(TouchEvent {ActionStage::DOWN, point, 0, options.clickHoldMs_});
        pointer.PushAction(TouchEvent {ActionStage::UP, point, options.clickHoldMs_, 0});
        recv = move(pointer);
    }

    static void DecomposeLongClick(PointerMatrix &recv, const Point &point, const UiOpArgs &options)
    {
        // should sleep after touch-down to make long-click duration
        constexpr uint32_t fingers = 1;
        constexpr uint32_t steps = 2;
        PointerMatrix pointer(fingers, steps);
        pointer.PushAction(TouchEvent {ActionStage::DOWN, point, 0, options.longClickHoldMs_});
        pointer.PushAction(TouchEvent {ActionStage::UP, point, options.longClickHoldMs_, 0});
        recv = move(pointer);
    }

    static void DecomposeDoubleClick(PointerMatrix &recv, const Point &point, const UiOpArgs &options)
    {
        const auto msInterval = options.doubleClickIntervalMs_;
        constexpr uint32_t fingers = 1;
        constexpr uint32_t steps = 4;
        PointerMatrix pointer(fingers, steps);
        pointer.PushAction(TouchEvent {ActionStage::DOWN, point, 0, options.clickHoldMs_});
        pointer.PushAction(TouchEvent {ActionStage::UP, point, options.clickHoldMs_, msInterval});

        pointer.PushAction(TouchEvent {ActionStage::DOWN, point, 0, options.clickHoldMs_});
        pointer.PushAction(TouchEvent {ActionStage::UP, point, options.clickHoldMs_, 0});
        recv = move(pointer);
    }

    static void DecomposeComputeSwipe(PointerMatrix &recv, const Point &from, const Point &to, TouchOp type,
                                      const UiOpArgs &options)
    {
        const int32_t distanceX = to.px_ - from.px_;
        const int32_t distanceY = to.py_ - from.py_;
        const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
        const uint32_t timeCostMs = (distance * 1000) / options.swipeVelocityPps_;
        if (distance < 1) {
            // do not need to execute swipe
            return;
        }
        uint32_t steps = options.swipeStepsCounts_;
        uint32_t intervalMs = timeCostMs / steps + 1;
        constexpr uint32_t fingers = 1;
        constexpr uint32_t intervalMsInSwipe = 5;
        if (type != TouchOp::FLING) {
            steps = timeCostMs / intervalMsInSwipe;
            intervalMs = intervalMsInSwipe;
        }
        PointerMatrix pointer(fingers, steps + 1);

        pointer.PushAction(TouchEvent {ActionStage::DOWN, {from.px_, from.py_}, 0, intervalMs});
        float stepLengthX = static_cast<double>(distanceX) / static_cast<double>(steps);
        float stepLengthY = static_cast<double>(distanceY) / static_cast<double>(steps);

        for (uint32_t step = 1; step < steps; step++) {
            const int32_t pointX = from.px_ + stepLengthX * step;
            const int32_t pointY = from.py_ + stepLengthY * step;
            const uint32_t timeOffsetMs = (timeCostMs * step) / steps;
            pointer.PushAction(TouchEvent {ActionStage::MOVE, {pointX, pointY}, timeOffsetMs, intervalMs});
        }

        pointer.PushAction(TouchEvent {ActionStage::UP, {to.px_, to.py_}, timeCostMs, intervalMs});
        if (type == TouchOp::DRAG) {
            // drag needs longPressDown firstly
            pointer.At(fingers - 1, 0).holdMs_ += options.longClickHoldMs_;
            for (uint32_t idx = 1; idx < pointer.GetSize(); idx++) {
                pointer.At(fingers - 1, idx).downTimeOffsetMs_ += options.longClickHoldMs_;
            }
        }
        recv = move(pointer);
    }

    void GenericClick::Decompose(PointerMatrix &recv, const UiOpArgs &options) const
    {
        DCHECK(type_ >= TouchOp::CLICK && type_ <= TouchOp::DOUBLE_CLICK_P);
        switch (type_) {
            case CLICK:
                DecomposeClick(recv, point_, options);
                break;
            case LONG_CLICK:
                DecomposeLongClick(recv, point_, options);
                break;
            case DOUBLE_CLICK_P:
                DecomposeDoubleClick(recv, point_, options);
                break;
            default:
                break;
        }
        for (uint32_t index = 0; index < recv.GetSize(); index++) {
            recv.At(recv.GetFingers() - 1, index).flags_ = type_;
        }
    }

    void GenericSwipe::Decompose(PointerMatrix &recv, const UiOpArgs &options) const
    {
        DCHECK(type_ >= TouchOp::SWIPE && type_ <= TouchOp::FLING);
        DecomposeComputeSwipe(recv, from_, to_, type_, options);
        for (uint32_t index = 0; index < recv.GetSize(); index++) {
            recv.At(recv.GetFingers() - 1, index).flags_ = type_;
        }
    }

    void GenericPinch::Decompose(PointerMatrix &recv, const UiOpArgs &options) const
    {
        const int32_t distanceX0 = abs(rect_.GetCenterX() - rect_.left_) * abs(scale_ - 1);
        PointerMatrix pointer1;
        PointerMatrix pointer2;
        if (scale_ > 1) {
            auto fromPoint0 = Point(rect_.GetCenterX() - options.pinchWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint0 = Point((fromPoint0.px_ - distanceX0), rect_.GetCenterY());
            auto fromPoint1 = Point(rect_.GetCenterX() + options.pinchWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint1 = Point((fromPoint1.px_ + distanceX0), rect_.GetCenterY());
            DecomposeComputeSwipe(pointer1, fromPoint0, toPoint0, TouchOp::SWIPE, options);
            DecomposeComputeSwipe(pointer2, fromPoint1, toPoint1, TouchOp::SWIPE, options);
        } else if (scale_ < 1) {
            auto fromPoint0 = Point(rect_.left_ + options.pinchWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint0 = Point((fromPoint0.px_ + distanceX0), rect_.GetCenterY());
            auto fromPoint1 = Point(rect_.right_ - options.pinchWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint1 = Point((fromPoint1.px_- distanceX0), rect_.GetCenterY());
            DecomposeComputeSwipe(pointer1, fromPoint0, toPoint0, TouchOp::SWIPE, options);
            DecomposeComputeSwipe(pointer2, fromPoint1, toPoint1, TouchOp::SWIPE, options);
        }

        PointerMatrix pointer3(pointer1.GetFingers() + pointer2.GetFingers(), pointer1.GetSteps());
        for (uint32_t index = 0; index < pointer1.GetSize(); index++) {
            pointer3.PushAction(pointer1.At(0, index));
        }
        for (uint32_t index = 0; index < pointer2.GetSize(); index++) {
            pointer3.PushAction(pointer2.At(0, index));
        }
        recv = move(pointer3);
    }

    void MultiPointerAction::Decompose(PointerMatrix &recv, const UiOpArgs &options) const
    {
        PointerMatrix matrix(pointers_.GetFingers(), pointers_.GetSteps() + 1);
        constexpr int32_t flag = 0x10000; // set the low 16 bits of data as coordinates.
        for (uint32_t finger = 0; finger < pointers_.GetFingers(); finger++) {
            uint32_t timeOffsetMs = 0;
            uint32_t intervalMs = 0;
            constexpr uint32_t unitConversionConstant = 1000;
            for (uint32_t step = 0; step < pointers_.GetSteps() - 1; step++) {
                const int32_t pxTo = (pointers_.At(finger, step + 1).point_.px_) % flag;
                const int32_t pxFrom = (pointers_.At(finger, step).point_.px_) % flag;
                const int32_t distanceX = pxTo - pxFrom;
                const int32_t pyTo = pointers_.At(finger, step + 1).point_.py_;
                const int32_t pyFrom = pointers_.At(finger, step).point_.py_;
                const int32_t distanceY = pyTo - pyFrom;
                auto stayMs = (pointers_.At(finger, step).point_.px_) / flag;
                const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
                intervalMs = (distance * unitConversionConstant) / options.swipeVelocityPps_;
                auto holdMs = (stayMs == 0) ? intervalMs : stayMs;
                if (step == 0) {
                    matrix.PushAction(TouchEvent {ActionStage::DOWN, {pxFrom, pyFrom}, 0, holdMs});
                } else {
                    timeOffsetMs += intervalMs;
                    matrix.PushAction(TouchEvent {ActionStage::MOVE, {pxFrom, pyFrom}, timeOffsetMs, holdMs});
                }
            }
            auto endPx = (pointers_.At(finger, pointers_.GetSteps() - 1).point_.px_) % flag;
            auto endPy = pointers_.At(finger, pointers_.GetSteps() - 1).point_.py_;
            auto endTime = (pointers_.At(finger, pointers_.GetSteps() - 1).point_.px_) / flag;
            auto endStayTime = (endTime == 0) ? intervalMs : endTime;
            matrix.PushAction(TouchEvent {ActionStage::MOVE, {endPx, endPy}, timeOffsetMs, endStayTime});
            matrix.PushAction(TouchEvent {ActionStage::UP, {endPx, endPy}, timeOffsetMs, intervalMs});
        }
        recv = move(matrix);
    }

    void TouchPadAction::Decompose(vector<TouchPadEvent> &recv, const UiOpArgs &options,
                                   const Point displaySize) const
    {
        int32_t numTwo = 2;
        int32_t displayCenterX = displaySize.px_ / numTwo;
        int32_t displayCenterY = displaySize.py_ / numTwo;
        int32_t pxFrom = displayCenterX;
        int32_t pyFrom = displayCenterY;
        int32_t pxTo = displayCenterX;
        int32_t pyTo = displayCenterY;
        switch (direction_) {
            case TO_LEFT:
                pxFrom += displayCenterX / numTwo;
                pxTo -= displayCenterX / numTwo;
                break;
            case TO_RIGHT:
                pxFrom -= displayCenterX / numTwo;
                pxTo += displayCenterX / numTwo;
                break;
            case TO_UP:
                pyFrom += displayCenterY / numTwo;
                pyTo -= displayCenterY / numTwo;
                break;
            case TO_DOWN:
                pyFrom -= displayCenterY / numTwo;
                pyTo += displayCenterY / numTwo;
                break;
            default:
                break;
        }
        const int32_t distanceX = pxTo - pxFrom;
        const int32_t distanceY = pyTo - pyFrom;
        const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
        const uint32_t timeCostMs = (distance * 1000) / options.swipeVelocityPps_;
        constexpr uint32_t intervalMs = 5;
        uint32_t steps = timeCostMs / intervalMs;
        recv.push_back(TouchPadEvent {ActionStage::DOWN, {pxFrom, pyFrom}, fingers_, intervalMs});
        float stepLengthX = static_cast<double>(distanceX) / static_cast<double>(steps);
        float stepLengthY = static_cast<double>(distanceY) / static_cast<double>(steps);
        for (uint32_t step = 1; step < steps; step++) {
            const int32_t pointX = pxFrom + stepLengthX * step;
            const int32_t pointY = pyFrom + stepLengthY * step;
            recv.push_back(TouchPadEvent {ActionStage::MOVE, {pointX, pointY}, fingers_, intervalMs});
        }
        if (stay_) {
            uint32_t stayPointerTimes = 5;
            for (uint32_t stayPointerTime = 0; stayPointerTime < stayPointerTimes; stayPointerTime++) {
                recv.push_back(TouchPadEvent {ActionStage::MOVE, {pxTo, pyTo}, fingers_, 200});
            }
        }
        recv.push_back(TouchPadEvent {ActionStage::UP, {pxTo, pyTo}, fingers_, intervalMs});
    }

    PointerMatrix::PointerMatrix() {};

    PointerMatrix::PointerMatrix(uint32_t fingersNum, uint32_t stepsNum)
    {
        this->fingerNum_ = fingersNum;
        this->stepNum_ = stepsNum;
        this->capacity_ = this->fingerNum_ * this->stepNum_;
        this->size_ = 0;
        this->data_ = std::make_unique<TouchEvent[]>(this->capacity_);
    }

    PointerMatrix& PointerMatrix::operator=(PointerMatrix&& other)
    {
        this->data_ = move(other.data_);
        this->fingerNum_ = other.fingerNum_;
        this->stepNum_ = other.stepNum_;
        this->capacity_ = other.capacity_;
        this->size_ = other.size_;
        other.fingerNum_ = 0;
        other.stepNum_ = 0;
        other.capacity_ = 0;
        other.size_ = 0;
        return *this;
    }

    PointerMatrix::~PointerMatrix() {}

    void PointerMatrix::PushAction(const TouchEvent& ptr)
    {
        if (this->capacity_ == this->size_) {
            return;
        }
        *(this->data_.get() + this->size_) = ptr;
        this->size_++;
    }

    bool PointerMatrix::Empty() const
    {
        if (this->size_ == 0) {
            return true;
        }
        return false;
    }

    TouchEvent& PointerMatrix::At(uint32_t fingerIndex, uint32_t stepIndex) const
    {
        return *(this->data_.get() + (fingerIndex * this->stepNum_ + stepIndex));
    }

    uint32_t PointerMatrix::GetCapacity() const
    {
        return this->capacity_;
    }

    uint32_t PointerMatrix::GetSize() const
    {
        return this->size_;
    }

    uint32_t PointerMatrix::GetSteps() const
    {
        return this->stepNum_;
    }

    uint32_t PointerMatrix::GetFingers() const
    {
        return this->fingerNum_;
    }

    void PointerMatrix::SetToolType(const TouchToolType type)
    {
        touchToolType_ = type;
    }

    TouchToolType PointerMatrix::GetToolType() const
    {
        return touchToolType_;
    }

    void PointerMatrix::SetTouchPressure(const float pressure)
    {
        touchPressure_ = pressure;
    }

    float PointerMatrix::GetTouchPressure() const
    {
        return touchPressure_;
    }

    void PointerMatrix::ConvertToPenEvents(PointerMatrix &recv) const
    {
        DCHECK(this->fingerNum_ == 1);
        recv.SetToolType(TouchToolType::PEN);
        constexpr uint32_t intervalMs = 5;
        recv.PushAction(TouchEvent { ActionStage::PROXIMITY_IN, this->At(0, 0).point_, 0, intervalMs });
        for (uint32_t step = 0; step < stepNum_; step++) {
            auto touchEvent = At(0, step);
            recv.PushAction(touchEvent);
        }
        recv.PushAction(TouchEvent { ActionStage::PROXIMITY_OUT, this->At(0, this->GetSteps() - 1).point_, 0,
            intervalMs });
    }

    void PointerMatrix::ConvertToMouseEvents(vector<MouseEvent> &recv) const
    {
        for (uint32_t finger = 0; finger < fingerNum_; finger++) {
            for (uint32_t step = 0; step < stepNum_; step++) {
                auto touchEvent = At(finger, step);
                recv.push_back(MouseEvent {touchEvent.stage_, touchEvent.point_, MouseButton::BUTTON_LEFT, {},
                                           touchEvent.holdMs_});
            }
        }
    }

    void MouseMoveTo::Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const
    {
        recv.push_back(MouseEvent {ActionStage::MOVE, point_, MouseButton::BUTTON_NONE, {}, 0});
    }

    void MouseSwipe::Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const
    {
        DCHECK(type_ >= TouchOp::SWIPE && type_ <= TouchOp::DRAG);
        PointerMatrix touchEvents;
        DecomposeComputeSwipe(touchEvents, from_, to_, type_, opt);
        touchEvents.ConvertToMouseEvents(recv);
        if (type_ == TouchOp::SWIPE) {
            recv.front().stage_ = ActionStage::MOVE;
            recv.back().stage_ = ActionStage::MOVE;
            for (size_t index = 0; index < recv.size(); index++) {
                recv[index].button_ = MouseButton::BUTTON_NONE;
            }
        }
    }

    void MouseClick::Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const
    {
        DCHECK(type_ >= TouchOp::CLICK && type_ <= TouchOp::DOUBLE_CLICK_P);
        PointerMatrix touchEvents;
        switch (type_) {
            case CLICK:
                DecomposeClick(touchEvents, point_, opt);
                break;
            case LONG_CLICK:
                DecomposeLongClick(touchEvents, point_, opt);
                break;
            case DOUBLE_CLICK_P:
                DecomposeDoubleClick(touchEvents, point_, opt);
                break;
            default:
                break;
        }
        touchEvents.ConvertToMouseEvents(recv);
        for (size_t index = 0; index < recv.size(); index++) {
            recv[index].button_ = button_;
        }
        vector<KeyEvent> keyAction1;
        keyAction1.push_back(KeyEvent {ActionStage::DOWN, key1_, opt.keyHoldMs_});
        keyAction1.push_back(KeyEvent {ActionStage::DOWN, key2_, opt.keyHoldMs_});
        auto keyDown = MouseEvent {ActionStage::MOVE, point_, MouseButton::BUTTON_NONE, keyAction1, opt.clickHoldMs_};
        recv.insert(recv.begin(), keyDown);

        vector<KeyEvent> keyAction2;
        keyAction2.push_back(KeyEvent {ActionStage::UP, key2_, opt.keyHoldMs_});
        keyAction2.push_back(KeyEvent {ActionStage::UP, key1_, opt.keyHoldMs_});
        auto keyUp = MouseEvent {ActionStage::UP, point_, MouseButton::BUTTON_NONE, keyAction2, 0};
        recv.push_back(keyUp);
    }

    void MouseScroll::Decompose(std::vector<MouseEvent> &recv, const UiOpArgs &opt) const
    {
        recv.push_back(MouseEvent {ActionStage::MOVE, point_, MouseButton::BUTTON_NONE, {}, 0});
        constexpr int32_t thousandMilliseconds = 1000;
        auto focusTimeMs = thousandMilliseconds / speed_ / 2;
        auto stage  = (scrollValue_ > 0) ? AXIS_DOWN : AXIS_UP;
        vector<KeyEvent> keyAction1;
        keyAction1.push_back(KeyEvent {ActionStage::DOWN, key1_, opt.keyHoldMs_});
        keyAction1.push_back(KeyEvent {ActionStage::DOWN, key2_, opt.keyHoldMs_});
        recv.push_back(MouseEvent {stage, point_, MouseButton::BUTTON_NONE, keyAction1, focusTimeMs});
        recv.push_back(MouseEvent {ActionStage::AXIS_STOP, point_, MouseButton::BUTTON_NONE, {}, focusTimeMs});

        auto steps = abs(scrollValue_);
        for (auto index = 1; index < steps - 1; index++) {
            recv.push_back(MouseEvent {stage, point_, MouseButton::BUTTON_NONE, {}, focusTimeMs});
            recv.push_back(MouseEvent {ActionStage::AXIS_STOP, point_, MouseButton::BUTTON_NONE, {}, focusTimeMs});
        }

        vector<KeyEvent> keyAction2;
        keyAction2.push_back(KeyEvent {ActionStage::UP, key2_, opt.keyHoldMs_});
        keyAction2.push_back(KeyEvent {ActionStage::UP, key1_, opt.keyHoldMs_});
        if (steps > 1) {
            recv.push_back(MouseEvent {stage, point_, MouseButton::BUTTON_NONE, {}, focusTimeMs});
            recv.push_back(MouseEvent {ActionStage::AXIS_STOP, point_, MouseButton::BUTTON_NONE, keyAction2,
                                       focusTimeMs});
        } else {
            recv.push_back(MouseEvent {ActionStage::NONE, point_, MouseButton::BUTTON_NONE, keyAction2, focusTimeMs});
        }
    }

    void GenericAtomicAction::Decompose(PointerMatrix &recv, const UiOpArgs &options) const
    {
        DCHECK(stage_ >= ActionStage::DOWN && stage_ <= ActionStage::UP);
        constexpr uint32_t fingers = 1;
        constexpr uint32_t steps = 1;
        PointerMatrix pointer(fingers, steps);
        pointer.PushAction(TouchEvent {stage_, point_, 0, 0, 0});
        recv = move(pointer);
    }
}
