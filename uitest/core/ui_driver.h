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

#ifndef UI_DRIVER_H
#define UI_DRIVER_H

#include <string>
#include "ui_controller.h"
#include "ui_model.h"
#include "widget_selector.h"
#include "widget_image.h"

namespace OHOS::uitest {

    class UiDriver : public ExternApi<TypeId::DRIVER> {
    public:
        /**Create UiDriver for the target device.*/
        explicit UiDriver(std::string_view device);

        ~UiDriver() {}

        DISALLOW_COPY_AND_ASSIGN(UiDriver);

        /**Perform given operation on the given widget.*/
        void PerformWidgetOperate(const WidgetImage &image, WidgetOp type, ApiCallErr &error);

        /**Trigger the given key action. */
        void TriggerKey(const KeyAction &action, ApiCallErr &error);

        /**Inject the given text to the given id-specified widget.*/
        void InputText(const WidgetImage &image, std::string_view text, ApiCallErr &error);

        /**Find widgets with the given selector. Results are arranged in the receiver in <b>DFS</b> order.
         * @returns the widget images.
         **/
        void FindWidgets(const WidgetSelector &select, std::vector<std::unique_ptr<WidgetImage>> &rev, ApiCallErr &err);

        /**Scroll on the given subject widget to find the target widget matching the selector.*/
        std::unique_ptr<WidgetImage> ScrollSearch(const WidgetImage &img, const WidgetSelector &selector,
                                                  ApiCallErr &err, int32_t deadZoneSize);

        /**Drag widget-A to widget-B.*/
        void DragWidgetToAnother(const WidgetImage &imgFrom, const WidgetImage &imgTo, ApiCallErr &err);

        /**Wait for the matching widget appear in the given timeout.*/
        std::unique_ptr<WidgetImage> WaitForWidget(const WidgetSelector &select, uint32_t maxMs, ApiCallErr &err);

        /**Update the attributes of the WidgetImage from current UI.*/
        void UpdateWidgetImage(WidgetImage &image, ApiCallErr &error);

        /**Perform generic-action on raw points.*/
        void PerformGenericClick(PointerOp type, const Point &point, ApiCallErr &err);

        /**Perform generic-swipe on raw points.*/
        void PerformGenericSwipe(PointerOp type, const Point &fromPoint, const Point &toPoint, ApiCallErr &err);

        /**Delay current thread for given duration.*/
        static void DelayMs(uint32_t ms);

        /**Take screen capture, save to given file path as PNG.*/
        void TakeScreenCap(std::string_view savePath, ApiCallErr &err);

        void WriteIntoParcel(nlohmann::json &data) const override;

        void ReadFromParcel(const nlohmann::json &data) override;

    private:
        /**Update UI controller and UI objects.*/
        void UpdateUi(bool updateUiTree, ApiCallErr &error);

        /**Retrieve widget represented by the given WidgetImage from updated UI.*/
        const Widget *RetrieveWidget(const WidgetImage &img, ApiCallErr &err, bool updateUi = true);

        /**Find scroll widget on current UI. <b>(without updating UI objects)</b>*/
        const Widget *FindScrollWidget(const WidgetImage &img) const;

        /**For multi-device UI operation, specifies the target device name.*/
        std::string deviceName_;
        /**The UI manipulation options.*/
        UiDriveOptions options_;
        // objects that are needed to be updated before each interaction and used in the interaction
        std::unique_ptr<WidgetTree> widgetTree_ = nullptr;
        const UiController *uiController_ = nullptr;
    };
}

#endif