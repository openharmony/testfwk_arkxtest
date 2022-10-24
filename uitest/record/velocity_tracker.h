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

#ifndef GESTURES_VELOCITY_TRACKER_H
#define GESTURES_VELOCITY_TRACKER_H
#include <fstream>
#include <regex>
#include <iostream>
#include <unistd.h>
#include "least_square_impl.h"
#include "touch_event.h"
#include "offset.h"
#include "velocity.h"

namespace OHOS::uitest {

enum class Axis {
    VERTICAL = 0,
    HORIZONTAL,
    FREE,
    NONE,
};

class VelocityTracker final {
public:
    VelocityTracker() = default;
    explicit VelocityTracker(Axis mainAxis) : mainAxis_(mainAxis) {}
    ~VelocityTracker() = default;

    void Reset()
    {
        lastPosition_.Reset();
        velocity_.Reset();
        delta_.Reset();
        isFirstPoint_ = true;
        xAxis_.Reset();
        yAxis_.Reset();
    }

    void UpdateTouchPoint(const TouchEventInfo& event, bool end = false);
    
    void SetMainVelocity(double mainVelocity) 
    {
        mainVelocity_ = mainVelocity;
    }

    const TouchEventInfo& GetFirstTrackPoint() const
    {
        return firstTrackPoint_;
    }

    const TouchEventInfo& GetCurrentTrackPoint() const
    {
        return currentTrackPoint_;
    }

    const Offset& GetPosition() const
    {
        return lastPosition_;
    }

    const Offset& GetDelta() const
    {
        return delta_;
    }

    const Velocity& GetVelocity()
    {
        UpdateVelocity();
        return velocity_;
    }

    double GetMainAxisPos() const
    {
        switch (mainAxis_) {
            case Axis::FREE:
                return lastPosition_.GetDistance();
            case Axis::HORIZONTAL:
                return lastPosition_.GetX();
            case Axis::VERTICAL:
                return lastPosition_.GetY();
            default:
                return 0.0;
        }
    }

    double GetInterVal() const
    {   
        //两次down事件的间隔
        std::chrono::duration<double> inter = firstTrackPoint_.time- downTrackPoint_.time;
        auto interval = inter.count();
        return interval;
    }

    double GetMainVelocity() const
    {
        return mainVelocity_;
    }

    double GetMoveDistance() const
    {
        return (lastPosition_ - firstPosition_).GetDistance();
    }

    double GetDuration() const
    {
        return seconds;
    }

    double GetMainAxisDeltaPos() const
    {
        switch (mainAxis_) {
            case Axis::FREE:
                return delta_.GetDistance();
            case Axis::HORIZONTAL:
                return delta_.GetX();
            case Axis::VERTICAL:
                return delta_.GetY();
            default:
                return 0.0;
        }
    }

    double GetMainAxisVelocity()
    {
        UpdateVelocity();
        switch (mainAxis_) {
            case Axis::FREE:
                return velocity_.GetVelocityValue();
            case Axis::HORIZONTAL:
                return velocity_.GetVelocityX();
            case Axis::VERTICAL:
                return velocity_.GetVelocityY();
            default:
                return 0.0;
        }
    }

    const TouchEventInfo& GetLastTrackPoint() const
    {
        return lastTrackPoint_;
    }

private:
    void UpdateVelocity();

    Axis mainAxis_ { Axis::VERTICAL }; //FREE
    TouchEventInfo firstTrackPoint_;
    TouchEventInfo currentTrackPoint_;
    TouchEventInfo lastTrackPoint_;
    TouchEventInfo downTrackPoint_;
    Offset firstPosition_;
    Offset lastPosition_;
    Velocity velocity_;
    double mainVelocity_;
    Offset delta_;
    Offset offset_;
    double seconds;
    bool isFirstPoint_ = true;
    TimeStamp firstTimePoint_;
    TimeStamp lastTimePoint_;
    LeastSquareImpl xAxis_ { 3, 5 };
    LeastSquareImpl yAxis_ { 3, 5 };
    bool isVelocityDone_ = false;
};

} // namespace OHOS::uitest

#endif // GESTURES_VELOCITY_TRACKER_H
