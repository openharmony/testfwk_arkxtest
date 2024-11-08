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
#include "external_calls.h"
#include "pointer_tracker.h"
namespace OHOS::uitest {
    void FingerTracker::HandleDownEvent(TouchEventInfo& event)
    {
        velocityTracker.UpdateTouchEvent(event, false);
        fingerInfo.SetFirstTouchEventInfo(event);
        fingerInfo.SetLastTouchEventInfo(event);
    }

    void FingerTracker::HandleMoveEvent(TouchEventInfo& event)
    {
        velocityTracker.UpdateTouchEvent(event, false);
        fingerInfo.SetLastTouchEventInfo(event);
    }

    void FingerTracker::UpdatevelocityTracker(TouchEventInfo& event)
    {
        velocityTracker.UpdateTouchEvent(event, false);
    }
    void FingerTracker::HandleUpEvent(TouchEventInfo& event)
    {
        fingerInfo.SetLastTouchEventInfo(event);
    }

    void FingerTracker::BuildFingerInfo()
    {
        TouchEventInfo lastTouch = fingerInfo.GetLastTouchEventInfo();
        TouchEventInfo firstTouch = fingerInfo.GetFirstTouchEventInfo();
        fingerInfo.SetStepLength(velocityTracker.GetStepLength());
        fingerInfo.SetVelocity(velocityTracker.GetMainAxisVelocity());
        fingerInfo.SetDirection(Offset(lastTouch.x-firstTouch.x, lastTouch.y-firstTouch.y));
    }

    bool FingerTracker::IsRecentSpeedLimit(TouchEventInfo& touchEvent)
    {
        auto preEventTime = velocityTracker.GetPreTime(ONE);
        double deltaT = touchEvent.durationSeconds * VelocityTracker::TIME_INDEX -
                        preEventTime * VelocityTracker::TIME_INDEX;
        auto preX = velocityTracker.GetPreX(ONE);
        auto preY = velocityTracker.GetPreY(ONE);
        auto speedX = (touchEvent.wx - preX) / deltaT;
        auto speedY = (touchEvent.wy -  preY)/ deltaT;
        auto speed = sqrt(speedX * speedX + speedY * speedY);
        double curSpeed = touchEvent.wy > preY ? speed : -speed;
        if (fabs(curSpeed) < 1e-6) {
            return false;
        }
        auto acceleration = (curSpeed - preSpeed) / deltaT;
        preSpeed = curSpeed;
        return (acceleration > PointerTracker::RECENT_ACCELERAT && curSpeed > PointerTracker::RECENT_SPEED2) ||
               (curSpeed > PointerTracker::RECENT_SPEED1 && deltaT > PointerTracker::RECENT_TIME);
    }

    // POINTER_TRACKER
    void PointerTracker::HandleDownEvent(TouchEventInfo& event)
    {
        // 接受down事件时,若存在上次操作时间与本次down时间相差较大情况,说明上次操作接收情况异常,本次录制异常
        if (fingerTrackers.size() != 0) {
            TimeStamp thisTime = event.GetDownTimeStamp();
            bool flag = false;
            for (auto it = fingerTrackers.begin(); it != fingerTrackers.end(); it++) {
                TimeStamp lastTime = it->second->GetVelocityTracker().GetLastTimePoint();
                double  duration = (thisTime - lastTime).count();
                if (duration > ERROR_POINTER && it->second != nullptr) {
                    flag = true;
                    delete it->second;
                    it = fingerTrackers.erase(it);
                    LOG_E("获取回调信息存在异常,请重新录制");
                    std::cout << "获取回调信息存在异常,请重新录制" << std::endl;
                    break;
                }
            }
            if (flag) {
                Resets();
            }
        }
        if (fingerTrackers.size() == 0) {
            firstTrackPoint_ = event;
            InitJudgeChain();
        }
        FingerTracker* fTracker = new FingerTracker();
        if (fTracker == nullptr) {
            LOG_E("Failedf to new FingerTracker");
        } else {
            fTracker->HandleDownEvent(event);
            fingerTrackers.insert({event.downTime, fTracker});
            currentFingerNum++;
        }
    }

