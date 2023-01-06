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
#include "ability_manager_client.h"

using namespace std;
using namespace std::chrono;
namespace OHOS::uitest {
    enum TouchOpt : uint8_t {
        OP_CLICK, OP_LONG_CLICK, OP_DOUBLE_CLICK, OP_SWIPE, OP_DRAG, \
        OP_FLING, OP_HOME, OP_RECENT, OP_RETURN
    };
    std::string g_operationType[9] = { "click", "longClick", "doubleClick", "swipe", "drag", \
                                       "fling", "home", "recent", "back" };
    TouchOpt g_touchop = OP_CLICK;
    VelocityTracker g_velocityTracker;
    bool g_isClick = false;
    int g_clickEventCount = 0;
    constexpr int32_t NAVI_VERTI_THRE_V = 200;
    constexpr int32_t NAVI_THRE_D = 10;
    constexpr float MAX_THRESHOLD = 15.0;
    constexpr float FLING_THRESHOLD = 45.0;
    constexpr float DURATIOIN_THRESHOLD = 0.6;
    constexpr float INTERVAL_THRESHOLD = 0.2;
    constexpr int32_t MaxVelocity = 40000;
    bool g_isOpDect = false;
    std::string g_filePath;
    std::string g_defaultDir = "/data/local/tmp/layout";
    std::ofstream g_outFile;
    auto driver = UiDriver();
    Rect windowBounds = Rect(0, 0, 0, 0);
    DataWrapper g_dataWrapper;
    
