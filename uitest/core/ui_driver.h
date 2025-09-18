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

#ifndef UI_DRIVER_H
#define UI_DRIVER_H

#include "ui_controller.h"
#include "ui_action.h"
#include "widget_selector.h"

namespace OHOS::uitest {
    struct WindowCacheModel {
        explicit WindowCacheModel(const Window &win) : window_(win), widgetIterator_(nullptr) {}
        Window window_;
        std::unique_ptr<ElementNodeIterator> widgetIterator_;
    };
    class UiDriver : public BackendClass {
    public:
        UiDriver() {}

        ~UiDriver() override {}

        /**Find widgets with the given selector. Results are arranged in the receiver in <b>DFS</b> order.
         * @returns the widget object.
         **/
        void FindWidgets(const WidgetSelector &select, vector<unique_ptr<Widget>> &rev,
            ApiCallErr &err, bool updateUi = true);

        /**Wait for the matching widget appear in the given timeout.*/
        std::unique_ptr<Widget> WaitForWidget(const WidgetSelector &select, const UiOpArgs &opt, ApiCallErr &err);
        /**Find window matching the given matcher.*/
        std::unique_ptr<Window> FindWindow(std::function<bool(const Window &)> matcher, ApiCallErr &err);

        /**Retrieve widget from updated UI.*/
        const Widget *RetrieveWidget(const Widget &widget, ApiCallErr &err, bool updateUi = true);

        /**Retrieve window from updated UI.*/
        const Window *RetrieveWindow(const Window &window, ApiCallErr &err);

        string GetHostApp(const Widget &widget);

        /**Trigger the given key action. */
        void TriggerKey(const KeyAction &key, const UiOpArgs &opt, ApiCallErr &error, int32_t displayId = -1);

        /**Perform the given touch action.*/
        void PerformTouch(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err);

        void PerformMouseAction(const MouseAction &touch, const UiOpArgs &opt, ApiCallErr &err);

        /**Delay current thread for given duration.*/
        static void DelayMs(uint32_t ms);

        /**Take screen capture, save to given file path as PNG.*/
        void TakeScreenCap(int32_t fd, ApiCallErr &err, Rect rect, int32_t displayId = 0);

        void DumpUiHierarchy(nlohmann::json &out, const DumpOption &option, ApiCallErr &error);

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return DRIVER_DEF;
        }

        void SetDisplayRotation(DisplayRotation rotation, ApiCallErr &error);

        DisplayRotation GetDisplayRotation(ApiCallErr &error, int32_t displayId = 0);

        void SetDisplayRotationEnabled(bool enabled, ApiCallErr &error);

        bool WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutSec, ApiCallErr &error);

        void WakeUpDisplay(ApiCallErr &error);

        Point GetDisplaySize(ApiCallErr &error, int32_t displayId = 0);

        Point GetDisplayDensity(ApiCallErr &error, int32_t displayId = 0);

        static void RegisterController(std::unique_ptr<UiController> controller);

        bool CheckStatus(bool isConnected, ApiCallErr &error);

        static void RegisterUiEventListener(std::shared_ptr<UiEventListener> listener);

        void InputText(string_view text, ApiCallErr &error, const UiOpArgs &opt, int32_t displayId = -1);

        void PerformTouchPadAction(const TouchPadAction &touch, const UiOpArgs &opt, ApiCallErr &error);

        void PerformPenTouch(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err);
        void PerformKnuckleAction(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err);

        void SetAamsWorkMode(const AamsWorkMode mode);

        bool IsWearable();

        bool IsAdjustWindowModeEnable();

        bool CheckDisplayExist(int32_t displayId);

        void CloseAamsEvent();

        void OpenAamsEvent();
        
        void ChangeWindowMode(int32_t windowId, WindowMode mode);

        bool IsKnuckleSnapshotEnable();

        bool IsKnuckleRecordEnable();

    private:
        bool TextToKeyEvents(string_view text, std::vector<KeyEvent> &events, ApiCallErr &error);
        // UI objects that are needed to be updated before each interaction and used in the interaction
        void UpdateUIWindows(ApiCallErr &error, int32_t targetDisplay = -1);
        void DumpWindowsInfo(const DumpOption &option, Rect &mergeBounds, nlohmann::json &childDom);
        static std::unique_ptr<UiController> uiController_;
        // CacheModel:
        std::map<int32_t, vector<WindowCacheModel>> displayToWindowCacheMap_;
        // unique widget object save
        std::vector<Widget> visitWidgets_;
        std::vector<int> targetWidgetsIndex_;
        AamsWorkMode mode_ = AamsWorkMode::NORMAL;
        bool eventObserverEnable_ = true;
    };
} // namespace OHOS::uitest

#endif