    void PointerTracker::HandleMoveEvent(TouchEventInfo& event)
    {
        if (fingerTrackers.count(event.downTime)) {
            if (pointerTypeJudgChain_.size() > 1) {
                auto judgeFunction = pointerTypeJudgMap_.find(pointerTypeJudgChain_[0])->second;
                judgeFunction(event);
            }
            FingerTracker* ftracker = fingerTrackers.find(event.downTime)->second;
            ftracker->HandleMoveEvent(event);
        }
    }

    void PointerTracker::HandleUpEvent(TouchEventInfo& event)
    {
        if (fingerTrackers.count(event.downTime)) {
            // 第一个抬起的手指,记录手指总数
            if (maxFingerNum == 0 && fingerTrackers.size() != 0) {
                maxFingerNum = fingerTrackers.size();
                isUpStage = true;
            }
            // 抬起判断
            bool flag = false;
            fingerTrackers.find(event.downTime)->second->UpdatevelocityTracker(event);
            while (pointerTypeJudgChain_.size() > 1 && !flag) {
                auto judgeFunction = pointerTypeJudgMap_.find(pointerTypeJudgChain_[0])->second;
                flag = judgeFunction(event);
            }
            fingerTrackers.find(event.downTime)->second->HandleUpEvent(event);
            fingerTrackers.find(event.downTime)->second->BuildFingerInfo();
            currentFingerNum--;
            // 最后一个抬起的手指,快照+复位
            if (currentFingerNum == 0) {
                snapshootPointerInfo = BuildPointerInfo();
                isNeedWrite = true;
                if (snapshootPointerInfo.GetTouchOpt() == OP_CLICK &&
                    isLastClickInTracker && GetInterVal() < INTERVAL_THRESHOLD) { // doubleClick
                    snapshootPointerInfo.SetTouchOpt(OP_DOUBLE_CLICK);
                    isLastClickInTracker = false;
                } else if (snapshootPointerInfo.GetTouchOpt() == OP_CLICK) { // click
                    lastClickInfo = snapshootPointerInfo;
                    isLastClickInTracker = true;
                }
                maxFingerNum = 0;
                isUpStage = false;
                ClearFingerTrackersValues();
            }
        }
    }

    void PointerTracker::HandleMoveEvent(TouchEventInfo& event, TouchOpt touchOpt)
    {
        pointerTypeJudgChain_.clear();
        pointerTypeJudgChain_ = {touchOpt};
        HandleMoveEvent(event);
    }

    void PointerTracker::HandleUpEvent(TouchEventInfo& event, TouchOpt touchOpt)
    {
        pointerTypeJudgChain_.clear();
        pointerTypeJudgChain_ = {touchOpt};
        HandleUpEvent(event);
    }

    void PointerTracker::ClearFingerTrackersValues()
    {
        for (auto it = fingerTrackers.begin(); it != fingerTrackers.end(); it++) {
            if (it->second != nullptr) {
                delete it->second;
            }
        }
        fingerTrackers.clear();
    }

