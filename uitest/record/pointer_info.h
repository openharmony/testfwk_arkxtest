/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef POINTER_INFO_H
#define POINTER_INFO_H
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include "ui_model.h"
#include "touch_event.h"
#include "velocity.h"
#include "offset.h"

namespace OHOS::uitest {
    enum TouchOpt : uint8_t {
        OP_CLICK = 1, OP_LONG_CLICK, OP_DOUBLE_CLICK, OP_SWIPE, OP_DRAG, OP_FLING, OP_PINCH,
        OP_HOME, OP_RECENT, OP_RETURN
    };

    class FingerInfo {
    public:
        static const int MAX_VELOCITY = 40000;
        
        nlohmann::json WriteData();
        std::string WriteWindowData(std::string actionType);

        void SetFirstTouchEventInfo(TouchEventInfo& first)
        {
            firstTouchEventInfo = first;
        }

        TouchEventInfo& GetFirstTouchEventInfo()
        {
            return firstTouchEventInfo;
        }

        void SetLastTouchEventInfo(TouchEventInfo& last)
        {
            lastTouchEventInfo = last;
        }

        TouchEventInfo& GetLastTouchEventInfo()
        {
            return lastTouchEventInfo;
        }

        void SetStepLength (int step)
        {
            stepLength = step;
        }

        int GetStepLength()
        {
            return stepLength;
        }

        void SetVelocity(double vel)
        {
            velocity = vel;
        }

        double GetVelocity()
        {
            return velocity;
        }

        void SetDirection(const Offset& dir)
        {
            direction = dir;
        }

        Offset& GetDirection()
        {
            return direction;
        }

    private:
        TouchEventInfo firstTouchEventInfo;
        TouchEventInfo lastTouchEventInfo;
        int stepLength = 0; // 步长
        double velocity; // 离手速度
        Offset direction; // 方向
    };

    class PointerInfo {
    public:
        static const std::string EVENT_TYPE;
        static std::map <int32_t, std::string> OP_TYPE;
        // json
        nlohmann::json WriteData();
        std::string WriteWindowData();

        void SetTouchOpt(TouchOpt opt)
        {
            touchOpt = opt;
        }

        TouchOpt& GetTouchOpt()
        {
            return touchOpt;
        }

        void SetFingerNumber(int8_t num)
        {
            fingerNumber = num;
        }

        int8_t GetFingerNumber()
        {
            return fingerNumber;
        }

        void AddFingerInfo(FingerInfo& fingerInfo)
        {
            fingers.push_back(fingerInfo);
        }

        std::vector<FingerInfo>& GetFingerInfoList()
        {
            return fingers;
        }

        void SetDuration(double dur)
        {
            duration = dur;
        }

        double GetDuration()
        {
            return duration;
        }

        void SetBundleName(std::string name)
        {
            bundleName = name;
        }

        std::string GetBundleName()
        {
            return bundleName;
        }

        void SetAbilityName(std::string name)
        {
            abilityName = name;
        }

        std::string GetAbilityName()
        {
            return abilityName;
        }

        void SetAvgDirection(Offset& direction)
        {
            avgDirection = direction;
        }

        Offset& GetAvgDirection()
        {
            return avgDirection;
        }

        void SetAvgStepLength(int stepLength)
        {
            avgStepLength = stepLength;
        }

        int GetAvgStepLength()
        {
            return avgStepLength;
        }

        void SetAvgVelocity(double velocity)
        {
            avgVelocity = velocity;
        }

        double GetAvgVelocity()
        {
            return avgVelocity;
        }

        void SetCenterSet(Point& center)
        {
            centerSet = center;
        }

        Point& GetCenterSet()
        {
            return centerSet;
        }

        void SetFirstTrackPoint(TouchEventInfo firstTrackPoint)
        {
            firstTrackPoint_ = firstTrackPoint;
        }

        TouchEventInfo GetFirstTrackPoint() const
        {
            return firstTrackPoint_;
        }
        
    private:
        TouchOpt touchOpt = OP_CLICK;
        int8_t fingerNumber = 0;
        std::vector<FingerInfo> fingers;
        double duration;
        std::string bundleName;
        std::string abilityName;
        Offset avgDirection;
        int avgStepLength = 0;
        double avgVelocity;
        Point centerSet;
        TouchEventInfo firstTrackPoint_;
    };
}
#endif // POINTER_INFO_H