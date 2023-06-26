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
#include <atomic>
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

    static constexpr int TIME_INDEX = 1000;
    void TrackResets()
    {
        lastTrackPoint_.Resets();
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

    int GetStepLength()
    {
        UpdateStepLength();
        return stepLength;
    }

    const Point& GetPosition() const
    {
        return lastPosition_;
    }

    const Point& GetFirstPosition() const
    {
        return firstPosition_;
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

    void SetClickInterVal(double interVal)
    {
        clickInterVal = interVal;
    }

    TimeStamp GetLastTimePoint()
    {
        return lastTimePoint_;
    }

    int GetAxisSize()
    {
        return xAxis_.GetTVals().size();
    }

    double GetPreTime(int num)
    {
        return xAxis_.GetTVals().at(xAxis_.GetTVals().size()-num);
    }

    double GetPreX(int num)
    {
        return xAxis_.GetPVals().at(xAxis_.GetPVals().size()-num);
    }

    double GetPreY(int num)
    {
        return yAxis_.GetPVals().at(yAxis_.GetPVals().size()-num);
    }

private:
    void UpdateVelocity();
    Axis mainAxis_ { Axis::FREE };
    Axis maxAxis_  {Axis::VERTICAL };
    TouchEventInfo lastTrackPoint_;
    Point firstPosition_;
    Point lastPosition_;
    Offset totalDelta_;
    Velocity velocity_;
    double mainVelocity_ = 0.0;
    Offset delta_;
    bool isFirstPoint_ = true;
    int useToCount = 4;
    int stepLength = 0;
    int stepCount = 0;
    TimeStamp firstTimePoint_;
    TimeStamp lastTimePoint_;
    LeastSquareImpl xAxis_ { 3, 5 };
    LeastSquareImpl yAxis_ { 3, 5 };
    bool isVelocityDone_ = false;
    double clickInterVal = 0;
};
} // namespace OHOS::uitest
#endif // VELOCITY_TRACKER_H