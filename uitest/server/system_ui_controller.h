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
        SysUiController();

        ~SysUiController() override;

        bool Initialize(ApiCallErr &error) override;

        void GetUiWindows(std::map<int32_t, vector<Window>> &out, int32_t targetDisplay = -1) override;

        bool GetWidgetsInWindow(const Window &winInfo, unique_ptr<ElementNodeIterator> &elementIterator,
            AamsWorkMode mode) override;

        bool WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutMs) const override;

        void InjectTouchEventSequence(const PointerMatrix &events) const override;

        void InjectMouseEventSequence(const vector<MouseEvent> &events) const override;

        void InjectKeyEventSequence(const std::vector<KeyEvent> &events, int32_t displayId) const override;

        bool IsTouchPadExist() const override;

        void InjectTouchPadEventSequence(const vector<TouchPadEvent>& events) const override;

        void PutTextToClipboard(std::string_view text, ApiCallErr &error) const override;

        bool TakeScreenCap(int32_t fd, std::stringstream &errReceiver, int32_t displayId, Rect rect = {0, 0, 0, 0})
            const override;

        bool GetCharKeyCode(char ch, int32_t& code, int32_t& ctrlCode) const override;

        bool IsWorkable() const override;

        bool IsWearable() const override;

        bool IsAdjustWindowModeEnable() const override;

        // setup method, connect to system ability AAMS
        bool ConnectToSysAbility(ApiCallErr &error);

        // teardown method, disconnect from system ability AAMS
        void DisConnectFromSysAbility();

        void SetDisplayRotation(DisplayRotation rotation) const override;

        DisplayRotation GetDisplayRotation(int32_t displayId) const override;

        void SetDisplayRotationEnabled(bool enabled) const override;

        Point GetDisplaySize(int32_t displayId) const override;

        Point GetDisplayDensity(int32_t displayId) const override;

        bool IsScreenOn() const override;

        void RegisterUiEventListener(std::shared_ptr<UiEventListener> listener) const override;

        void GetHidumperInfo(std::string windowId, char **buf, size_t &len) override;

        bool CheckDisplayExist(int32_t displayId) const override;

        void CloseAamsEvent() const override;

        void OpenAamsEvent() const override;

        void ChangeWindowMode(int32_t windowId, WindowMode mode) const override;
    private:
        void  InjectMouseEvent(const MouseEvent &event) const;
        int32_t GetValidDisplayId(int32_t id) const;
        bool connected_ = false;
        std::mutex dumpMtx;
        mutable std::vector<int32_t> downKeys_;
    };
}

#endif