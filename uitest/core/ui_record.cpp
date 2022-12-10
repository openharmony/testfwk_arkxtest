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

#include "ui_record.h"

using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
    std::string g_operationType[6] = { "click", "longClick", "doubleClick", "swipe", "drag", "fling" };
    TouchOp g_touchop = CLICK;
    bool g_isClick = false;
    int g_clickEventCount = 0;
    VelocityTracker g_velocityTracker;
    bool g_isOpDect = false;
    bool g_isFirstOp = true;
    std::string g_filePath;
    std::string g_defaultDir = "/data/local/tmp/layout";
    std::ofstream g_outFile;
    double MAX_THRESHOLD = 15.0;
    double FLING_THRESHOLD = 45.0;
    double DURATIOIN_THRESHOLD = 0.6;
    double INTERVAL_THRESHOLD = 0.2;
    int MaxVelocity = 40000;

    void EventData::WriteEventData(std::ofstream &outFile, const VelocityTracker &velocityTracker, \
                                   const std::string &actionType)
    {
        if (outFile.is_open()) {
            outFile << actionType << ',';
            outFile << velocityTracker.GetFirstTrackPoint().x << ',';
            outFile << velocityTracker.GetFirstTrackPoint().y << ',';
            outFile << velocityTracker.GetLastTrackPoint().x << ',';
            outFile << velocityTracker.GetLastTrackPoint().y << ',';
            outFile << velocityTracker.GetInterVal() << ',';
            outFile << g_velocityTracker.GetStepLength() << ',';
            outFile << velocityTracker.GetMainVelocity() << std::endl;
            if (outFile.fail()) {
                std::cout<< " outFile failed. " <<std::endl;
            }
        } else {
            std::cout << "outFile is not opened!"<< std::endl;
        }
    }
    void EventData::ReadEventLine()
    {
        std::ifstream inFile(g_defaultDir + "/" + "record.csv");
        enum CaseTypes : uint8_t { OP_TYPE = 0, X_POSI, Y_POSI, X2_POSI, Y2_POSI, INTERVAL, LENGTH, VELO };
        char buffer[50];
        while (!inFile.eof()) {
            inFile >> buffer;
            std::string delim = ",";
            auto caseInfo = TestUtils::split(buffer, delim);
            if (inFile.fail()) {
                break;
            } else {
                std::cout << caseInfo[OP_TYPE] << ";"
                        << std::stoi(caseInfo[X_POSI]) << ";"
                        << std::stoi(caseInfo[Y_POSI]) << ";"
                        << std::stoi(caseInfo[X2_POSI]) << ";"
                        << std::stoi(caseInfo[Y2_POSI]) << ";"
                        << std::stoi(caseInfo[INTERVAL]) << ";"
                        << std::stoi(caseInfo[LENGTH]) << ";"
                        << std::stoi(caseInfo[VELO]) << ";" << std::endl;
            }
            int gTimeIndex = 1000;
            usleep(std::stoi(caseInfo[INTERVAL]) * gTimeIndex);
        }
    }
    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
    {
        std::cout << "keyCode" << keyEvent->GetKeyCode() << std::endl;
    }
    void InputEventCallback::HandleDownEvent(const TouchEventInfo& event) const
    {
        g_velocityTracker.UpdateTouchEvent(event, false);
    }
    void InputEventCallback::HandleMoveEvent(const TouchEventInfo& event) const
    {
        g_velocityTracker.UpdateTouchEvent(event, false);
        if (g_velocityTracker.GetDurationTime() >= DURATIOIN_THRESHOLD &&
           g_velocityTracker.GetMoveDistance() < MAX_THRESHOLD) {
            g_touchop = LONG_CLICK;
            g_isOpDect = true;
            g_isClick = false;
        }
    }
    void InputEventCallback::HandleUpEvent(const TouchEventInfo& event) const
    {
        g_velocityTracker.UpdateTouchEvent(event, true);
        if (!g_isOpDect) {
            double mainVelocity = g_velocityTracker.GetMainAxisVelocity();
            g_velocityTracker.SetMainVelocity(mainVelocity);
            // 移动距离超过15 => LONG_CLICK(中间结果)
            if (g_velocityTracker.GetDurationTime() >= DURATIOIN_THRESHOLD &&
                g_velocityTracker.GetMoveDistance() < MAX_THRESHOLD) {
                g_touchop = LONG_CLICK;
                g_isClick = false;
            } else if (g_velocityTracker.GetMoveDistance() > MAX_THRESHOLD) {
                // 抬手速度大于45 => FLING_
                if (fabs(mainVelocity) > FLING_THRESHOLD) {
                    g_touchop = FLING;
                    g_isClick = false;
                } else {
                    g_touchop = SWIPE;
                    g_isClick = false;
                }
            } else {
                // up-down>=0.6s => longClick
                if (g_isClick && g_velocityTracker.GetInterVal() < INTERVAL_THRESHOLD) {
                    // if lastOp is click && downTime-lastDownTime < 0.1 => double_click
                    g_touchop = DOUBLE_CLICK_P;
                    g_isClick = false;
                } else {
                    g_touchop = CLICK;
                    g_isClick = true;
                }
            }
        } else if (g_velocityTracker.GetMoveDistance() >= MAX_THRESHOLD) {
            g_touchop = DRAG;
            g_isClick = false;
        }
        if (!g_outFile.is_open()) {
            g_outFile.open(g_filePath, std::ios_base::out | std::ios_base::trunc);
        }
        EventData::WriteEventData(g_outFile, g_velocityTracker, g_operationType[g_touchop]);
        std::cout << " PointerEvent:" << g_operationType[g_touchop]
                        << " X_posi:" << g_velocityTracker.GetFirstTrackPoint().x
                        << " Y_posi:" << g_velocityTracker.GetFirstTrackPoint().y
                        << " X2_posi:" << g_velocityTracker.GetLastTrackPoint().x
                        << " Y2_posi:" << g_velocityTracker.GetLastTrackPoint().y
                        << " Interval:" << g_velocityTracker.GetInterVal()
                        << " Step:" << g_velocityTracker.GetStepLength()
                        << " Velocity:" << g_velocityTracker.GetMainVelocity()
                        << " Max_Velocity:" << MaxVelocity
                        << std::endl;
        g_velocityTracker.Resets();
        g_isOpDect = false;
    }
    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
    {
        MMI::PointerEvent::PointerItem item;
        bool result = pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item);
        if (!result) {
            std::cout << "GetPointerItem Fail" << std::endl;
        }
        if (g_isFirstOp) {
            g_velocityTracker.TrackResets();
            g_isFirstOp = false;
        }
        TouchEventInfo touchEvent {};
        g_touchTime = GetMillisTime();
        TimeStamp t {std::chrono::duration_cast<TimeStamp::duration>( \
                     std::chrono::nanoseconds(pointerEvent->GetActionTime()*1000))};
        touchEvent.time = t;
        touchEvent.x = item.GetDisplayX();
        touchEvent.y = item.GetDisplayY();
        if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_DOWN) {
            HandleDownEvent(touchEvent);
        } else if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_MOVE) {
            HandleMoveEvent(touchEvent);
        } else if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_UP) {
            HandleUpEvent(touchEvent);
        }
    }
    std::shared_ptr<InputEventCallback> InputEventCallback::GetPtr()
    {
        return std::make_shared<InputEventCallback>();
    }
    bool InitReportFolder()
    {
        if (opendir(g_defaultDir.c_str()) == nullptr) {
            int ret = mkdir(g_defaultDir.c_str(), S_IROTH | S_IRWXU | S_IRWXG);
            if (ret != 0) {
                std::cerr << "failed to create dir: " << g_defaultDir << std::endl;
                return false;
            }
        }
        return true;
    }
    bool InitEventRecordFile()
    {
        if (!InitReportFolder()) {
            return false;
        }
        g_filePath = g_defaultDir + "/" + "record.csv";
        g_outFile.open(g_filePath, std::ios_base::out | std::ios_base::trunc);
        if (!g_outFile) {
            std::cerr << "Failed to create csv file at:" << g_filePath << std::endl;
            return false;
        }
        std::cout << "The result will be written in csv file at location: " << g_filePath << std::endl;
        return true;
    }
} // namespace OHOS::uitest