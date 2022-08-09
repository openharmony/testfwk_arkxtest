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

    static void DecomposeComputeSwipe(PointerMatrix &recv, const Point &from, const Point &to, bool drag,
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
        const auto steps = options.swipeStepsCounts_;
        const uint32_t intervalMs = timeCostMs / steps + 1;
        constexpr uint32_t fingers = 1;
        PointerMatrix pointer(fingers, steps + 1);

        pointer.PushAction(TouchEvent {ActionStage::DOWN, {from.px_, from.py_}, 0, intervalMs});
        for (uint16_t step = 1; step < steps; step++) {
            const int32_t pointX = from.px_ + (distanceX * step) / steps;
            const int32_t pointY = from.py_ + (distanceY * step) / steps;
            const uint32_t timeOffsetMs = (timeCostMs * step) / steps;
            pointer.PushAction(TouchEvent {ActionStage::MOVE, {pointX, pointY}, timeOffsetMs, intervalMs});
        }

        pointer.PushAction(TouchEvent {ActionStage::UP, {to.px_, to.py_}, timeCostMs, intervalMs});
        if (drag) {
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
        DCHECK(type_ >= TouchOp::SWIPE && type_ <= TouchOp::DRAG);
        DecomposeComputeSwipe(recv, from_, to_, type_ == TouchOp::DRAG, options);
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
            auto fromPoint0 = Point(rect_.GetCenterX() - options.scrollWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint0 = Point((rect_.GetCenterX() - distanceX0), rect_.GetCenterY());
            auto fromPoint1 = Point(rect_.GetCenterX() + options.scrollWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint1 = Point((rect_.GetCenterX() + distanceX0), rect_.GetCenterY());
            DecomposeComputeSwipe(pointer1, fromPoint0, toPoint0, false, options);
            DecomposeComputeSwipe(pointer2, fromPoint1, toPoint1, false, options);
        } else if (scale_ < 1) {
            auto fromPoint0 = Point(rect_.left_ + options.scrollWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint0 = Point((rect_.left_ + distanceX0), rect_.GetCenterY());
            auto fromPoint1 = Point(rect_.right_ - options.scrollWidgetDeadZone_, rect_.GetCenterY());
            auto toPoint1 = Point((rect_.right_ - distanceX0), rect_.GetCenterY());
            DecomposeComputeSwipe(pointer1, fromPoint0, toPoint0, false, options);
            DecomposeComputeSwipe(pointer2, fromPoint1, toPoint1, false, options);
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
        PointerMatrix matrix(pointers_.GetFingers(), pointers_.GetSteps());
        for (uint32_t finger = 0; finger < pointers_.GetFingers(); finger++) {
            uint32_t timeOffsetMs = 0;
            uint32_t intervalMs = 0;
            constexpr uint32_t unitConversionConstant = 1000;
            for (uint32_t step = 0; step < pointers_.GetSteps() - 1; step++) {
                const int32_t pxTo = pointers_.At(finger, step + 1).point_.px_;
                const int32_t pxFrom = pointers_.At(finger, step).point_.px_;
                const int32_t distanceX = pxTo - pxFrom;
                const int32_t pyTo = pointers_.At(finger, step + 1).point_.py_;
                const int32_t pyFrom = pointers_.At(finger, step).point_.py_;
                const int32_t distanceY = pyTo - pyFrom;
                const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
                intervalMs = (distance * unitConversionConstant) / options.swipeVelocityPps_;
                if (distance < 1) {
                    return;
                }
                if (step == 0) {
                    matrix.PushAction(TouchEvent {ActionStage::DOWN, {pxFrom, pyFrom}, 0, intervalMs});
                } else {
                    timeOffsetMs += intervalMs;
                    matrix.PushAction(TouchEvent {ActionStage::MOVE, {pxFrom, pyFrom}, timeOffsetMs, intervalMs});
                }
            }
            auto endPx = pointers_.At(finger, pointers_.GetSteps() - 1).point_.px_;
            auto endPy = pointers_.At(finger, pointers_.GetSteps() - 1).point_.py_;
            matrix.PushAction(TouchEvent {ActionStage::UP, {endPx, endPy}, timeOffsetMs, intervalMs});
        }
        recv = move(matrix);
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
}
