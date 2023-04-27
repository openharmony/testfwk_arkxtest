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
#include "ui_record.h"
#include "ability_manager_client.h"


namespace OHOS::uitest {
    using namespace std;
    using namespace std::chrono;
    using nlohmann::json;

    const string UITEST_RECORD = "record";
    const string UITEST_DAEMON = "daemon";

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

    void InputEventCallback::SubscribeMonitorInit()
    {
        // 电源键按下订阅
        std::set<int32_t> preKeys;
        std::shared_ptr<MMI::KeyOption> keyOption = std::make_shared<MMI::KeyOption>();
        keyOption->SetPreKeys(preKeys);
        keyOption->SetFinalKey(MMI::KeyEvent::KEYCODE_POWER);
        keyOption->SetFinalKeyDown(true);
        keyOption->SetFinalKeyDownDuration(10);
        subscribeId1_ = MMI::InputManager::GetInstance()->SubscribeKeyEvent(keyOption,
            [this](std::shared_ptr<MMI::KeyEvent> keyEvent) {
            OnInputEvent(keyEvent);
        });

        // 电源键抬起订阅
        std::shared_ptr<MMI::KeyOption> keyOption2 = std::make_shared<MMI::KeyOption>();
        keyOption2->SetPreKeys(preKeys);
        keyOption2->SetFinalKey(MMI::KeyEvent::KEYCODE_POWER);
        keyOption2->SetFinalKeyDown(false);
        keyOption2->SetFinalKeyDownDuration(0);
        subscribeId2_ = MMI::InputManager::GetInstance()->SubscribeKeyEvent(keyOption2,
            [this](std::shared_ptr<MMI::KeyEvent> keyEvent) {
            OnInputEvent(keyEvent);
        });

        // 音量UP键按下订阅
        std::shared_ptr<MMI::KeyOption> keyOption3 = std::make_shared<MMI::KeyOption>();
        keyOption3->SetPreKeys(preKeys);
        keyOption3->SetFinalKey(MMI::KeyEvent::KEYCODE_VOLUME_UP);
        keyOption3->SetFinalKeyDown(true);
        keyOption3->SetFinalKeyDownDuration(0);
        subscribeId3_ = MMI::InputManager::GetInstance()->SubscribeKeyEvent(keyOption3,
            [this](std::shared_ptr<MMI::KeyEvent> keyEvent) {
            OnInputEvent(keyEvent);
        });

        // 音量DOWN键按下订阅
        std::shared_ptr<MMI::KeyOption> keyOption4 = std::make_shared<MMI::KeyOption>();
        keyOption4->SetPreKeys(preKeys);
        keyOption4->SetFinalKey(MMI::KeyEvent::KEYCODE_VOLUME_DOWN);
        keyOption4->SetFinalKeyDown(true);
        keyOption4->SetFinalKeyDownDuration(0);
        subscribeId4_ = MMI::InputManager::GetInstance()->SubscribeKeyEvent(keyOption4,
            [this](std::shared_ptr<MMI::KeyEvent> keyEvent) {
            OnInputEvent(keyEvent);
        });
    }
    // key取消订阅
    void InputEventCallback::SubscribeMonitorCancel()
    {
        MMI::InputManager* inputManager = MMI::InputManager::GetInstance();
        if (inputManager == nullptr) {
            return;
        }
        if (subscribeId1_ >= 0) {
            inputManager->UnsubscribeKeyEvent(subscribeId1_);
        }
        if (subscribeId2_ >= 0) {
            inputManager->UnsubscribeKeyEvent(subscribeId2_);
        }
        if (subscribeId3_ >= 0) {
            inputManager->UnsubscribeKeyEvent(subscribeId3_);
        }
        if (subscribeId4_ >= 0) {
            inputManager->UnsubscribeKeyEvent(subscribeId4_);
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
        if (g_useSocket) {
            auto data = nlohmann::json();
            data["OP_TYPE"] = actionType;
            data["X_POSI"] = std::to_string(downEvent.x);
            data["Y_POSI"] = std::to_string(downEvent.y);
            data["X2_POSI"] = std::to_string(upEvent.x);
            data["Y2_POSI"] = std::to_string(upEvent.y);
            data["W1_ID"] = downEvent.attributes.find("id")->second;
            data["W1_Type"] = downEvent.attributes.find("type")->second;
            data["W1_Text"] = downEvent.attributes.find("text")->second;
            data["W1_BOUNDS"] = downEvent.attributes.find("bounds")->second;
            data["W1_HIER"] = downEvent.attributes.find("hierarchy")->second;
            data["INTERVAL"] = std::to_string(velo.GetEventInterVal());
            data["LENGTH"] = std::to_string(velo.GetStepLength());
            data["VELO"] = std::to_string(velo.GetMainVelocity());
            data["MAX_VEL"] = std::to_string(MaxVelocity);
            if (actionType == "drag") {
                data["W2_ID"] = upEvent.attributes.find("id")->second;
                data["W2_Type"] = upEvent.attributes.find("type")->second;
                data["W2_Text"] = upEvent.attributes.find("text")->second;
                data["W2_BOUNDS"] = upEvent.attributes.find("bounds")->second;
                data["W2_HIER"] = upEvent.attributes.find("hierarchy")->second;
            } else {
                data["W2_ID"] = "";
                data["W2_Type"] = "";
                data["W2_Text"] = "";
                data["W2_BOUNDS"] = "";
                data["W2_HIER"] = "";
            }
            if (actionType == "click") {
                std::vector<std::string> names = GetForeAbility();
                data["BUNDLE"] = names[0];
                data["ABILITY"] = names[1];
            } else {
                data["BUNDLE"] = "";
                data["ABILITY"] = "";
            }
            std::lock_guard<mutex> guard(*g_socket_lock);
            g_eventQueue->push("AA" + data.dump() + "BB");
        } else {
            std::string eventItems[9]={actionType, std::to_string(downEvent.x), std::to_string(downEvent.y), std::to_string(upEvent.x), \
            std::to_string(upEvent.y), std::to_string(velo.GetEventInterVal()), std::to_string(velo.GetStepLength()), std::to_string(velo.GetMainVelocity()), \
            std::to_string(MaxVelocity)};
            std::string eventData;
            for (int i = 0; i < 9; i++) {
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
        // std :: cout << "@@@@@KeyCode:" << keyEvent->GetKeyCode() 
        //         << " KeyAction:"<< keyEvent->GetKeyAction() 
        //         << " KeyActionTime:"<< keyEvent->GetActionTime() <<std::endl;
        // g_keyeventTracker.printEventItems();
        if (keyEvent->GetKeyCode() == MMI::KeyEvent::KEYCODE_BACK || keyEvent->GetKeyCode() == MMI::KeyEvent::KEYCODE_HOME){
            if (keyEvent->GetKeyAction() != MMI::KeyEvent::KEY_ACTION_UP) {
                g_isSpecialclick = true;
                return;
            }
            if (keyEvent->GetKeyCode()== MMI::KeyEvent::KEYCODE_BACK ){
                g_touchop = OP_RETURN;
            }else if(keyEvent->GetKeyCode()== MMI::KeyEvent::KEYCODE_HOME){
                g_touchop = OP_HOME;
            }
            g_isSpecialclick = false;
            g_eventData.WriteEventData(g_velocityTracker, g_operationType[g_touchop]);
            return;
        }
        auto item = keyEvent->GetKeyItem(keyEvent->GetKeyCode());
        if (!item) {
            std::cout << "GetPointerItem Fail" << std::endl;
        }
        KeyEventInfo info;
        info.SetActionTime(keyEvent->GetActionTime());
        info.SetKeyCode(keyEvent->GetKeyCode());
        if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_DOWN) {
            g_keyeventTracker.AddDownKeyEvent(info);
        } else if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_CANCEL) {
            g_keyeventTracker.AddCancelKeyEvent(info);
        } else if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_UP) {
            //获取快照
            KeyeventTracker snapshootKeyTracker = g_keyeventTracker.AddUpKeyEvent(info);
            // cout打印 + record.csv保存
            if(snapshootKeyTracker.GetisNeedRecord() && !g_useSocket){
                snapshootKeyTracker.WriteData(g_cout_lock);
                snapshootKeyTracker.WriteData(g_outFile,g_csv_lock);
            }
            // daemon socket输出,编json
            if(snapshootKeyTracker.GetisNeedRecord() && g_useSocket){
                snapshootKeyTracker.WriteData(g_eventQueue,g_socket_lock);
            }
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
        if (g_touchop == OP_DRAG && g_recordMode != "point") {
            g_velocityTracker.GetLastTrackPoint().attributes = FindWidget(driver, event.x, event.y).GetAttrMap();
        } 
        g_isOpDect = false;
        // 补充打印上一次的click
        // if (g_isLastClick && g_touchop != OP_DOUBLE_CLICK){
        //     g_isLastClick = false;
        //     g_eventData.WriteEventData(snapshootVelocityTracker, g_operationType[OP_CLICK]);
        // }
        snapshootVelocityTracker = VelocityTracker(g_velocityTracker);
        g_velocityTracker.Resets();
        if (!g_isSpecialclick && g_touchop == OP_CLICK){
            g_isLastClick = true;
            g_velocityTracker.SetClickInterVal(snapshootVelocityTracker.GetInterVal());
            clickCon.notify_all();
            return;
        }
        // back or home
        if(g_isSpecialclick) {}
        // 非click,正常输出
        else if(g_touchop != OP_CLICK){
            g_isLastClick = false;
            g_eventData.WriteEventData(snapshootVelocityTracker, g_operationType[g_touchop]);
        }
        
        canFindWidgets = true;
        widgetsCon.notify_all();
    }

    void InputEventCallback::TimerReprintClickFunction (){
        while (true){
            std::unique_lock <std::mutex> clickLck(g_clickMut);
            while(!g_isLastClick)
            {
                clickCon.wait(clickLck);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(INTERVAL_THRESHOLD*1000)));
            // std::chrono::duration<double> downDuration = snapshootVelocityTracker.GetDownTrackPoint().time.time_since_epoch();
            // std::this_thread::sleep_until(
            //     std::chrono::system_clock::from_time_t(std::time_t(downDuration.count())) + 
            //     std::chrono::milliseconds((int)(INTERVAL_THRESHOLD*1000)));
            if (g_isLastClick){
                g_isLastClick = false;
                g_eventData.WriteEventData(g_velocityTracker, g_operationType[g_touchop]);

                canFindWidgets = true;
                widgetsCon.notify_all();
            }
        }
    }

