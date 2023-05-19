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
#include <chrono>
#include <thread>
#include "ability_manager_client.h"
#include "ui_record.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace std::chrono;
    using nlohmann::json;

    const string UITEST_RECORD = "record";
    const string UITEST_DAEMON = "daemon";

    enum TouchOpt : uint8_t {
        OP_CLICK, OP_LONG_CLICK, OP_DOUBLE_CLICK, OP_SWIPE, OP_DRAG, \
        OP_FLING, OP_HOME, OP_RECENT, OP_RETURN
    };


    std::string g_operationType[9] = { "click", "longClick", "doubleClick", "swipe", "drag", \
                                       "fling", "home", "recent", "back" };

    const std::map <int32_t, TouchOpt> SPECIAL_KET_MAP = {
        {MMI::KeyEvent::KEYCODE_BACK, TouchOpt::OP_RETURN},
        {MMI::KeyEvent::KEYCODE_HOME, TouchOpt::OP_HOME},
    };

    int TIMEINTERVAL = 5000;
    std::string g_recordMode = "";;
    shared_ptr<mutex> g_cout_lock = make_shared<std::mutex>();
    shared_ptr<mutex> g_csv_lock = make_shared<std::mutex>();

    VelocityTracker snapshootVelocityTracker;
    TouchOpt g_touchop = OP_CLICK;
    VelocityTracker g_velocityTracker;
    KeyeventTracker g_keyeventTracker;
    EventData g_eventData;
    bool g_isClick = false;
    int g_clickEventCount = 0;
    bool g_isOpDect = false;
    std::string g_filePath;
    std::string g_defaultDir = "/data/local/tmp/layout";
    std::ofstream g_outFile;
    auto driver = UiDriver();
    Rect windowBounds = Rect(0, 0, 0, 0);
    DataWrapper g_dataWrapper;
    auto selector = WidgetSelector();
    vector<std::unique_ptr<Widget>> rev;
    ApiCallErr err(NO_ERROR);

    bool SpecialKeyMapExistKey(int32_t keyCode, TouchOpt &touchop)
    {
        auto iter = SPECIAL_KET_MAP.find(keyCode);
        if (iter!=SPECIAL_KET_MAP.end()) {
            touchop = iter->second;
            return true;
        }
        return false;
    }

    std::vector<std::string> GetForeAbility()
    {
        std::vector<std::string> elements;
        std::string bundleName, abilityName;
        auto amcPtr = AAFwk::AbilityManagerClient::GetInstance();
        if (amcPtr == nullptr) {
            std::cout<<"AbilityManagerClient is nullptr"<<std::endl;
            abilityName = "";
            bundleName = "";
        } else {
            auto elementName = amcPtr->GetTopAbility();
            if (elementName.GetBundleName().empty()) {
                std::cout<<"GetTopAbility GetBundleName is nullptr"<<std::endl;
                bundleName = "";
            } else {
                bundleName = elementName.GetBundleName();
            }
            if (elementName.GetAbilityName().empty()) {
                std::cout<<"GetTopAbility GetAbilityName is nullptr"<<std::endl;
                abilityName = "";
            } else {
                abilityName = elementName.GetAbilityName();
            }
        }
        elements.push_back(bundleName);
        elements.push_back(abilityName);
        return elements;
    }

    void InputEventCallback::SubscribeTemplate(int32_t keyCode, bool isDown, int32_t subId_)
    {
        std::set<int32_t> preKeys;
        std::shared_ptr<MMI::KeyOption> keyOption = std::make_shared<MMI::KeyOption>();
        keyOption->SetPreKeys(preKeys);
        keyOption->SetFinalKey(keyCode);
        keyOption->SetFinalKeyDown(isDown);
        keyOption->SetFinalKeyDownDuration(0);
        subId_ = MMI::InputManager::GetInstance()->SubscribeKeyEvent(keyOption,
            [this](std::shared_ptr<MMI::KeyEvent> keyEvent) {
            OnInputEvent(keyEvent);
        });
    }

    void InputEventCallback::SubscribeMonitorInit()
    {
        // 电源键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_POWER, true, powerDownSubId_);

        // 电源键抬起订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_POWER, false, powerUpSubId_);

        // 音量UP键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_VOLUME_UP, true, volumeUpDownId_);

        // 音量DOWN键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_VOLUME_DOWN, true, volumeDownDownId_);

        // esc键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_ESCAPE, true, escDownId_);

        // esc键抬起订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_ESCAPE, false, escUpId_);

        // f1键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_F1, true, f1DownId_);

        // f1键抬起订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_F1, false, f1UpId_);
        
        // alt-left键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_ALT_LEFT, true, altLeftDownId_);

        // alt-left键抬起订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_ALT_LEFT, false, altLeftUpId_);
                
        // alt-right键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_ALT_RIGHT, true, altRightDownId_);

        // alt-right键抬起订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_ALT_RIGHT, false, altRightUpId_);

        // fn键按下订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_FN, true, fnDownId_);

        // fn键抬起订阅
        SubscribeTemplate(MMI::KeyEvent::KEYCODE_FN, false, fnUpId_);
    }
    // key取消订阅
    void InputEventCallback::SubscribeMonitorCancel()
    {
        MMI::InputManager* inputManager = MMI::InputManager::GetInstance();
        if (inputManager == nullptr) {
            return;
        }
        if (powerDownSubId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(powerDownSubId_);
        }
        if (powerUpSubId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(powerUpSubId_);
        }
        if (volumeUpDownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(volumeUpDownId_);
        }
        if (volumeDownDownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(volumeDownDownId_);
        }
        if (escDownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(escDownId_);
        }
        if (escUpId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(escUpId_);
        }
        if (f1DownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(f1DownId_);
        }
        if (f1UpId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(f1UpId_);
        }
        if (altLeftDownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(altLeftDownId_);
        }
        if (altLeftUpId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(altLeftUpId_);
        }
        if (altRightDownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(altRightDownId_);
        }
        if (altRightUpId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(altRightUpId_);
        }
        if (fnDownId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(fnDownId_);
        }
        if (fnUpId_ >= 0) {
            inputManager->UnsubscribeKeyEvent(fnUpId_);
        }
    }

    void PrintLine(VelocityTracker &velo, const std::string &actionType)
    {
        TouchEventInfo downEvent = velo.GetFirstTrackPoint();
        TouchEventInfo upEvent = velo.GetLastTrackPoint();
        std::cout << "Interval: " << velo.GetEventInterVal() << std::endl;
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
                std::cout << "Off-hand speed:" << velo.GetMainVelocity() << ", "
                            << "Step length:" << velo.GetStepLength() << std::endl;
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
    void CommonPrintLine(VelocityTracker &velo)
    {
        std::cout << " PointerEvent:" << g_operationType[g_touchop]
                    << " X_posi:" << velo.GetFirstTrackPoint().x
                    << " Y_posi:" << velo.GetFirstTrackPoint().y
                    << " X2_posi:" << velo.GetLastTrackPoint().x
                    << " Y2_posi:" << velo.GetLastTrackPoint().y
                    << " Interval:" << velo.GetEventInterVal()
                    << " Step:" << velo.GetStepLength()
                    << " Velocity:" << velo.GetMainVelocity()
                    << " Max_Velocity:" << MaxVelocity
                    << std::endl;
    }
    void EventData::WriteEventData(const VelocityTracker &velocityTracker, const std::string &actionType) const
    {
        VelocityTracker velo = velocityTracker;
        TouchEventInfo downEvent = velo.GetFirstTrackPoint();
        TouchEventInfo upEvent = velo.GetLastTrackPoint();
        constexpr int ACTION_INFO_NUM = 9;
        std::string eventItems[ACTION_INFO_NUM] = {
            actionType, std::to_string(downEvent.x), std::to_string(downEvent.y), std::to_string(upEvent.x),
            std::to_string(upEvent.y), std::to_string(velo.GetEventInterVal()), std::to_string(velo.GetStepLength()),
            std::to_string(velo.GetMainVelocity()), std::to_string(MaxVelocity)};
        std::string eventData;
        for (int i = 0; i < ACTION_INFO_NUM; i++) {
            eventData += eventItems[i] + ",";
        }
        if (g_recordMode == "point") {
            eventData += ",,,,,,,,";
            CommonPrintLine(velo);
        } else {
            eventData += downEvent.attributes.find("id")->second + ",";
            eventData += downEvent.attributes.find("type")->second + ',';
            eventData += downEvent.attributes.find("text")->second + ',';
            if (actionType == "drag") {
                eventData += upEvent.attributes.find("id")->second + ',';
                eventData += upEvent.attributes.find("type")->second + ',';
                eventData += upEvent.attributes.find("text")->second + ',';
            } else {
                eventData += ",,,";
            }
            if (actionType == "click") {
                std::vector<std::string> names = GetForeAbility();
                eventData += names[ZERO] + ',';
                eventData += names[ONE] + ',';
            } else {
                eventData += ",,";
            }
            PrintLine(velo, actionType);
        }
        g_outFile << eventData << std::endl;
        if (g_outFile.fail()) {
            std::cout<< " outFile failed. " <<std::endl;
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

    // KEY_ACTION
    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
    {
        if (SpecialKeyMapExistKey(keyEvent->GetKeyCode(), g_touchop)) {
            if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_DOWN) {
                g_isSpecialclick = true;
                return;
            } else if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_UP) {
                g_isSpecialclick = false;
                g_eventData.WriteEventData(g_velocityTracker, g_operationType[g_touchop]);
                findWidgetsAllow = true;
                widgetsCon.notify_all();
                return;
            }
        }
        g_touchTime = GetCurrentMillisecond();
        auto item = keyEvent->GetKeyItem(keyEvent->GetKeyCode());
        if (!item) {
            std::cout << "GetPointerItem Fail" << std::endl;
        }
        KeyEventInfo info;
        info.SetActionTime(keyEvent->GetActionTime());
        info.SetKeyCode(keyEvent->GetKeyCode());
        if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_DOWN) {
            if (KeyeventTracker::isCombinationKey(info.GetKeyCode())) {
                g_keyeventTracker.SetNeedRecord(true);
                g_keyeventTracker.AddDownKeyEvent(info);
            } else {
                g_keyeventTracker.SetNeedRecord(false);
                KeyeventTracker snapshootKeyTracker = g_keyeventTracker.GetSnapshootKey(info);
                 // cout打印 + record.csv保存
                snapshootKeyTracker.WriteSingleData(info, g_cout_lock);
                snapshootKeyTracker.WriteSingleData(info, g_outFile, g_csv_lock);
            }
        } else if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_UP) {
            if (!KeyeventTracker::isCombinationKey(info.GetKeyCode())) {
                return;
            }
            if (g_keyeventTracker.IsNeedRecord()) {
                g_keyeventTracker.SetNeedRecord(false);
                KeyeventTracker snapshootKeyTracker = g_keyeventTracker.GetSnapshootKey(info);
                // cout打印 + record.csv保存
                snapshootKeyTracker.WriteCombinationData(g_cout_lock);
                snapshootKeyTracker.WriteCombinationData(g_outFile, g_csv_lock);
            }
            g_keyeventTracker.AddUpKeyEvent(info);
        }
    }

    // AXIS_ACTION
    void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const {
    }

    // POINTER_ACTION
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

    void InputEventCallback::WriteDataAndFindWidgets(const TouchEventInfo& event) const
    {
        if (g_touchop == OP_DRAG && g_recordMode != "point") {
            g_velocityTracker.GetLastTrackPoint().attributes = FindWidget(driver, event.x, event.y).GetAttrMap();
        }
        g_isOpDect = false;
        snapshootVelocityTracker = VelocityTracker(g_velocityTracker);
        g_velocityTracker.Resets();
        if (!g_isSpecialclick && g_touchop == OP_CLICK) {
            g_isLastClick = true;
            g_velocityTracker.SetClickInterVal(snapshootVelocityTracker.GetInterVal());
            clickCon.notify_all();
            return;
        }

        // 非click,非back or home正常输出
        if (g_touchop != OP_CLICK && !g_isSpecialclick) {
            g_isLastClick = false;
            g_eventData.WriteEventData(snapshootVelocityTracker, g_operationType[g_touchop]);
            findWidgetsAllow = true;
            widgetsCon.notify_all();
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
                if (g_isClick && g_velocityTracker.GetInterVal() < INTERVAL_THRESHOLD) {
                    // if lastOp is click && downTime-lastDownTime < 0.2 => double_click
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
        WriteDataAndFindWidgets(event);
    }

    void InputEventCallback::TimerReprintClickFunction ()
    {
        while (true) {
            std::unique_lock <std::mutex> clickLck(g_clickMut);
            while (!g_isLastClick) {
                clickCon.wait(clickLck);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(INTERVAL_THRESHOLD * gTimeIndex)));
            if (g_isLastClick) {
                g_isLastClick = false;
                g_eventData.WriteEventData(g_velocityTracker, g_operationType[g_touchop]);

                findWidgetsAllow = true;
                widgetsCon.notify_all();
            }
        }
    }

    void InputEventCallback::TimerTouchCheckFunction()
    {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEINTERVAL));
            int currentTime = GetCurrentMillisecond();
            int diff = currentTime - g_touchTime;
            if (diff >= TIMEINTERVAL) {
                std::cout << "No operation detected for 5 seconds, press ctrl + c to save this file?" << std::endl;
            }
        }
    }

    void InputEventCallback::FindWidgetsFunction()
    {
        while (true) {
            std::unique_lock<std::mutex> widgetsLck(widgetsMut);
            while (!findWidgetsAllow) {
                widgetsCon.wait(widgetsLck);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(gTimeIndex)); // 确保界面已更新
            driver.FindWidgets(selector, rev, err, true);
            findWidgetsAllow = false;
            widgetsCon.notify_all();
        }
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
            std::unique_lock<std::mutex> widgetsLck(widgetsMut);
            while (findWidgetsAllow) {
                widgetsCon.wait(widgetsLck);
            }
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
        driver.FindWidgets(selector, rev, err, true);
        auto screenSize = driver.GetDisplaySize(err);
        windowBounds = Rect(0, screenSize.px_, 0,  screenSize.py_);
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