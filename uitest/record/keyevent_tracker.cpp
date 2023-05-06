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
#include "keyevent_tracker.h"

namespace OHOS::uitest{

    const std::vector<int32_t> KeyeventTracker::COMBINATION_KET = {
        MMI::KeyEvent::KEYCODE_CTRL_LEFT,
        MMI::KeyEvent::KEYCODE_CTRL_RIGHT,
        MMI::KeyEvent::KEYCODE_ALT_LEFT,
        MMI::KeyEvent::KEYCODE_ALT_RIGHT,
        MMI::KeyEvent::KEYCODE_SHIFT_LEFT,
        MMI::KeyEvent::KEYCODE_SHIFT_RIGHT,
        MMI::KeyEvent::KEYCODE_META_LEFT,
        MMI::KeyEvent::KEYCODE_META_RIGHT,
        // MMI::KeyEvent::KEYCODE_FN,
        MMI::KeyEvent::KEYCODE_POWER
    };

    bool KeyeventTracker::AddDownKeyEvent(KeyEventInfo &info){
        //三键以上的同时按键无效
        if(infos_.size() >=MAX_COMBINATION_SIZE){
            std::cout << "More than three keys are invalid at the same time" << std::endl;
            return false;
        }
        // 该按键是否已down
        if (std::find(infos_.begin(), infos_.end(), info) != infos_.end()){
            // std::cout << "the key is already down" << std::endl;
            return false;
        }
        infos_.push_back(info);
        return true;
    }

    void KeyeventTracker::AddUpKeyEvent(KeyEventInfo &info){
        KeyCodeDone(info.GetKeyCode());
        if (infos_.size() == 0){
            isCombination = false;
        }
        return ;
    }

    KeyeventTracker KeyeventTracker::GetSnapshootKey(KeyEventInfo &info){
        KeyeventTracker snapshootKeyTracker;
        snapshootKeyTracker.infos_ =  infos_;
        snapshootKeyTracker.actionUpTime = info.GetActionTime();
        snapshootKeyTracker.isNeedRecord = true;
        return snapshootKeyTracker;
    }

    void KeyeventTracker::KeyCodeDone(int32_t keyCode){
        auto infoit = std::find_if(infos_.begin(), infos_.end(), [keyCode](const KeyEventInfo& info) {
            return info.GetKeyCode() == keyCode;
        });
        if(infoit != infos_.end()){
            infos_.erase(infoit);
            return;
        }
        std::cout << "keyCode:" << keyCode << " donot have down action" << std::endl;
    }

    // cout
    bool KeyeventTracker::WriteCombinationData(shared_ptr<mutex> &cout_lock){
        if (infos_.size()==0){
            // LOG_E("cout异常");
            return false;
        }
        buildEventItems();
        std::lock_guard<mutex> guard(*cout_lock);
        for (size_t i = 0; i < 6; i++){
            std::cout << eventItems[i] << ", ";
        }
        std::cout << eventItems[6] << std::endl;
        return true;
    }
    bool KeyeventTracker::WriteSingleData(KeyEventInfo &info,shared_ptr<mutex> &cout_lock){
        buildEventItems(info);
        std::lock_guard<mutex> guard(*cout_lock);
        for (size_t i = 0; i < 6; i++){
            std::cout << eventItems[i] << ", ";
        }
        std::cout << eventItems[6] << std::endl;
        return true;
    }

