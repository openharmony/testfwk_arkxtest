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
#include <chrono>
#include "keyevent_tracker.h"

namespace OHOS::uitest {
    const std::vector<int32_t> KeyeventTracker::COMBINATION_KET = {
        MMI::KeyEvent::KEYCODE_CTRL_LEFT,
        MMI::KeyEvent::KEYCODE_CTRL_RIGHT,
        MMI::KeyEvent::KEYCODE_ALT_LEFT,
        MMI::KeyEvent::KEYCODE_ALT_RIGHT,
        MMI::KeyEvent::KEYCODE_SHIFT_LEFT,
        MMI::KeyEvent::KEYCODE_SHIFT_RIGHT,
        MMI::KeyEvent::KEYCODE_POWER
    };

    const std::string KeyeventTracker::EVENT_TYPE = "key" ;

    bool KeyeventTracker::AddDownKeyEvent(KeyEventInfo &info)
    {
        // 该按键是否已down
        if (std::find(infos_.begin(), infos_.end(), info) != infos_.end()) {
            return false;
        }
        infos_.push_back(info);
        return true;
    }

    void KeyeventTracker::AddUpKeyEvent(KeyEventInfo &info)
    {
        KeyCodeDone(info.GetKeyCode());
        if (infos_.size() == 0) {
            isCombination = false;
        }
        return ;
    }

    KeyeventTracker KeyeventTracker::GetSnapshootKey(KeyEventInfo &info)
    {
        KeyeventTracker snapshootKeyTracker;
        snapshootKeyTracker.infos_ =  infos_;
        snapshootKeyTracker.actionUpTime = info.GetActionTime();
        snapshootKeyTracker.isNeedRecord = true;
        return snapshootKeyTracker;
    }

    void KeyeventTracker::KeyCodeDone(int32_t keyCode)
    {
        auto infoIt = std::find_if(infos_.begin(), infos_.end(), [keyCode](const KeyEventInfo& info) {
            return info.GetKeyCode() == keyCode;
        });
        if (infoIt != infos_.end()) {
            infos_.erase(infoIt);
            return;
        }
        LOG_E("keyCode:%{keyCode} did not received down event before the up event.", keyCode);
    }

    // cout
    std::string KeyeventTracker::WriteCombinationData(shared_ptr<mutex> &cout_lock)
    {
        std::stringstream sout;
        if (infos_.size()==0) {
            LOG_E("Failed to obtain the combination_key when cout keyEvent.");
            return "Failed to obtain the combination_key when cout keyEvent.";
        }
        buildEventItems();
        std::lock_guard<mutex> guard(*cout_lock);
        for (size_t i = 0; i < INFO_SIZE-1; i++) {
            sout << eventItems[i] << ", ";
        }
        sout << eventItems[INFO_SIZE-1];
        std::cout << sout.str() << std::endl;
        return sout.str();
    }
    std::string KeyeventTracker::WriteSingleData(KeyEventInfo &info, shared_ptr<mutex> &cout_lock)
    {
        std::stringstream sout;
        buildEventItems(info);
        std::lock_guard<mutex> guard(*cout_lock);
        for (size_t i = 0; i < INFO_SIZE-1; i++) {
            sout << eventItems[i] << ", ";
        }
        sout << eventItems[INFO_SIZE-1];
        std::cout << sout.str() << std::endl;
        return sout.str();
    }