    std::vector<std::string> GetForeAbility()
    {
        std::vector<std::string> elements;
        auto amcPtr = AAFwk::AbilityManagerClient::GetInstance();
        if (amcPtr == nullptr) {
            std::cout<<"AbilityManagerClient is nullptr"<<std::endl;
            return elements;
        }
        auto elementName = amcPtr->GetTopAbility();
        if (elementName.GetBundleName().empty()) {
            std::cout<<"GetTopAbility GetBundleName is nullptr"<<std::endl;
            return elements;
        }
        std::string bundleName = elementName.GetBundleName();
        std::string abilityName = elementName.GetAbilityName();
        elements.push_back(bundleName);
        elements.push_back(abilityName);
        return elements;
    }
    void PrintLine(const TouchEventInfo &downEvent, const TouchEventInfo &upEvent, const std::string &actionType)
    {
        std::cout << "Interval: " << g_velocityTracker.GetInterVal() << std::endl;
        std::cout << actionType << ": " ;
        if (actionType == "fling" || actionType == "swipe" || actionType == "drag") {
            if (downEvent.attributes.find("id")->second != "" || downEvent.attributes.find("text")->second != "") {
                std::cout << "from Widget(id: " << downEvent.attributes.find("id")->second << ", "
                            << "type: " << downEvent.attributes.find("type")->second << ", "
                            << "text: " << downEvent.attributes.find("text")->second << ") " << std::endl;
            } else {
                std::cout << "from Point(x:" << downEvent.x << ", y:" << downEvent.y
                            << ") to Point(x:" << upEvent.x << ", y:" << upEvent.y << ") " << std::endl;
            }
            if (actionType == "fling" || actionType == "swipe") {
                std::cout << "Off-hand speed:" << g_velocityTracker.GetMainVelocity() << ", "
                            << "Step length:" << g_velocityTracker.GetStepLength() << std::endl;
            }
        } else if (actionType == "click" || actionType == "longClick" || actionType == "doubleClick") {
            std::cout << actionType << ": " ;
            if (downEvent.attributes.find("id")->second != "" || downEvent.attributes.find("text")->second != "") {
                std::cout << " at Widget( id: " << downEvent.attributes.find("id")->second << ", "
                        << "text: " << downEvent.attributes.find("text")->second << ", "
                        << "type: " << downEvent.attributes.find("type")->second<< ") "<< std::endl;
            } else {
                std::cout <<" at Point(x:" << downEvent.x << ", y:" << downEvent.y << ") "<< std::endl;
            }
        } else {
            std::cout << std::endl;
        }
    }
    void CommonPrintLine(TouchEventInfo &downEvent, TouchEventInfo &upEvent, const std::string &actionType)
    {
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
    }
    void EventData::WriteEventData(const VelocityTracker &velocityTracker, const std::string &actionType)
    {
        v = velocityTracker;
        action = actionType;
        TouchEventInfo downEvent = v.GetFirstTrackPoint();
        TouchEventInfo upEvent = v.GetLastTrackPoint();
        if (g_recordMode == "point") {
            CommonPrintLine(downEvent, upEvent, action);
        } else {
            PrintLine(downEvent, upEvent, action);
        }
        if (g_outFile.is_open()) {
            g_outFile << actionType << ',';
            g_outFile << downEvent.x << ',';
            g_outFile << downEvent.y << ',';
            g_outFile << upEvent.x << ',';
            g_outFile << upEvent.y << ',';
            g_outFile << v.GetInterVal() << ',';
            g_outFile << v.GetStepLength() << ',';
            g_outFile << v.GetMainVelocity() << ',';
            g_outFile << MaxVelocity << ',';
            if (g_recordMode == "point") {
                g_outFile << ",,,,,,,,";
            } else {
                g_outFile << downEvent.attributes.find("id")->second << ',';
                g_outFile << downEvent.attributes.find("type")->second << ',';
                g_outFile << downEvent.attributes.find("text")->second << ',';
                if (actionType == "drag") {
                    g_outFile << upEvent.attributes.find("id")->second << ',';
                    g_outFile << upEvent.attributes.find("type")->second << ',';
                    g_outFile << upEvent.attributes.find("text")->second << ',';
                } else {
                    g_outFile << ",,,";
                }
                if (actionType == "click") {
                    std::vector<std::string> names = GetForeAbility();
                    g_outFile << names[ZERO] << ',';
                    g_outFile << names[ONE] << ',';
                } else {
                    g_outFile << ",,";
                }
            }
            g_outFile << std::endl;
            if (g_outFile.fail()) {
                std::cout<< " outFile failed. " <<std::endl;
            }
        } else {
            std::cout << "outFile is not opened!"<< std::endl;
        }
    }
    void EventData::ReadEventLine()
    {
        std::ifstream inFile(g_defaultDir + "/" + "record.csv");
        enum CaseTypes : uint8_t {
            OP_TYPE = 0, X_POSI, Y_POSI, X2_POSI, Y2_POSI, INTERVAL, LENGTH, VELO, \
            MAX_VEL, W_ID, W_TYPE, W_TEXT, W2_ID, W2_TYPE, W2_TEXT, BUNDLE, ABILITY
        };
        char buffer[100];
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
						<< std::stoi(caseInfo[VELO]) << ";"
						<< std::stoi(caseInfo[MAX_VEL]) << ";"
                        << caseInfo[W_ID] << ";"
                        << caseInfo[W_TYPE] << ";"
                        << caseInfo[W_TEXT] << ";"
                        << caseInfo[W2_ID] << ";"
                        << caseInfo[W2_TYPE] << ";"
                        << caseInfo[W2_TEXT] << ";"
                        << caseInfo[BUNDLE] << ";"
                        << caseInfo[ABILITY] << ";"
						<< std::endl;
            }
            int gTimeIndex = 1000;
            usleep(std::stoi(caseInfo[INTERVAL]) * gTimeIndex);
        }
    }
    void SetEventData(EventData &eventData)
    {
        eventData.WriteEventData(g_velocityTracker, g_operationType[g_touchop]);
    }
    void SaveEventData()
    {
        g_dataWrapper.ProcessData(SetEventData);
    }
    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
    {
        if (keyEvent->GetKeyCode() == KEYCODE_BACK) {
            g_touchop = OP_RETURN;
        }
        std::thread t(SaveEventData);
        t.join();
    }
    void InputEventCallback::HandleDownEvent(TouchEventInfo& event) const
    {
        if (g_recordMode != "point") {
            event.attributes = FindWidget(driver, event.x, event.y).GetAttrMap();
        }
        g_velocityTracker.UpdateTouchEvent(event, false);
    }
    void InputEventCallback::HandleMoveEvent(const TouchEventInfo& event) const
    {
        g_velocityTracker.UpdateTouchEvent(event, false);
        if (g_velocityTracker.GetDurationTime() >= DURATIOIN_THRESHOLD &&
           g_velocityTracker.GetMoveDistance() < MAX_THRESHOLD) {
            g_touchop = OP_LONG_CLICK;
            g_isOpDect = true;
            g_isClick = false;
        }
    }
    void InputEventCallback::HandleUpEvent(const TouchEventInfo& event) const
    {
        g_velocityTracker.UpdateTouchEvent(event, true);
        int moveDistance = g_velocityTracker.GetMoveDistance();
        if (!g_isOpDect) {
            double mainVelocity = g_velocityTracker.GetMainAxisVelocity();
            if (g_velocityTracker.GetDurationTime() >= DURATIOIN_THRESHOLD &&
                moveDistance < MAX_THRESHOLD) {
                g_touchop = OP_LONG_CLICK;
                g_isClick = false;
            } else if (moveDistance > MAX_THRESHOLD) {
                int startY = g_velocityTracker.GetFirstTrackPoint().y;
                Axis maxAxis_ = g_velocityTracker.GetMaxAxis();
                if (fabs(mainVelocity) > FLING_THRESHOLD) {
                    g_touchop = OP_FLING;
                    g_isClick = false;
                    if ((windowBounds.bottom_ - startY <= NAVI_THRE_D) && (maxAxis_ == Axis::VERTICAL) && \
						(moveDistance >= NAVI_VERTI_THRE_V)) {
                        g_touchop = OP_HOME;
                    }
                } else {
                    g_touchop = OP_SWIPE;
                    g_isClick = false;
                    if ((windowBounds.bottom_ - startY <= NAVI_THRE_D) && (maxAxis_ == Axis::VERTICAL) && \
						(moveDistance >= NAVI_VERTI_THRE_V)) {
                        g_touchop = OP_RECENT;
                    }
                }
                g_velocityTracker.UpdateStepLength();
            } else {
                // up-down>=0.6s => longClick
                if (g_isClick && g_velocityTracker.GetInterVal() < INTERVAL_THRESHOLD) {
                    // if lastOp is click && downTime-lastDownTime < 0.1 => double_click
                    g_touchop = OP_DOUBLE_CLICK;
                    g_isClick = false;
                } else {
                    g_touchop = OP_CLICK;
                    g_isClick = true;
                }
            }
        } else if (moveDistance >= MAX_THRESHOLD) {
            g_touchop = OP_DRAG;
            g_isClick = false;
        }
        if (g_touchop == OP_DRAG && g_recordMode != "point") {
            g_velocityTracker.GetLastTrackPoint().attributes = FindWidget(driver, event.x, event.y).GetAttrMap();
        }
        std::thread t(SaveEventData);
        t.join();
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
        TouchEventInfo touchEvent {};
        g_touchTime = GetCurrentMillisecond();
        TimeStamp t {std::chrono::duration_cast<TimeStamp::duration>( \
                     std::chrono::nanoseconds(pointerEvent->GetActionTime()*1000))};
        touchEvent.time = t;
        touchEvent.x = item.GetDisplayX();
        touchEvent.y = item.GetDisplayY();
        touchEvent.wx = item.GetWindowX();
        touchEvent.wy = item.GetWindowY();
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
    void RecordInitEnv(const std::string &modeOpt)
    {
        g_recordMode = modeOpt;
        g_velocityTracker.TrackResets();
        ApiCallErr err(NO_ERROR);
        auto selector = WidgetSelector();
        vector<std::unique_ptr<Widget>> rev;
        driver.FindWidgets(selector, rev, err, true);
        auto tree = driver.GetWidgetTree();
        windowBounds = tree->GetRootWidget()->GetBounds();
        std::cout<< "windowBounds : (" << windowBounds.left_ << ","
                << windowBounds.top_ << "," << windowBounds.right_ << ","
                << windowBounds.bottom_ << ")" << std::endl;
        std::vector<std::string> names = GetForeAbility();
        std::cout << "Current ForAbility :" << names[ZERO] << ", " << names[ONE] << std::endl;
        if (g_outFile.is_open()) {
            g_outFile << "windowBounds" << ',';
            g_outFile << windowBounds.left_ << ',';
            g_outFile << windowBounds.top_ << ',';
            g_outFile << windowBounds.right_ << ',';
            g_outFile << windowBounds.bottom_ << ',';
            g_outFile << "0,0,0,0,,,,,,,";
            g_outFile << names[ZERO] << ',';
            g_outFile << names[ONE] << ',' << std::endl;
        }
    }
} // namespace OHOS::uitest