    // record.csv
    bool KeyeventTracker::WriteCombinationData(ofstream& outFile , shared_ptr<mutex> &csv_lock) {
        if (infos_.size()==0){
            // LOG_E("record.csv存储异常");
            return false;
        }
        buildEventItems();
        std::lock_guard<mutex> guard(*csv_lock);
        if (outFile.is_open()){
            for (size_t i = 0; i < 6; i++){
                outFile << eventItems[i] << ", ";
            }
            outFile << eventItems[6] << std::endl;
        }
        return true;
    }
    bool KeyeventTracker::WriteSingleData(KeyEventInfo &info,ofstream &outFile , shared_ptr<mutex> &csv_lock){
        buildEventItems(info);
        std::lock_guard<mutex> guard(*csv_lock);
        if (outFile.is_open()){
            for (size_t i = 0; i < 6; i++){
                outFile << eventItems[i] << ", ";
            }
            outFile << eventItems[6] << std::endl;
        }
        return true;
    }
    // daemon socket
    bool KeyeventTracker::WriteCombinationData(shared_ptr<queue<string>> &eventQueue,shared_ptr<mutex> &socket_lock){
        if (infos_.size()==0){
            // LOG_E("daemon socket异常");
            std::cout << "daemon socket ERROR" << std::endl;
            return false;
        }
        buildEventItems();
        auto data = nlohmann::json();
        data["ActionStartTime"] = eventItems[0];
        data["ActionDurationTime"] = eventItems[1];
        data["OP_TYPE"] =  eventItems[2];
        data["keyItemsCount"] = eventItems[3];
        data["KeyCode1"] = eventItems[4];
        data["KeyCode2"] = eventItems[5];
        data["KeyCode3"] = eventItems[6];
        std::lock_guard<mutex> guard(*socket_lock);
        std::string send = data.dump();
        std::cout << send << std::endl;
        eventQueue->push("AA" + send + "BB");
        return true;
    }
    bool KeyeventTracker::WriteSingleData(KeyEventInfo &info,shared_ptr<queue<string>> &eventQueue,shared_ptr<mutex> &socket_lock){
        buildEventItems(info);
        auto data = nlohmann::json();
        data["ActionStartTime"] = eventItems[0];
        data["ActionDurationTime"] = eventItems[1];
        data["OP_TYPE"] =  eventItems[2];
        data["keyItemsCount"] = eventItems[3];
        data["KeyCode1"] = eventItems[4];
        data["KeyCode2"] = eventItems[5];
        data["KeyCode3"] = eventItems[6];
        std::lock_guard<mutex> guard(*socket_lock);
        std::string send = data.dump();
        std::cout << send << std::endl;
        eventQueue->push("AA" + send + "BB");
        return true;
    }

    void KeyeventTracker::buildEventItems(KeyEventInfo &info){
        if (eventItems[0] != "-1"){
            return ;
        }
        actionStartTime = info.GetActionTime();
        eventItems[0] = std::to_string(actionStartTime);
        eventItems[1] = std::to_string(actionUpTime - actionStartTime);
        eventItems[2] = "key";
        eventItems[3] = std::to_string(infos_.size()+1);
        for (size_t i = 0; i < infos_.size() && i < MAX_COMBINATION_SIZE; i++){
            eventItems[4+i] = std::to_string(infos_[i].GetKeyCode());
        }
        eventItems[4+infos_.size()] = std::to_string(info.GetKeyCode());
    }

    void KeyeventTracker::buildEventItems(){
        if (eventItems[0] != "-1" || infos_.size() == 0){
            return ;
        }
        actionStartTime = infos_[infos_.size()-1].GetActionTime();
        eventItems[0] = std::to_string(actionStartTime);
        eventItems[1] = std::to_string(actionUpTime - actionStartTime);
        eventItems[2] = "key";
        eventItems[3] = std::to_string(infos_.size());
        for (size_t i = 0; i < infos_.size() && i < MAX_COMBINATION_SIZE; i++){
            eventItems[4+i] = std::to_string(infos_[i].GetKeyCode());
        }
    }

    void KeyeventTracker::printEventItems(){
        std::cout << "infos:" ;
        for (size_t i = 0; i < infos_.size() ; i++)
        {
           std::cout << std::to_string(infos_[i].GetKeyCode()) << ",";
        }
        std::cout << std::endl;
    }
}// namespace OHOS::uitest