    void PointerTracker::InitJudgeChain()
    {
        pointerTypeJudgChain_.clear();
        pointerTypeJudgChain_ = {
            OP_CLICK, OP_LONG_CLICK, OP_DRAG, OP_RECENT, OP_HOME, OP_RETURN, OP_SWIPE, OP_FLING
        };
        isStartRecent = false;
        if (pointerTypeJudgMap_.size() == 0) {
            pointerTypeJudgMap_.emplace(std::make_pair(OP_CLICK,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsClick, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_LONG_CLICK,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsLongClick, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_DRAG,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsDrag, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_RETURN,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsBack, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_SWIPE,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsSwip, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_RECENT,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsRecent, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_FLING,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsFling, this, placeholders::_1))));
            pointerTypeJudgMap_.emplace(std::make_pair(OP_HOME,
                function<bool(TouchEventInfo&)>(bind(&PointerTracker::IsHome, this, placeholders::_1))));
        }
    }

    bool PointerTracker::IsClick(TouchEventInfo& touchEvent) // click(back)
    {
        // 时间 > DURATIOIN_THRESHOLD && move > MAX_THRESHOLD
        int moveDistance = fingerTrackers.find(touchEvent.downTime)->second->GetMoveDistance();
        if (touchEvent.durationSeconds > DURATIOIN_THRESHOLD || moveDistance > MAX_THRESHOLD) {
            // 不满足点击操作,删除OP_CLICK
            RemoveTypeJudge(pointerTypeJudgChain_, OP_CLICK);
            return false;
        }
        return true;
    }

    bool PointerTracker::IsLongClick(TouchEventInfo& touchEvent)
    {
        // 先时间 > DURATIOIN_THRESHOLD,不满足则删除LONCLICK && DRAG
        if (touchEvent.durationSeconds < DURATIOIN_THRESHOLD) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_LONG_CLICK, OP_DRAG);
            return false;
        }
        // 初始时间长,排除滑动类操作
        RemoveTypeJudge(pointerTypeJudgChain_, OP_SWIPE, OP_FLING, OP_HOME, OP_RECENT);
        // 后 move > MAX_THRESHOLD ,若不满足却通过时间判断则为DRAG
        int moveDistance = fingerTrackers.find(touchEvent.downTime)->second->GetMoveDistance();
        if (moveDistance > MAX_THRESHOLD) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_LONG_CLICK);
            return false;
        }
        return true;
    }

    bool PointerTracker::IsDrag(TouchEventInfo& touchEvent)
    {
        // 其实LongClick已经判断过了
        // 时间 > DURATIOIN_THRESHOLD && move > MAX_THRESHOLD
        auto ftracker = fingerTrackers.find(touchEvent.downTime)->second;
        int moveDistance = ftracker->GetMoveDistance();
        if (touchEvent.durationSeconds < DURATIOIN_THRESHOLD || moveDistance < MAX_THRESHOLD) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_DRAG);
            return false;
        }
        return true;
    }

    // bool PinchJudge(TouchEventInfo& touchEvent)
    
    bool PointerTracker::IsRecent(TouchEventInfo& touchEvent)
    {
        // 起点位置判断
        auto ftracker = fingerTrackers.find(touchEvent.downTime)->second;
        TouchEventInfo startEvent = ftracker->GetFingerInfo().GetFirstTouchEventInfo();
        if (startEvent.y <= windowBounds.bottom_ * NAVI_THRE_D) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_RECENT, OP_HOME);
            return false;
        }
        if (isStartRecent) {
            return true;
        } else {
            // 滑动位移判断
            bool isDistance = (double)(windowBounds.bottom_ - touchEvent.y) / windowBounds.bottom_ >= RECENT_DISTANCE;
            // 速度判断
            bool isRecentSpeed = ftracker -> IsRecentSpeedLimit(touchEvent);
            if (isDistance && isRecentSpeed) {
                isStartRecent = true;
                return true;
            } else if (isUpStage) {
                RemoveTypeJudge(pointerTypeJudgChain_, OP_RECENT);
                return false;
            }
            return true;
        }
    }

    bool PointerTracker::IsHome(TouchEventInfo& touchEvent)
    {
        // 起点位置已判断
        // 滑动位移判断
        bool isDistance = (double)(windowBounds.bottom_ - touchEvent.y) / windowBounds.bottom_ >= HOME_DISTANCE;
        if (isDistance) {
            return true;
        } else if (isUpStage) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_HOME);
            return false;
        }
        return true;
    }

    bool PointerTracker::IsBack(TouchEventInfo& touchEvent)
    {
        // 滑动类只有起手才能判断
        if (!isUpStage) {
            return true;
        }
        auto ftracker = fingerTrackers.find(touchEvent.downTime)->second;
        TouchEventInfo startEvent = ftracker->GetFingerInfo().GetFirstTouchEventInfo();
        VelocityTracker vTracker = ftracker->GetVelocityTracker();
        if (startEvent.x - windowBounds.left_ <= NAVI_THRE_D &&
            ftracker->GetMoveDistance() >= NAVI_VERTI_THRE_V) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_SWIPE, OP_RECENT, OP_FLING, OP_HOME);
            return true;
        }
        RemoveTypeJudge(pointerTypeJudgChain_, OP_RETURN);
        return false;
    }

    bool PointerTracker::IsSwip(TouchEventInfo& touchEvent) // swip
    {
        // 滑动类只有起手才能判断
        if (!isUpStage) {
            return true;
        }
        auto ftracker = fingerTrackers.find(touchEvent.downTime)->second;
        double mainVelocity = ftracker->GetVelocityTracker().GetMainAxisVelocity();
        // 离手v < FLING_THRESHOLD
        if (mainVelocity >= FLING_THRESHOLD) {
            RemoveTypeJudge(pointerTypeJudgChain_, OP_SWIPE);
            return false;
        }
        return true;
    }

    bool PointerTracker::IsFling(TouchEventInfo& touchEvent) // fling
    {
        // SWIP判断过离手速度
        return true;
    }

    PointerInfo PointerTracker::BuildPointerInfo()
    {
        PointerInfo pointerInfo;
        pointerInfo.SetTouchOpt(pointerTypeJudgChain_[0]);
        pointerInfo.SetFingerNumber(maxFingerNum);
        pointerInfo.SetFirstTrackPoint(firstTrackPoint_);
        for (auto it = fingerTrackers.begin(); it != fingerTrackers.end(); it++) {
            pointerInfo.AddFingerInfo(it->second->GetFingerInfo());
        }
        auto fastTouch = pointerInfo.GetFingerInfoList()[0].GetLastTouchEventInfo();
        auto lastTouch = pointerInfo.GetFingerInfoList()[fingerTrackers.size()-1].GetLastTouchEventInfo();
        pointerInfo.SetDuration((lastTouch.GetActionTimeStamp() - fastTouch.GetDownTimeStamp()).count());
        if (pointerInfo.GetTouchOpt() == OP_CLICK || pointerInfo.GetTouchOpt() == OP_LONG_CLICK) {
            std::vector<std::string> names = GetFrontAbility();
            pointerInfo.SetBundleName(names[0]);
            pointerInfo.SetAbilityName(names[1]);
        }
        if (pointerInfo.GetTouchOpt() == OP_DRAG ||
            pointerInfo.GetTouchOpt() == OP_SWIPE ||
            pointerInfo.GetTouchOpt() == OP_FLING) {
            // 平均方向->向量相加
            Offset avgDirection;
            int addStepLength = 0;
            double addVelocity = 0.0;
            for (auto it = fingerTrackers.begin(); it != fingerTrackers.end(); it++) {
                avgDirection += it->second->GetFingerInfo().GetDirection();
                addStepLength += it->second->GetVelocityTracker().GetStepLength();
                addVelocity += it->second->GetFingerInfo().GetVelocity();
            }
            pointerInfo.SetAvgDirection(avgDirection);
            pointerInfo.SetAvgStepLength(addStepLength/fingerTrackers.size());
            double avgVelocity = addVelocity/fingerTrackers.size();
            pointerInfo.SetAvgVelocity(avgVelocity);
        }
        return pointerInfo;
    }

    // cout
    std::string PointerTracker::WriteData(PointerInfo pointerInfo, shared_ptr<mutex> &cout_lock)
    {
        auto out = pointerInfo.WriteWindowData();
        std::lock_guard<mutex> guard(*cout_lock);
        std::cout << out << std::endl;
        return out;
    }

    // record.csv
    nlohmann::json PointerTracker::WriteData(PointerInfo pointerInfo, ofstream& outFile, shared_ptr<mutex> &csv_lock)
    {
        auto out = pointerInfo.WriteData();
        std::lock_guard<mutex> guard(*csv_lock);
        if (outFile.is_open()) {
            outFile << out.dump() << std::endl;
        }
        return out;
    }
}