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
#include "pointer_tracker.h"
#include "pointer_info.h"
#include "input_manager.h"

namespace OHOS::uitest {
    struct RecordOption {
        bool saveLayout = false;
        bool terminalCout = true;
        bool saveWidget = true;
    };
    class InputEventCallback : public MMI::IInputEventConsumer {
    public:
        void RecordInitEnv(const RecordOption &option);
        bool InitReportFolder();
        bool InitEventRecordFile();
        bool dealSpecialKey(std::shared_ptr<MMI::KeyEvent> keyEvent) const;
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void HandleDownEvent(TouchEventInfo& event) const;
        void HandleMoveEvent(const TouchEventInfo& event) const;
        void HandleUpEvent(const TouchEventInfo& event) const;
        void WriteDataAndFindWidgets(const TouchEventInfo& event) const;
        void WritePointerInfo() const;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
        void SubscribeMonitorInit();
        void KeyEventSubscribeTemplate(SubscribeKeyevent& subscribeKeyevent);
        void SubscribeMonitorCancel();
        void TimerReprintClickFunction();
        void TimerTouchCheckFunction();
        void FindWidgetsandWriteData();
        void DoAbcCallBack(nlohmann::json jsonData) const;
        void SetAbcCallBack(std::function<void(nlohmann::json)> handler)
        {
            abcCallBack = handler;
        }
        auto GetAbcCallBack()
        {
            return abcCallBack;
        }
        void WriteLayout(nlohmann::json layout) const;
        void OnInputEventUp(std::shared_ptr<MMI::KeyEvent> keyEvent, KeyEventInfo &info) const;
        void OnInputEventDown(std::shared_ptr<MMI::KeyEvent> keyEvent, KeyEventInfo &info) const;
        static constexpr int TIMEINTERVAL = 5000;
        static constexpr int KEY_DOWN_DURATION = 0;
        static const std::string DEFAULT_DIR;
        mutable volatile int touchTime = 0;
        mutable volatile bool isLastClick_ = false;
        mutable shared_ptr<mutex> cout_lock = make_shared<std::mutex>();
        mutable shared_ptr<mutex> csv_lock = make_shared<std::mutex>();
        mutable std::mutex g_clickMut;
        mutable std::condition_variable clickCon;
        mutable volatile bool findWidgetsAllow_ = false;
        mutable std::mutex widgetsMut;
        mutable std::condition_variable widgetsCon;
        bool stopFlag = false;
        std::mutex timerMut;
        std::condition_variable timerCon;
    private:
        shared_ptr<queue<std::string>> eventQueue_;
        RecordOption recordMode;
        std::string filePath;
        WidgetSelector selector = WidgetSelector();
        vector<std::unique_ptr<Widget>> rev;
        std::function<void(nlohmann::json)> abcCallBack = nullptr;
        mutable std::ofstream outFile;
        mutable UiDriver driver = UiDriver();
        mutable PointerTracker pointerTracker_;
        mutable KeyeventTracker keyeventTracker_;
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

    class EventData {
    public:
        void WriteEventData(const VelocityTracker &velocityTracker, const std::string &actionType) const;
        static void ReadEventLine();
    private:
        static std::string defaultDir;
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
    int32_t UiDriverRecordStart(RecordOption modeOpt);
    int32_t UiDriverRecordStart(std::function<void(nlohmann::json)> handler, std::string modeOpt);
    int32_t UiDriverRecordStartTemplate(RecordOption modeOpt);
    void UiDriverRecordStop();
} // namespace OHOS::uitest
#endif // UI_RECORD_H