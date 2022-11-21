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

#ifndef SYSTEM_UI_CONTROLLER_H
#define SYSTEM_UI_CONTROLLER_H

#include "ui_controller.h"

namespace OHOS::uitest {
    /**The default UiController of ohos, which is effective for all apps.*/
    class SysUiController final : public UiController {
    public:
        explicit SysUiController(std::string_view name);

        ~SysUiController() override;

        void GetUiHierarchy(std::vector<std::pair<Window, nlohmann::json>>& out) override;

        bool WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutMs) const override;

        void InjectTouchEventSequence(const PointerMatrix &events) const override;

        void InjectKeyEventSequence(const std::vector<KeyEvent> &events) const override;

        void PutTextToClipboard(std::string_view text) const override;

        bool TakeScreenCap(std::string_view savePath, std::stringstream &errReceiver) const override;

        bool GetCharKeyCode(char ch, int32_t& code, int32_t& ctrlCode) const override;

        bool IsWorkable() const override;

        // setup method, connect to system ability AAMS
        bool ConnectToSysAbility();

        // teardown method, disconnect from system ability AAMS
        void DisConnectFromSysAbility();

        void SetDisplayRotation(DisplayRotation rotation) const override;

        DisplayRotation GetDisplayRotation() const override;

        void SetDisplayRotationEnabled(bool enabled) const override;

        Point GetDisplaySize() const override;

        Point GetDisplayDensity() const override;

        bool IsScreenOn() const override;

    private:
        bool connected_ = false;
    };
}

#endif