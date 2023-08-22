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
#include "json.hpp"
#include "ui_action.h"

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

        virtual bool Initialize()
        {
            return true;
        };

        virtual void GetUiHierarchy(std::vector<std::pair<Window, nlohmann::json>> &out, bool getWidgetNodes,
            string targetApp = "") {};

        virtual bool WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutSec) const
        {
            return false;
        };

        virtual void InjectTouchEventSequence(const PointerMatrix& events) const {};

        virtual void InjectKeyEventSequence(const std::vector<KeyEvent>& events) const {};

        virtual void InjectMouseEventSequence(const vector<MouseEvent>& events) const {};

        virtual void PutTextToClipboard(std::string_view text) const {};

        virtual bool TakeScreenCap(int32_t fd, std::stringstream &errReceiver, Rect rect) const
        {
            return false;
        };

        virtual bool GetCharKeyCode(char ch, int32_t& code, int32_t& ctrlCode) const
        {
            return false;
        };

        virtual void SetDisplayRotation(DisplayRotation rotation) const {};

        virtual DisplayRotation GetDisplayRotation() const
        {
            return ROTATION_0;
        };

        virtual void SetDisplayRotationEnabled(bool enabled) const {};

        virtual Point GetDisplaySize() const
        {
            return Point(0, 0);
        };

        virtual Point GetDisplayDensity() const
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

        virtual void RegisterUiEventListener(std::shared_ptr<UiEventListener> listener) const {};
    };
}

#endif