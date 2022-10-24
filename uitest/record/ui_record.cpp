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
#include "ui_record.h"

using namespace std;
using namespace std::chrono;
namespace OHOS::uitest {

    std::string g_operationType[6] = {"click", "longClick", "doubleClick", "swipe", "drag", "fling"};
    enum GTouchop : uint8_t {CLICK_ = 0, LONG_CLICK_, DOUBLE_CLICK_, SWIPE_, DRAG_, FLING_};
    GTouchop g_touchop = CLICK_;
    bool g_isClick = false;
    int g_clickEventCount = 0;
    VelocityTracker velocityTracker_;
    bool isOpDect = false;

    void EventData::WriteEventData(std::ofstream &outFile, VelocityTracker velocityTracker, std::string actionType)
    {   
        if(outFile.is_open()){
            outFile << actionType << ',';
            outFile << velocityTracker.GetFirstTrackPoint().x << ',';
            outFile << velocityTracker.GetFirstTrackPoint().y << ',';
            outFile << velocityTracker.GetLastTrackPoint().x << ',';
            outFile << velocityTracker.GetLastTrackPoint().y << ',';
            outFile << velocityTracker.GetInterVal() << ',';
            outFile << STEP_LENGTH << ',';
            outFile << velocityTracker.GetMainVelocity() << std::endl;
            if(outFile.fail()){
                std::cout<< " outFile failed. " <<std::endl;
            }
        }else{
            std::cout << "outFile is not opened!"<< std::endl;
        }
    }
    void EventData::ReadEventLine(std::ifstream &inFile)
    {        
        enum caseTypes : uint8_t {Type = 0, XPosi, YPosi, X2Posi, Y2Posi, Interval, Length, Velo };
        char buffer[50];
        std::string type;
        int xPosi = -1;
        int yPosi = -1;
        int x2Posi = -1;
        int y2Posi = -1;
        int interval = -1;
        int length = -1;
        int velocity = -1;
        while (!inFile.eof()) {
            inFile >> buffer;
            std::string delim = ",";
            auto caseInfo = TestUtils::split(buffer, delim);
            type = caseInfo[Type];
            xPosi = std::stoi(caseInfo[XPosi]);
            yPosi = std::stoi(caseInfo[YPosi]);
            x2Posi = std::stoi(caseInfo[X2Posi]);
            y2Posi = std::stoi(caseInfo[Y2Posi]);
            interval = std::stoi(caseInfo[Interval]);
            length = std::stoi(caseInfo[Length]);
            velocity = std::stoi(caseInfo[Velo]);
            if (inFile.fail()) {
                break;
            } else {
                std::cout << type << ";"
                        << xPosi << ";"
                        << yPosi << ";"
                        << x2Posi << ";"
                        << y2Posi << ";"
                        << interval << ";"
                        << length << ";"
                        << velocity << ";" << std::endl;
            }
            int g_timeindex = 1000;
            usleep(interval * g_timeindex);
        }
    }
    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const 
    {
        std::cout << "keyCode" << keyEvent->GetKeyCode() << std::endl;
    }

    void InputEventCallback::HandleDownEvent(TouchEventInfo& event) const 
    {
        velocityTracker_.UpdateTouchPoint(event, false);
    }

    void InputEventCallback::HandleMoveEvent(TouchEventInfo& event) const
    {
        velocityTracker_.UpdateTouchPoint(event, false);
        if(velocityTracker_.GetDuration()>=DURATIOIN_THRESHOLD && velocityTracker_.GetMoveDistance()<MAX_THRESHOLD)
        {
            g_touchop = LONG_CLICK_;
            isOpDect = true;
            g_isClick = false;
        }
    }

    void InputEventCallback::HandleUpEvent(TouchEventInfo& event) const
    {
        velocityTracker_.UpdateTouchPoint(event, true);
        if(!isOpDect)
        {
            double mainVelocity = velocityTracker_.GetMainAxisVelocity();
            velocityTracker_.SetMainVelocity(mainVelocity);
            //移动距离超过15
            if(velocityTracker_.GetDuration()>=DURATIOIN_THRESHOLD && velocityTracker_.GetMoveDistance()<MAX_THRESHOLD) {
                g_touchop = LONG_CLICK_;
                g_isClick = false;
            } else if(velocityTracker_.GetMoveDistance()>MAX_THRESHOLD) {
                //抬手速度大于45 fling
                if(fabs(mainVelocity) > FLING_THRESHOLD) {
                    g_touchop = FLING_;
                    g_isClick = false;
                } else {
                    g_touchop = SWIPE_;
                    g_isClick = false;
                }
            } else {
                //up-down>=0.6s => longClick
                if(g_isClick && velocityTracker_.GetInterVal()<INTERVAL_THRESHOLD) {
                    //if lastOp is click && downTime-lastDownTime < 0.1 => double_click
                    g_touchop = DOUBLE_CLICK_;
                    g_isClick = false;
                } else {
                    g_touchop = CLICK_;
                    g_isClick = true;
                }
            }
        } else if(isOpDect && velocityTracker_.GetMoveDistance()>=MAX_THRESHOLD) {
            g_touchop = DRAG_;
            g_isClick = false;
        }
        if(!g_outFile.is_open()){
            g_outFile.open(filePath, std::ios_base::out | std::ios_base::trunc);
        }
        EventData::WriteEventData(g_outFile, velocityTracker_,g_operationType[g_touchop]);

        std::cout << " PointerEvent:" << g_operationType[g_touchop]
                        << " xPosi:" << velocityTracker_.GetFirstTrackPoint().x
                        << " yPosi:" << velocityTracker_.GetFirstTrackPoint().y
                        << " x2Posi:" << velocityTracker_.GetLastTrackPoint().x
                        << " y2Posi:" << velocityTracker_.GetLastTrackPoint().y
                        << " interval:" << velocityTracker_.GetInterVal() //ops interval
                        // << " duration:" << velocityTracker_.GetDuration() //op duration
                        << " velocity:" << velocityTracker_.GetMainVelocity() // up velocity
                        // << " distance:" << velocityTracker_.GetMoveDistance()
                        << std::endl;
        velocityTracker_.Reset();
        isOpDect = false;
    }

    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
    {
        MMI::PointerEvent::PointerItem item;
        bool result = pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item);
        if (!result) {
            std::cout << "GetPointerItem Fail" << std::endl;
        }
        TouchEventInfo touchEvent {};
        g_touchtime = GetMillisTime();
        touchEvent.time = std::chrono::high_resolution_clock::now();
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
        DIR *rootDir = nullptr;
        if ((rootDir = opendir(defaultDir.c_str())) == nullptr) {
            int ret = mkdir(defaultDir.c_str(), S_IROTH | S_IRWXU | S_IRWXG);
            if (ret != 0) {
                std::cerr << "failed to create dir: " << defaultDir << std::endl;
                return false;
            }
        }
        return true;
    }

    bool InitEventRecordFile(std::ofstream &outFile)
    {
        if (!InitReportFolder()) {
            return false;
        }
        filePath = defaultDir + "/" + "record.csv";
        outFile.open(filePath, std::ios_base::out | std::ios_base::trunc);
        if (!outFile) {
            std::cerr << "Failed to create csv file at:" << filePath << std::endl;
            return false;
        }
        std::cout << "The result will be written in csv file at location: " << filePath << std::endl;
        return true;
    }
    
} // namespace OHOS::uitest