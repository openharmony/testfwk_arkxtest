/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <list>
#include <string>
#include <sstream>
#include <memory>
#include <mutex>
#include <functional>
#include "nlohmann/json.hpp"
#include "ui_action.h"
#include "select_strategy.h"

namespace OHOS::uitest {
    struct UiEventSourceInfo {
        std::string bundleName;
        std::string text;
        std::string type;
    };

    class UiEventListener {
    public:
        UiEventListener() = default;
        virtual ~UiEventListener() = default;
        virtual void OnEvent(const std::string &event, const UiEventSourceInfo &source) {};
    };

    class UiController {
    public:
        UiController() {};

        virtual ~UiController() = default;

        virtual bool Initialize(ApiCallErr &error)
        {
            return true;
        };

        virtual void GetUiWindows(std::map<int32_t, vector<Window>> &out, int32_t targetDisplay = -1){};

        virtual bool GetWidgetsInWindow(const Window &winInfo, unique_ptr<ElementNodeIterator> &elementIterator,
            AamsWorkMode mode)
        {
            return false;
        };

        virtual bool WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutSec) const
        {
            return false;
        };

        virtual void InjectTouchEventSequence(const PointerMatrix& events) const {};

        virtual void InjectKeyEventSequence(const std::vector<KeyEvent>& events, int32_t displayId) const {};

        virtual void InjectMouseEventSequence(const vector<MouseEvent>& events) const {};

        virtual bool IsTouchPadExist() const
        {
            return false;
        };

        virtual void InjectTouchPadEventSequence(const vector<TouchPadEvent>& events) const {};

        virtual void PutTextToClipboard(std::string_view text, ApiCallErr &error) const {};

        virtual bool TakeScreenCap(int32_t fd, std::stringstream &errReceiver, int32_t displayId, Rect rect) const
        {
            return false;
        };

        virtual bool GetCharKeyCode(char ch, int32_t& code, int32_t& ctrlCode) const
        {
            return false;
        };

        virtual void SetDisplayRotation(DisplayRotation rotation) const {};

        virtual DisplayRotation GetDisplayRotation(int32_t displayId) const
        {
            return ROTATION_0;
        };

        virtual void SetDisplayRotationEnabled(bool enabled) const {};

        virtual Point GetDisplaySize(int32_t displayId) const
        {
            return Point(0, 0);
        };

        virtual Point GetDisplayDensity(int32_t displayId) const
        {
            return Point(0, 0);
        };

        virtual bool IsScreenOn() const
        {
            return true;
        };

        /**
         * Tells if this controller is effective for current UI.
         * */
        virtual bool IsWorkable() const = 0;

        virtual bool IsWearable() const = 0;

        virtual bool IsAdjustWindowModeEnable() const = 0;

        virtual void RegisterUiEventListener(std::shared_ptr<UiEventListener> listener) const {};

        virtual void GetHidumperInfo(std::string windowId, char **buf, size_t &len) {};

        virtual bool CheckDisplayExist(int32_t displayId) const
        {
            return true;
        };

        virtual void ChangeWindowMode(int32_t windowId, WindowMode mode) const {};
    };
}

#endif
