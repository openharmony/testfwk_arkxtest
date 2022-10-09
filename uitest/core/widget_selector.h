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

#ifndef WIDGET_SELECTOR_H
#define WIDGET_SELECTOR_H

#include "frontend_api_handler.h"
#include "widget_matcher.h"

namespace OHOS::uitest {
    /**
     * Selector that searches Widgets on the given WidgetTree according to the matchers
     * of the target widget and its adjacent (front/rear/ancestor/descendant) widgets.
     * */
    class WidgetSelector : public BackendClass {
    public:
        WidgetSelector() = default;

        ~WidgetSelector() override {};

        /**Add a matcher as the target widget attribute requirement.*/
        void AddMatcher(const WidgetAttrMatcher &matcher);

        /**Add a selector as the front locator widget requirement.*/
        void AddFrontLocator(const WidgetSelector &selector, ApiCallErr &error);

        /**Add a selector as the rear locator widget requirement.*/
        void AddRearLocator(const WidgetSelector &selector, ApiCallErr &error);

        /**Select all the matched widgets on the given tree. Results are arranged in the
         * receiver in <b>DFS</b> order. */
        void Select(const WidgetTree &tree, std::vector<std::reference_wrapper<const Widget>> &results) const;

        /**Returns a description of this selector.*/
        std::string Describe() const;

        const FrontEndClassDef& GetFrontendClassDef() const override
        {
            return ON_DEF;
        }

    private:
        std::vector<WidgetAttrMatcher> selfMatchers_;
        std::vector<WidgetSelector> frontLocators_;
        std::vector<WidgetSelector> rearLocators_;
    };
}

#endif