    void InputEventCallback::TimerTouchCheckFunction(){
        while (true){
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEINTERVAL));
            int currentTime = GetCurrentMillisecond();
            int diff = currentTime - g_touchTime;
            if (diff >= TIMEINTERVAL) {
                std::cout << "No operation detected for 5 seconds, press ctrl + c to save this file?" << std::endl;
            }
        }
    }

    void InputEventCallback::FindWidgetsFunction(){
        while (true){
            std::unique_lock<std::mutex> widgetsLck(widgetsMut);

            std::cout << "@@@@@canFindWidgets in thread: " << canFindWidgets <<std::endl;
            while(!canFindWidgets){
                widgetsCon.wait(widgetsLck);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 确保界面已更新
            driver.FindWidgets(selector, rev, err, true);
            std::cout << "@@@@WIDGETS ERROR: " << err.message_ << std::endl;
            std::cout << "@@@@@@@@@@@@@@@@ widgets in main @@@@@@@@@@@@@@@@@@@" <<std::endl;
            canFindWidgets = false;
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
            
            std::cout <<"@@@@@@@@ touch down"<< std::endl;
            std::unique_lock<std::mutex> widgetsLck(widgetsMut);
            std::cout << "@@@@@canFindWidgets in main: " << canFindWidgets <<std::endl;
            while(canFindWidgets){
                widgetsCon.wait(widgetsLck);
            }

            HandleDownEvent(touchEvent);
        } else if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_MOVE) {
            HandleMoveEvent(touchEvent);
        } else if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_UP) {
            std::cout <<"@@@@@@@@ touch up"<< std::endl;
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
    void SetSocketUtils(const bool useSocket, const shared_ptr<mutex> lock, const shared_ptr<queue<string>> eventQueue) {
        g_useSocket = useSocket;
        g_socket_lock = lock;
        g_eventQueue = eventQueue;
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
    void RecordInitEnv(const std::string &modeOpt, const std::string opt)
    {
        g_recordMode = modeOpt;
        g_recordOpt = opt;
        g_velocityTracker.TrackResets();
        driver.FindWidgets(selector, rev, err, true);
        auto screenSize = driver.GetDisplaySize(err);
        windowBounds = Rect(0, screenSize.px_, 0,  screenSize.py_);
        if (opt == "record") {
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
    }
} // namespace OHOS::uitest