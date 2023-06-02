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

#ifndef POINTER_TRACKER_H
#define POINTER_TRACKER_H

#include <map>
#include <chrono>
#include <utility>
#include <functional>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include "find_widget.h"
#include "utils.h"
#include "ui_driver.h"
#include "velocity_tracker.h"
#include "pointer_info.h"

namespace OHOS::uitest {
    class FingerTracker{
    public:
        FingerTracker() = default;
        ~FingerTracker() = default;
        void HandleDownEvent(TouchEventInfo& touchEvent);
        void HandleMoveEvent(TouchEventInfo& touchEvent);
        void HandleUpEvent(TouchEventInfo& touchEvent);
        void BuileFingerInfo();
        double GetMoveDistance() const
        {
            return Offset(velocityTracker.GetFirstPosition(), velocityTracker.GetPosition()).GetDistance();
        }
        FingerInfo& GetFingerInfo(){
            return fingerInfo;
        }
        VelocityTracker& GetVelocityTracker()
        {
            return velocityTracker;
        }

    private:
        FingerInfo fingerInfo {}; // 输出封装
        VelocityTracker velocityTracker {};
    };

    // using PointerTypeJudgFunction = std::function<bool(TouchEventInfo&)>;
    // typedef bool (*PointerTypeJudgFunction)(TouchEventInfo&);
    class PointerTracker{
    public:
        PointerTracker() = default;
        ~PointerTracker() = default;
        
        void HandleDownEvent(TouchEventInfo& touchEvent);
        void HandleMoveEvent(TouchEventInfo& touchEvent);
        void HandleUpEvent(TouchEventInfo& touchEvent);
        
        void InitJudgeChain();
        bool ClickJudge(TouchEventInfo& touchEvent); // click(back)
        bool LongClickJudge(TouchEventInfo& touchEvent);
        bool DragJudge(TouchEventInfo& touchEvent);
        // bool PinchJudge(TouchEventInfo& touchEvent);
        bool SwipJudge(TouchEventInfo& touchEvent); // swip(recent)
        bool FlingJudge(TouchEventInfo& touchEvent); // fling(home)
        bool RecentJudge(TouchEventInfo& touchEvent);
        bool HomeJudge(TouchEventInfo& touchEvent);
        bool BackJudge(TouchEventInfo& touchEvent);

        // cout
        std::string WriteData(PointerInfo pointerInfo, shared_ptr<mutex> &cout_lock);
        // record.csv
        nlohmann::json WriteData(PointerInfo pointerInfo, ofstream &outFile, shared_ptr<mutex> &csv_lock);
        //clear
        void ClearFingerTrackersValues();
        void SetWindow(Rect window)
        {
            windowBounds = window;
        }

        bool IsNeedWrite()
        {
            return isNeedWrite;
        }

        void SetNeedWrite(bool needWirte)
        {
            isNeedWrite = needWirte;
        }

        PointerInfo GetLastClickInfo()
        {
            return lastClickInfo;
        }

        PointerInfo& GetSnapshootPointerInfo()
        {
            return snapshootPointerInfo;
        }

        void SetLastClickInTracker(bool isLastClick){
            isLastClickInTracker = isLastClick;
        }
        
        double GetInterVal() const
        {
            // 两次down事件的间隔
            std::chrono::duration<double> inter = lastClickInfo.GetFirstTrackPoint().GetDownTimeStamp()- firstTrackPoint_.GetDownTimeStamp();
            auto interval = inter.count();
            return interval;
        }

        PointerInfo BuilePointerInfo(); // 输出封装
    public:
        static constexpr float ERROR_POINTER = 1; // move后长时间没有Up说明异常
        static constexpr float INTERVAL_THRESHOLD = 0.2; // 双击间隔时间上限
        static constexpr float MAX_THRESHOLD = 15.0; // 长按位移上限
        static constexpr float DURATIOIN_THRESHOLD = 0.6; // 长按时间下限
        static constexpr float FLING_THRESHOLD = 45.0; // 快慢划离手速度界限
        static constexpr int32_t NAVI_VERTI_THRE_V = 200; // home/recent步长下限
        static constexpr int32_t NAVI_THRE_D = 10; // home/recent起始位置
    private:
        std::map <int64_t, FingerTracker*> fingerTrackers; //finger记录
        int maxFingerNum = 0;
        int currentFingerNum = 0;
        Rect windowBounds = Rect(0, 0, 0, 0);
        std::map<TouchOpt, std::function<bool(TouchEventInfo&)>> pointerTypeJudgMap_;
        std::vector<TouchOpt> pointerTypeJudgChain_;
        bool isUpStage = false;
        bool isNeedWrite = false;
        bool isLastClickInTracker = false;
        TouchEventInfo firstTrackPoint_;
        PointerInfo lastClickInfo; // for doubleclick
        PointerInfo snapshootPointerInfo;
        std::string recordMode = "";
        
    private:
        template<typename T, typename... Args>
        void RemoveTypeJudge(std::vector<T>& vec, Args&&... args)
        {
            auto remove_func = [&](const T& value) {
                return ((value == args) || ...);
            };
            vec.erase(std::remove_if(vec.begin(), vec.end(), remove_func), vec.end());
        }
    };
}
#endif // POINTER_TRACKER_H
