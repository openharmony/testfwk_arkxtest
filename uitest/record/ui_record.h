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
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "least_square_impl.h"
#include "touch_event.h"
#include "offset.h"
#include "velocity.h"
#include "velocity_tracker.h"
#include "keyevent_tracker.h"
#include "ui_driver.h"
#include "ui_action.h"
#include "input_manager.h"
#include "i_input_event_consumer.h"
#include "pointer_event.h"
#include "widget_operator.h"
#include "window_operator.h"
#include "widget_selector.h"
#include "ui_model.h"
#include "find_widget.h"
#include "key_event.h"
#include "key_option.h"

namespace OHOS::uitest {
    enum TouchOpt : uint8_t {
        OP_CLICK, OP_LONG_CLICK, OP_DOUBLE_CLICK, OP_SWIPE, OP_DRAG, \
        OP_FLING, OP_HOME, OP_RECENT, OP_RETURN
    };


    static std::string g_operationType[9] = { "click", "longClick", "doubleClick", "swipe", "drag", \
                                       "fling", "home", "recent", "back" };

    static const std::map <int32_t, TouchOpt> SPECIAL_KET_MAP= {
        {MMI::KeyEvent::KEYCODE_BACK,TouchOpt::OP_RETURN},
        {MMI::KeyEvent::KEYCODE_HOME,TouchOpt::OP_HOME},
    };
    static int TIMEINTERVAL = 5000;
    static std::string g_recordMode = "";
    static std::string g_recordOpt = "";
    static shared_ptr<mutex> g_cout_lock = make_shared<std::mutex>();
    static shared_ptr<mutex> g_csv_lock = make_shared<std::mutex>();
    
    class InputEventCallback : public MMI::IInputEventConsumer {
        public:
            void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
            void HandleDownEvent(TouchEventInfo& event) const;
            void HandleMoveEvent(const TouchEventInfo& event) const;
            void HandleUpEvent(const TouchEventInfo& event) const;
            void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
            void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
            void SubscribeMonitorInit();
            void SubscribeTemplate(int32_t keyCode,bool isDown, int32_t subId_);
            void SubscribeMonitorCancel();
            void TimerReprintClickFunction ();
            void TimerTouchCheckFunction();
            void FindWidgetsFunction();
            static std::shared_ptr<InputEventCallback> GetPtr();

        private:
            shared_ptr<queue<std::string>> eventQueue_;
            shared_ptr<mutex> lock_;
            int32_t powerDownSubId_;
            int32_t powerUpSubId_;
            int32_t volumeUpDownId_;
            int32_t volumeDownDownId_;
            int32_t escDownId_;
            int32_t escUpId_;
            int32_t f1DownId_;
            int32_t f1UpId_;
            int32_t altLeftDownId_;
            int32_t altLeftUpId_;
            int32_t altRightDownId_;
            int32_t altRightUpId_;
            int32_t fnDownId_;
            int32_t fnUpId_;
        public:
            mutable volatile int g_touchTime = 0;
            mutable volatile bool g_isLastClick = false;
            mutable volatile bool g_isSpecialclick = false;
            mutable std::mutex g_clickMut;
            mutable std::condition_variable clickCon;
            mutable volatile bool findWidgetsAllow = false;
            mutable std::mutex widgetsMut;
            mutable std::condition_variable widgetsCon;
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

    bool InitEventRecordFile();

    void RecordInitEnv(const std::string &modeOpt, const std::string opt);

    class EventData {
    public:
        void WriteEventData(const VelocityTracker &velocityTracker, const std::string &actionType) const;
        static void ReadEventLine();
    };

    class DataWrapper {
    public:
        template<typename Function>
        void ProcessData(Function userFunc)
        {
            std::lock_guard<std::mutex> lock(mut);
            userFunc(data);
        }
    private:
        EventData data;
        UiDriver d;
        std::mutex mut;
    };
} // namespace OHOS::uitest
#endif // UI_RECORD_H