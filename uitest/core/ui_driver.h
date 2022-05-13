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

#include <string>
#include "ui_controller.h"
#include "ui_model.h"
#include "widget_selector.h"

namespace OHOS::uitest {
    class UiDriver : public BackendClass {
    public:
        UiDriver() {}

        ~UiDriver() override {}

        /**Perform given operation on the given widget.*/
        void OperateWidget(const Widget &widget, WidgetOp op, const UiOpArgs& opt, ApiCallErr &error);

        /**Trigger the given key action. */
        void TriggerKey(const KeyAction &key, const UiOpArgs& opt, ApiCallErr &error);

        /**Inject the given text to the given id-specified widget.*/
        void InputText(const Widget &widget, std::string_view text, const UiOpArgs& opt, ApiCallErr &error);

        /**Find widgets with the given selector. Results are arranged in the receiver in <b>DFS</b> order.
         * @returns the widget images.
         **/
        void FindWidgets(const WidgetSelector &select, std::vector<std::unique_ptr<Widget>> &rev, ApiCallErr &err);

        /**Scroll on the given subject widget to find the target widget matching the selector.*/
        std::unique_ptr<Widget> ScrollSearch(const Widget &widget, const WidgetSelector &selector,
                                                  const UiOpArgs& opt, ApiCallErr &err);

        /**Scroll widget to the end.*/
        void ScrollToEnd(const Widget &img, bool scrollUp, const UiOpArgs& opt, ApiCallErr &err);
		
        /**Drag widget-A to widget-B.*/
        void DragIntoWidget(const Widget &imgA, const Widget &imgB, const UiOpArgs& opt, ApiCallErr &err);

        /**Wait for the matching widget appear in the given timeout.*/
        std::unique_ptr<Widget> WaitForWidget(const WidgetSelector &select, const UiOpArgs& opt, ApiCallErr &err);

        /**Get the snapshot on current UI for the given widget.*/
        std::unique_ptr<Widget> GetWidgetSnapshot(Widget &image, ApiCallErr &error);

        /**Perform generic-action on raw points.*/
        void PerformClick(PointerOp op, const Point &point, const UiOpArgs& opt, ApiCallErr &err);

        /**Perform generic-swipe on raw points.*/
        void PerformSwipe(PointerOp op, const Point &p0, const Point &p1, const UiOpArgs& opt, ApiCallErr &err);

        /**Delay current thread for given duration.*/
        static void DelayMs(uint32_t ms);

        /**Take screen capture, save to given file path as PNG.*/
        void TakeScreenCap(std::string_view savePath, ApiCallErr &err);

        const FrontEndClassDef& GetFrontendClassDef() const override
        {
            return UI_DRIVER_DEF;
        }

    private:
        /**Update UI controller and UI objects.*/
        void UpdateUi(bool updateUiTree, ApiCallErr &error);

        /**Retrieve widget represented by the given widget from updated UI.*/
        const Widget *RetrieveWidget(const Widget &img, ApiCallErr &err, bool updateUi = true);

        /**Find scroll widget on current UI. <b>(without updating UI objects)</b>*/
        const Widget *FindScrollWidget(const Widget &img) const;
        // objects that are needed to be updated before each interaction and used in the interaction
        std::unique_ptr<WidgetTree> widgetTree_ = nullptr;
        const UiController *uiController_ = nullptr;
    };
}

#endif