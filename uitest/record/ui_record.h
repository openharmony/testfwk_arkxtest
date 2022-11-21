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

#ifndef UI_RECORD_H
#define UI_RECORD_H
#include <fstream>
#include <regex>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <typeinfo>
#include <string>
#include <vector>
#include <cmath>
#include <thread>
#include "least_square_impl.h"
#include "touch_event.h"
#include "offset.h"
#include "velocity.h"
#include "velocity_tracker.h"
#include "ui_driver.h"
#include "input_manager.h"
#include "i_input_event_consumer.h"
#include "pointer_event.h"
#include "widget_operator.h"
#include "window_operator.h"
#include "widget_selector.h"

namespace OHOS::uitest {
    static int g_touchtime;
    static std::ofstream g_outFile;
    static std::string defaultDir = "/data/local/tmp/layout";
    static std::string filePath;
    constexpr int TIMEINTERVAL = 5000;
    constexpr double MAX_THRESHOLD = 15.0;
    constexpr double FLING_THRESHOLD = 45.0;
    constexpr double DURATIOIN_THRESHOLD = 0.6;
    constexpr double INTERVAL_THRESHOLD = 0.2;

    class InputEventCallback : public MMI::IInputEventConsumer {
    public:

        virtual void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        
        void HandleDownEvent(TouchEventInfo& event) const;
        void HandleMoveEvent(TouchEventInfo& event) const;
        void HandleUpEvent(TouchEventInfo& event) const;
        virtual void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        virtual void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override {}
        static std::shared_ptr<InputEventCallback> GetPtr();
    };

    class TestUtils {
    public:
        TestUtils() = default;
        virtual ~TestUtils() = default;
        static std::vector<std::string> split(const std::string &in, const std::string &delim)
        {
            std::regex reg(delim);
            std::vector<std::string> res = {
                std::sregex_token_iterator(in.begin(), in.end(), reg, -1), std::sregex_token_iterator()
            };
            return res;
        };
    };

    bool InitEventRecordFile(std::ofstream &outFile);

    static int64_t GetMillisTime()
    {
        auto timeNow = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow.time_since_epoch());
        return tmp.count();
    }

    class EventData {
    public:
        static void WriteEventData(std::ofstream &outFile, const VelocityTracker &velocityTracker, \
                                   const std::string &actionType);

        static void ReadEventLine(std::ifstream &inFile);
    };
    
    class Timer {
    public:
        Timer(): expired(true), tryToExpire(false)
        {}
        Timer(const Timer& timer)
        {
            expired = timer.expired.load();
            tryToExpire = timer.tryToExpire.load();
        }
        Timer& operator = (const Timer& timer);
        ~Timer()
        {
            Stop();
        }
        void Start(int interval, std::function<void()> task)
        {
            if (expired == false) {
                return;
            }
            expired = false;
            std::thread([this, interval, task]() {
                while (!tryToExpire) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEINTERVAL));
                    task();
                }

                {
                    std::unique_lock<std::mutex> lk(index, std::try_to_lock);
                    expired = true;
                    expiredCond.notify_one();
                }
            }).detach();
        }
        void Stop()
        {
            if (expired) {
                return;
            }

            if (tryToExpire) {
                return;
            }

            tryToExpire = true; // change this bool value to make timer while loop stop
            {
                std::unique_lock<std::mutex> lk(index, std::try_to_lock);
                expiredCond.wait(lk, [this] {return expired == true; });

                // Resets the timer
                if (expired == true) {
                    tryToExpire = false;
                }
            }
        }

    private:
        std::atomic<bool> expired; // timer stopped status
        std::atomic<bool> tryToExpire; // timer is in stop process
        std::mutex index;
        std::condition_variable expiredCond;
    };
} // namespace OHOS::uitest
#endif // UI_RECORD_H