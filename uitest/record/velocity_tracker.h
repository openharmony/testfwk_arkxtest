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

#ifndef VELOCITY_TRACKER_H
#define VELOCITY_TRACKER_H
#include <regex>
#include <cmath>
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

    void Resets()
    {
        lastPosition_.Resets();
        velocity_.Resets();
        delta_.Resets();
        isFirstPoint_ = true;
        xAxis_.Resets();
        yAxis_.Resets();
        totalDelta_.Resets();
    }
    void TrackResets()
    {
        downTrackPoint_.Resets();
        firstTrackPoint_.Resets();
    }

    void UpdateTouchEvent(const TouchEventInfo& event, bool end = false);
    
    void SetMainVelocity(double mainVelocity)
    {
        mainVelocity_ = mainVelocity;
    }

    int GetStepCount() const
    {
        return xAxis_.GetPVals().size();
    }

    void UpdateStepLength()
    {
        stepCount = GetStepCount();
        std::vector<double> xs = xAxis_.GetPVals();
        std::vector<double> ys = yAxis_.GetPVals();
        if (stepCount == 1) {
            return;
        }
        if (stepCount < useToCount) {
            useToCount = stepCount;
        }
        for (int i = 1; i < useToCount; i++) {
            totalDelta_ += Offset(xs.at(stepCount - i), ys.at(stepCount - i)) - \
                            Offset(xs.at(stepCount - i - 1), ys.at(stepCount - i - 1));
        }
        stepLength = (totalDelta_ / (useToCount - 1)).GetDistance();
    }

    int GetStepLength() const
    {
        return stepLength;
    }
    TouchEventInfo& GetFirstTrackPoint()
    {
        return firstTrackPoint_;
    }

    const Offset& GetPosition() const
    {
        return lastPosition_;
    }

    const Offset& GetDelta() const
    {
        return delta_;
    }

    const Velocity& GetVelo()
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
        // 两次down事件的间隔
        std::chrono::duration<double> inter = firstTrackPoint_.time- downTrackPoint_.time;
        auto interval = inter.count();
        return interval;
    }

    double GetMainVelocity() const
    {
        return mainVelocity_;
    }

    Axis GetMaxAxis()
    {
        UpdateVelocity();
        if (fabs(velocity_.GetVeloX()) > fabs(velocity_.GetVeloY())) {
            maxAxis_ = Axis::HORIZONTAL;
        } else {
            maxAxis_ = Axis::VERTICAL;
        }
        return maxAxis_;
    }

    double GetMoveDistance() const
    {
        return (lastPosition_ - firstPosition_).GetDistance();
    }

    double GetDurationTime() const
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
                mainVelocity_ =  velocity_.GetVeloValue();
                break;
            case Axis::HORIZONTAL:
                mainVelocity_ = velocity_.GetVeloX();
                break;
            case Axis::VERTICAL:
                mainVelocity_ = velocity_.GetVeloY();
                break;
            default:
                mainVelocity_ = 0.0;
        }
        return mainVelocity_;
    }

    TouchEventInfo& GetLastTrackPoint()
    {
        return lastTrackPoint_;
    }

private:
    void UpdateVelocity();
    Axis mainAxis_ { Axis::FREE };
    Axis maxAxis_  {Axis::VERTICAL };
    TouchEventInfo firstTrackPoint_;
    TouchEventInfo currentTrackPoint_;
    TouchEventInfo lastTrackPoint_;
    TouchEventInfo downTrackPoint_;
    Offset firstPosition_;
    Offset lastPosition_;
    Offset totalDelta_;
    Velocity velocity_;
    double mainVelocity_ = 0.0;
    Offset delta_;
    Offset offset_;
    double seconds = 0;
    bool isFirstPoint_ = true;
    int useToCount = 4;
    int stepLength = 0;
    int stepCount = 0;
    TimeStamp firstTimePoint_;
    TimeStamp lastTimePoint_;
    LeastSquareImpl xAxis_ { 3, 5 };
    LeastSquareImpl yAxis_ { 3, 5 };
    bool isVelocityDone_ = false;
};
} // namespace OHOS::uitest
#endif // VELOCITY_TRACKER_H