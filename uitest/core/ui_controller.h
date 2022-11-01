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
    enum Priority : uint8_t {
        HIGH = 3, MEDIUM = 2, LOW = 1
    };

    enum DisplayRotation : uint32_t {
        ROTATION_0,
        ROTATION_90,
        ROTATION_180,
        ROTATION_270
    };

    class UiController;
    // Prototype of function that provides UiControllers, used to install controllers on demand.
    using UiControllerProvider = std::function<void(std::list<std::unique_ptr<UiController>> &)>;

    class UiController {
    public:
        explicit UiController(std::string_view name);

        virtual ~UiController() = default;

        virtual void GetUiHierarchy(std::vector<std::pair<Window, nlohmann::json>>& out) {};

        virtual bool WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutSec) const
        {
            return false;
        };

        virtual void InjectTouchEventSequence(const PointerMatrix& events) const {};

        virtual void InjectKeyEventSequence(const std::vector<KeyEvent>& events) const {};

        virtual void PutTextToClipboard(std::string_view text) const {};

        virtual bool TakeScreenCap(std::string_view savePath, std::stringstream &errReceiver) const
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

        std::string GetName() const
        {
            return name_;
        }

        void SetPriority(Priority val)
        {
            this->priority_ = val;
        }

        static void RegisterControllerProvider(UiControllerProvider func);

        static void RegisterController(std::unique_ptr<UiController> controller, Priority priority);

        static void RemoveController(std::string_view name);

        static void RemoveAllControllers();

        /**Install UiControllers with registered controllerProvider.*/
        static void InstallFromProvider();

        /**The the currently active UiController, returns null if none is available.*/
        static UiController *GetController();

    private:
        const std::string name_;
        Priority priority_ = Priority::MEDIUM;
        static std::mutex controllerAccessMutex_;
        static std::list<std::unique_ptr<UiController>> controllers_;
        static UiControllerProvider controllerProvider_;

        static bool Comparator(const std::unique_ptr<UiController> &c1, const std::unique_ptr<UiController> &c2);
    };
}

#endif