/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <chrono>
#include <iostream>
#include "velocity_tracker.h"

using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
void VelocityTracker::UpdateTouchEvent(const TouchEventInfo& event, bool end)
{
    isVelocityDone_ = false;
    if (isFirstPoint_) {
        firstPosition_ = event.GetOffset();
        isFirstPoint_ = false;
    } else {
        delta_ = event.GetOffset() - lastPosition_;
    }
    std::chrono::duration<double> diffTime = event.GetActionTimeStamp() - lastTimePoint_;
    lastTimePoint_ = event.GetActionTimeStamp();
    lastPosition_ = event.GetOffset();
    lastTrackPoint_ = event;
    static const double range = 0.05;
    if (delta_.IsZero() && end && (diffTime.count() < range)) {
        return;
    }
    // nanoseconds duration to seconds.
    xAxis_.UpdatePoint(event.durationSeconds, event.wx);
    yAxis_.UpdatePoint(event.durationSeconds, event.wy);
}

void VelocityTracker::UpdateVelocity()
{
    if (isVelocityDone_) {
        return;
    }
    // the least square method three params curve is 0 * x^3 + a2 * x^2 + a1 * x + a0
    // the velocity is 2 * a2 * x + a1;
    static const int32_t linearParam = 2;
    std::vector<double> xAxis { 3, 0 };
    auto xValue = xAxis_.GetTVals().back();
    double xVelocity = 0.0;
    if (xAxis_.GetLSMParams(xAxis)) {
        xVelocity = linearParam * xAxis[ZERO] * xValue + xAxis[ONE];
    }
    std::vector<double> yAxis { 3, 0 };
    auto yValue = yAxis_.GetTVals().back();
    double yVelocity = 0.0;
    if (yAxis_.GetLSMParams(yAxis)) {
        yVelocity = linearParam * yAxis[ZERO] * yValue + yAxis[ONE];
    }
    velocity_.SetOffsetPerSecond({ xVelocity, yVelocity });
    isVelocityDone_ = true;
}
} // namespace OHOS::uitest
