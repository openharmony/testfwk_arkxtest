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

#ifndef WIDGET_OPERATOR_H
#define WIDGET_OPERATOR_H

#include "ui_driver.h"

namespace OHOS::uitest {
    class WidgetOperator {
    public:
        WidgetOperator(UiDriver &driver, const Widget &widget, const UiOpArgs &options);

        /**Perform generic-click widget.*/
        void GenericClick(TouchOp op, ApiCallErr &error) const;

        /**Inject the given text to the widget.*/
        void InputText(std::string_view text, ApiCallErr &error) const;

        /**Scroll widget to the end (top or bottom).*/
        void ScrollToEnd(bool toTop, ApiCallErr &error) const;

        /**Drag widget to another one.*/
        void DragIntoWidget(const Widget &another, ApiCallErr &error) const;

        /**Pinch widget to the target scale*/
        void PinchWidget(float_t scale, ApiCallErr &error) const;

        /**Scroll on the widget to find the target widget matching the selector.*/
        std::unique_ptr<Widget> ScrollFindWidget(const WidgetSelector &selector, ApiCallErr &error) const;

    private:
        UiDriver &driver_;
        const Widget &widget_;
        const UiOpArgs &options_;
    };
} // namespace OHOS::uitest

#endif