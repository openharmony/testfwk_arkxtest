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
#include "velocity_tracker.h"

using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
void VelocityTracker::UpdateTouchPoint(const TouchEventInfo& event, bool end)
{
    isVelocityDone_ = false;
    currentTrackPoint_ = event;
    if (isFirstPoint_) {
        downTrackPoint_ = firstTrackPoint_;
        firstPosition_ = event.GetOffset();
        firstTimePoint_ = event.time;
        firstTrackPoint_ = event;
        isFirstPoint_ = false;
    } else {
        delta_ = event.GetOffset() - lastPosition_;
    }
    std::chrono::duration<double> diffTime = event.time - lastTimePoint_;
    lastTimePoint_ = event.time;
    lastPosition_ = event.GetOffset() ;
    lastTrackPoint_ = event;
    static const double range = 0.05;
    if (delta_.IsZero() && end && (diffTime.count() < range)) {
        return;
    }
    // nanoseconds duration to seconds.
    std::chrono::duration<double> duration = event.time - firstTrackPoint_.time;
    seconds = duration.count();
    xAxis_.UpdatePoint(seconds, event.x);
    yAxis_.UpdatePoint(seconds, event.y);
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
    auto xValue = xAxis_.GetXVals().back();
    double xVelocity = 0.0;
    if (xAxis_.GetLeastSquareParams(xAxis)) {
        xVelocity = linearParam * xAxis[0] * xValue + xAxis[1];
    }
    std::vector<double> yAxis { 3, 0 };
    auto yValue = yAxis_.GetXVals().back();
    double yVelocity = 0.0;
    if (yAxis_.GetLeastSquareParams(yAxis)) {
        yVelocity = linearParam * yAxis[0] * yValue + yAxis[1];
    }
    velocity_.SetOffsetPerSecond({ xVelocity, yVelocity });
    isVelocityDone_ = true;
}
} // namespace OHOS::uitest
