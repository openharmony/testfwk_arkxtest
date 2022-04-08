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

    void UiDriveOptions::WriteIntoParcel(json &data) const
    {
        auto arr = json::array();
        arr.push_back(clickHoldMs_);
        arr.push_back(longClickHoldMs_);
        arr.push_back(doubleClickIntervalMs_);
        arr.push_back(keyHoldMs_);
        arr.push_back(swipeVelocityPps_);
        arr.push_back(uiSteadyThresholdMs_);
        arr.push_back(waitUiSteadyMaxMs_);
        data["data"] = arr;
    }

    void UiDriveOptions::ReadFromParcel(const json &data)
    {
        auto arr = data["data"];
        size_t index = 0;
        clickHoldMs_ = arr.at(index++);
        longClickHoldMs_ = arr.at(index++);
        doubleClickIntervalMs_ = arr.at(index++);
        keyHoldMs_ = arr.at(index++);
        swipeVelocityPps_ = arr.at(index++);
        uiSteadyThresholdMs_ = arr.at(index++);
        waitUiSteadyMaxMs_ = arr.at(index++);
    }

    static void DecomposeClick(vector<TouchEvent> &recv, const Point &point, const UiDriveOptions &options)
    {
        recv.push_back(TouchEvent {ActionStage::DOWN, point, 0, options.clickHoldMs_});
        recv.push_back(TouchEvent {ActionStage::UP, point, options.clickHoldMs_, 0});
    }

    static void DecomposeLongClick(vector<TouchEvent> &recv, const Point &point, const UiDriveOptions &options)
    {
        // should sleep after touch-down to make long-click duration
        recv.push_back(TouchEvent {ActionStage::DOWN, point, 0, options.longClickHoldMs_});
        recv.push_back(TouchEvent {ActionStage::UP, point, options.longClickHoldMs_, 0});
    }

    static void DecomposeDoubleClick(vector<TouchEvent> &recv, const Point &point, const UiDriveOptions &options)
    {
        const auto msInterval = options.doubleClickIntervalMs_;
        recv.push_back(TouchEvent {ActionStage::DOWN, point, 0, options.clickHoldMs_});
        recv.push_back(TouchEvent {ActionStage::UP, point, options.clickHoldMs_, msInterval});

        recv.push_back(TouchEvent {ActionStage::DOWN, point, 0, options.clickHoldMs_});
        recv.push_back(TouchEvent {ActionStage::UP, point, options.clickHoldMs_, 0});
    }

    static void DecomposeComputeSwipe(vector<TouchEvent> &recv, const Point &from, const Point &to, bool drag,
                                      const UiDriveOptions &options)
    {
        const int32_t distanceX = to.px_ - from.px_;
        const int32_t distanceY = to.py_ - from.py_;
        const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
        const uint32_t timeCostMs = (distance * 1000) / options.swipeVelocityPps_;
        if (distance < 1) {
            // do not need to execute swipe
            return;
        }
        constexpr uint16_t steps = 50;
        const uint32_t intervalMs = timeCostMs / steps + 1;
        recv.push_back(TouchEvent {ActionStage::DOWN, {from.px_, from.py_}, 0, intervalMs});
        for (uint16_t step = 1; step < steps; step++) {
            const int32_t pointX = from.px_ + (distanceX * step) / steps;
            const int32_t pointY = from.py_ + (distanceY * step) / steps;
            const uint32_t timeOffsetMs = (timeCostMs * step) / steps;
            recv.push_back(TouchEvent {ActionStage::MOVE, {pointX, pointY}, timeOffsetMs, intervalMs});
        }
        recv.push_back(TouchEvent {ActionStage::UP, {to.px_, to.py_}, timeCostMs, intervalMs});
        if (drag) {
            // drag needs longPressDown firstly
            recv.at(0).holdMs_ += options.longClickHoldMs_;
            for (std::size_t idx = 1; idx < recv.size(); idx++) {
                recv.at(idx).downTimeOffsetMs_ += options.longClickHoldMs_;
            }
        }
    }

    void GenericClick::Decompose(vector<TouchEvent> &recv, const Point &point, const UiDriveOptions &options) const
    {
        DCHECK(type_ >= PointerOp::CLICK_P && type_ <= PointerOp::DOUBLE_CLICK_P);
        switch (type_) {
            case CLICK_P:
                DecomposeClick(recv, point, options);
                break;
            case LONG_CLICK_P:
                DecomposeLongClick(recv, point, options);
                break;
            case DOUBLE_CLICK_P:
                DecomposeDoubleClick(recv, point, options);
                break;
            default:
                break;
        }
        for (auto &event:recv) {
            event.flags_ = type_;
        }
    }

    void GenericSwipe::Decompose(vector<TouchEvent> &recv, const Point &fromPoint, const Point &toPoint,
                                 const UiDriveOptions &options) const
    {
        DCHECK(type_ >= PointerOp::SWIPE_P && type_ <= PointerOp::DRAG_P);
        DecomposeComputeSwipe(recv, fromPoint, toPoint, type_ == PointerOp::DRAG_P, options);
        for (auto &event:recv) {
            event.flags_ = type_;
        }
    }
}
