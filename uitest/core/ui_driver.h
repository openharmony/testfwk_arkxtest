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
#include "ui_model.h"
#include "widget_selector.h"

namespace OHOS::uitest {
    class UiDriver : public BackendClass {
    public:
        UiDriver() {}

        ~UiDriver() override {}

        /**Find widgets with the given selector. Results are arranged in the receiver in <b>DFS</b> order.
         * @returns the widget object.
         **/
        void FindWidgets(const WidgetSelector &select, std::vector<std::unique_ptr<Widget>> &rev,
            ApiCallErr &err, bool updateUi = true);

        /**Wait for the matching widget appear in the given timeout.*/
        std::unique_ptr<Widget> WaitForWidget(const WidgetSelector &select, const UiOpArgs &opt, ApiCallErr &err);

        /**Find window matching the given matcher.*/
        std::unique_ptr<Window> FindWindow(std::function<bool(const Window &)> matcher, ApiCallErr &err);

        /**Retrieve widget from updated UI.*/
        const Widget *RetrieveWidget(const Widget &widget, ApiCallErr &err, bool updateUi = true);

        /**Retrieve window from updated UI.*/
        const Window *RetrieveWindow(const Window &window, ApiCallErr &err);

        /**Trigger the given key action. */
        void TriggerKey(const KeyAction &key, const UiOpArgs &opt, ApiCallErr &error);

        /**Perform the given touch action.*/
        void PerformTouch(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err);

        /**Delay current thread for given duration.*/
        static void DelayMs(uint32_t ms);

        /**Take screen capture, save to given file path as PNG.*/
        void TakeScreenCap(std::string_view savePath, ApiCallErr &err);

        void DumpUiHierarchy(nlohmann::json &out, ApiCallErr &error);

        /** Get current UI tree.*/
        const WidgetTree *GetWidgetTree() const;

        /** Get current UI controller.*/
        const UiController *GetUiController(ApiCallErr &error);

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return DRIVER_DEF;
        }

    private:
        /**Update UI controller and UI objects.*/
        void UpdateUi(bool updateUiTree, ApiCallErr &error);
        // UI objects that are needed to be updated before each interaction and used in the interaction
        UiController *uiController_ = nullptr;
        std::unique_ptr<WidgetTree> widgetTree_ = nullptr;
        std::vector<Window> windows_;
    };
} // namespace OHOS::uitest

#endif