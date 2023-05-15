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

#ifndef KEYEVENT_TRACKER_H
#define KEYEVENT_TRACKER_H
#include <regex>
#include <cmath>
#include <chrono>
#include <iostream>
#include <queue>
#include <fstream>
#include <nlohmann/json.hpp>
#include "utils.h"
#include "key_event.h"
#include "key_option.h"

namespace OHOS::uitest {
    using namespace std;

    class KeyEventInfo {
        public:
            KeyEventInfo() = default;
            ~KeyEventInfo() = default;

            void SetActionTime(int64_t time){
                time_ = time;
            }
            int64_t GetActionTime()const{
                return time_;
            }
            void SetKeyCode(int32_t keyCode){
                keyCode_ = keyCode;
            }
            int32_t GetKeyCode() const{
                return keyCode_;
            }
            bool operator ==(const KeyEventInfo &info) const{
                return keyCode_ == info.GetKeyCode();
            }
        private:
            int64_t time_;
            int32_t keyCode_;
    };
    class KeyeventTracker{
        public:
            static const std::vector<int32_t> COMBINATION_KET;
            static const int MAX_COMBINATION_SIZE = 3;
        public:
            KeyeventTracker() = default;
            ~KeyeventTracker() = default;

            bool GetisNeedRecord() const{
                return isNeedRecord;
            }

            void SetisNeedRecord(bool needRecord){
                isNeedRecord = needRecord;
            }

            bool GetisCombination() const{
                return isCombination;
            }

            void SetisCombination(bool combination){
                isCombination = combination;
            }

            // 判断是否为组合按键中的特殊按键
            static bool isCombinationKey(int32_t keyCode) {
                auto it = std::find(COMBINATION_KET.begin(), COMBINATION_KET.end(), keyCode);
                if (it != COMBINATION_KET.end()) {
                    return true;
                } 
                return false;
            };

            bool AddDownKeyEvent(KeyEventInfo &info);
            bool AddCancelKeyEvent(KeyEventInfo &info);
            void AddUpKeyEvent(KeyEventInfo &info);
            KeyeventTracker GetSnapshootKey(KeyEventInfo &info);
            // cout
            bool WriteCombinationData(shared_ptr<mutex> &cout_lock);
            bool WriteSingleData(KeyEventInfo &info,shared_ptr<mutex> &cout_lock);
            // record.csv
            bool WriteCombinationData(ofstream &outFile , shared_ptr<mutex> &csv_lock);
            bool WriteSingleData(KeyEventInfo &info,ofstream &outFile , shared_ptr<mutex> &csv_lock);
            // daemon socket
            bool WriteCombinationData(shared_ptr<queue<string>> &eventQueue,shared_ptr<mutex> &socket_lock);
            bool WriteSingleData(KeyEventInfo &info,shared_ptr<queue<string>> &eventQueue,shared_ptr<mutex> &socket_lock);
            void printEventItems();

        private:
            void KeyCodeDone(int32_t keyCode);
            void KeyCodeCancel(int32_t keyCode);
            void buildEventItems();
            void buildEventItems(KeyEventInfo &info);
        private:
            std::vector<KeyEventInfo> infos_;
            bool isNeedRecord = false;
            bool isCombination = false;
            int64_t actionStartTime = 0;
            int64_t actionUpTime = 0;
            // starttime/duration/eventtype/keycount/k1/k2/k3
            std::string eventItems[7] = {"-1"};
            
    };

}// namespace OHOS::uitest
#endif // KEYEVENT_TRACKER_H