    // record.csv
    nlohmann::json KeyeventTracker::WriteCombinationData(nlohmann::json& abcOut, ofstream& outFile, shared_ptr<mutex> &csv_lock)
    {        
        auto data = nlohmann::json();
        if (infos_.size()==0) {
            LOG_E("Failed to obtain the combination_key when save keyEvent into record.csv.");
            data["ERROR_INFO"] = "Failed to obtain the combination_key when save keyEvent into record.csv.";
            return data;
        }
        buildEventItems();
        data["ActionStartTime"] = eventItems[CaseTypes::ActionStartTime];
        data["ActionDurationTime"] = eventItems[CaseTypes::ActionDurationTime];
        data["EVENT_TYPE"] =  eventItems[CaseTypes::E_TYPE];
        data["keyItemsCount"] = eventItems[CaseTypes::keyItemsCount];
        data["KeyCode1"] = eventItems[CaseTypes::KeyCode1];
        data["KeyCode2"] = eventItems[CaseTypes::KeyCode2];
        data["KeyCode3"] = eventItems[CaseTypes::KeyCode3];
        std::lock_guard<mutex> guard(*csv_lock);
        if (outFile.is_open()) {
            outFile << data.dump() << std::endl;
        }
        abcOut.push_back(data);
        return data;
    }

    nlohmann::json KeyeventTracker::WriteSingleData(nlohmann::json& abcOut, KeyEventInfo &info, ofstream &outFile, shared_ptr<mutex> &csv_lock)
    {
        buildEventItems(info);
        auto data = nlohmann::json();
        data["ActionStartTime"] = eventItems[CaseTypes::ActionStartTime];
        data["ActionDurationTime"] = eventItems[CaseTypes::ActionDurationTime];
        data["EVENT_TYPE"] =  eventItems[CaseTypes::E_TYPE];
        data["keyItemsCount"] = eventItems[CaseTypes::keyItemsCount];
        data["KeyCode1"] = eventItems[CaseTypes::KeyCode1];
        data["KeyCode2"] = eventItems[CaseTypes::KeyCode2];
        data["KeyCode3"] = eventItems[CaseTypes::KeyCode3];
        std::lock_guard<mutex> guard(*csv_lock);
        if (outFile.is_open()) {
            outFile << data.dump() << std::endl;
        }
        abcOut.push_back(data);
        return data;
    }

    void KeyeventTracker::buildEventItems(KeyEventInfo &info)
    {
        if (eventItems[0] != "-1") {
            return ;
        }
        actionStartTime = infos_.size()==0?info.GetActionTime():infos_[0].GetActionTime();
        eventItems[CaseTypes::ActionStartTime] = std::to_string(actionStartTime);
        eventItems[CaseTypes::ActionDurationTime] = std::to_string(actionUpTime - actionStartTime);
        eventItems[CaseTypes::E_TYPE] = EVENT_TYPE;
        eventItems[CaseTypes::keyItemsCount] = std::to_string(infos_.size()+1);
        for (size_t i = 0; i < infos_.size() && i < MAX_COMBINATION_SIZE; i++) {
            eventItems[CaseTypes::KeyCode1+i] = std::to_string(infos_[i].GetKeyCode());
        }
        eventItems[CaseTypes::KeyCode1+infos_.size()] = std::to_string(info.GetKeyCode());
    }

    void KeyeventTracker::buildEventItems()
    {
        if (eventItems[0] != "-1" || infos_.size() == 0) {
            return ;
        }
        actionStartTime = infos_[0].GetActionTime();
        eventItems[CaseTypes::ActionStartTime] = std::to_string(actionStartTime);
        eventItems[CaseTypes::ActionDurationTime] = std::to_string(actionUpTime - actionStartTime);
        eventItems[CaseTypes::E_TYPE] = EVENT_TYPE;
        eventItems[CaseTypes::keyItemsCount] = std::to_string(infos_.size());
        for (size_t i = 0; i < infos_.size() && i < MAX_COMBINATION_SIZE; i++) {
            eventItems[CaseTypes::KeyCode1+i] = std::to_string(infos_[i].GetKeyCode());
        }
    }

    void KeyeventTracker::printEventItems()
    {
        std::cout << "infos:" ;
        for (size_t i = 0; i < infos_.size() ; i++) {
            std::cout << std::to_string(infos_[i].GetKeyCode()) << ",";
        }
        std::cout << std::endl;
    }
} // namespace OHOS::uitest