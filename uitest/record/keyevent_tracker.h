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
            KeyeventTracker() = default;
            ~KeyeventTracker() = default;

            bool GetisDownStage() const{
                return isDownStage;
            }

            bool GetisNeedRecord() const{
                return isNeedRecord;
            }

            bool AddDownKeyEvent(KeyEventInfo &info);
            bool AddCancelKeyEvent(KeyEventInfo &info);
            KeyeventTracker AddUpKeyEvent(KeyEventInfo &info);
            // cout
            bool WriteData(shared_ptr<mutex> &cout_lock);
            // record.csv
            bool WriteData(ofstream &outFile , shared_ptr<mutex> &csv_lock);
            // daemon socket
            bool WriteData(shared_ptr<queue<string>> &eventQueue,shared_ptr<mutex> &socket_lock);
            void printEventItems();

        private:
            void KeyCodeDone(int32_t keyCode);
            void KeyCodeCancel(int32_t keyCode);
            void buildEventItems();
        private:
            std::vector<KeyEventInfo> infos_;
            std::vector<KeyEventInfo> cancelInfos_;
            bool isDownStage = false;  
            bool isNeedRecord = false;
            int64_t actionStartTime = 0;
            int64_t actionUpTime = 0;
            // starttime/duration/eventtype/keycount/k1/k2/k3
            std::string eventItems[7] = {"-1"};
            
    };

}// namespace OHOS::uitest
#endif // KEYEVENT_TRACKER_H