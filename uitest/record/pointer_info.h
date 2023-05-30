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

    static std::map <int32_t, std::string> OP_TYPE= {
        {TouchOpt::OP_CLICK,"click"},
        {TouchOpt::OP_LONG_CLICK,"longClick"},
        {TouchOpt::OP_DOUBLE_CLICK,"doubleClick"},
        {TouchOpt::OP_SWIPE,"swipe"},
        {TouchOpt::OP_DRAG,"drag"},
        {TouchOpt::OP_FLING,"fling"},
        {TouchOpt::OP_PINCH,"pinch"},
        {TouchOpt::OP_HOME,"home"},
        {TouchOpt::OP_RECENT,"recent"},
        {TouchOpt::OP_RETURN,"back"},
    };

    class FingerInfo{
    public:
        static const int MAX_VELOCITY = 40000;
        
        nlohmann::json WriteData();
        std::string WriteWindowData(std::string actionType);

        void SetFirstTouchEventInfo(TouchEventInfo& first){
            firstTouchEventInfo = first;
        }

        TouchEventInfo& GetFirstTouchEventInfo(){
            return firstTouchEventInfo;
        }

        void SetLastTouchEventInfo(TouchEventInfo& last){
            lastTouchEventInfo = last;
        }

        TouchEventInfo& GetLastTouchEventInfo(){
            return lastTouchEventInfo;
        }

        void SetStepLength (int stepLength_){
            stepLength = stepLength_;
        }

        int GetStepLength(){
            return stepLength;
        }

        void SetVelocity(double velocity_){
            velocity = velocity_;
        }

        double GetVelocity(){
            return velocity;
        }

        void SetDirection(const Offset& direction_){
            direction = direction_;
        }

        Offset& GetDirection(){
            return direction;
        }

    private:
        TouchEventInfo firstTouchEventInfo;
        TouchEventInfo lastTouchEventInfo;
        int stepLength = 0; // 步长
        double velocity; // 离手速度
        Offset direction; // 方向
    };

    class PointerInfo{
    public:
        static const std::string EVENT_TYPE;  
        // json
        nlohmann::json WriteData();
        std::string WriteWindowData();

        void SetTouchOpt(TouchOpt touchOpt_)
        {
            touchOpt = touchOpt_;
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

        void SetDuration(double duration_)
        {
            duration = duration_;
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
            avg_direction = direction;
        }

        Offset& GetAvgDirection()
        {
            return avg_direction;
        }

        void SetAvgStepLength(int stepLength)
        {
            avg_stepLength = stepLength;
        }

        int GetAvgStepLength()
        {
            return avg_stepLength;
        }

        void SetAvgVelocity(double velocity)
        {
            avg_velocity = velocity;
        }

        double GetAvgVelocity()
        {
            return avg_velocity;
        }

        void SetCenterSet(Point& center_)
        {
            center_set = center_;
        }

        Point& GetCenterSet()
        {
            return center_set;
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
        Offset avg_direction;
        int avg_stepLength = 0; 
        double avg_velocity;
        Point center_set;
        TouchEventInfo firstTrackPoint_;
    };
}
#endif // POINTER_INFO_H