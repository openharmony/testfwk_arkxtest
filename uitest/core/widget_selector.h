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
#include "select_strategy.h"

namespace OHOS::uitest {
    /**
     * Selector that searches Widgets on the given WidgetTree according to the matchers
     * of the target widget and its adjacent (front/rear/ancestor/descendant) widgets.
     * */
    class WidgetSelector : public BackendClass {
    public:
        explicit WidgetSelector(bool addVisibleMatcher = true);

        ~WidgetSelector() override{};

        /**Add a matcher as the target widget attribute requirement.*/
        void AddMatcher(const WidgetMatchModel &matcher);

        /**Add a selector as the front locator widget requirement.*/
        void AddFrontLocator(const WidgetSelector &selector, ApiCallErr &error);

        /**Add a selector as the rear locator widget requirement.*/
        void AddRearLocator(const WidgetSelector &selector, ApiCallErr &error);

        void AddParentLocator(const WidgetSelector &selector, ApiCallErr &error);

        void AddAppLocator(string appName);

        /**Returns a description of this selector.*/
        std::string Describe() const;

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return ON_DEF;
        }

        void SetWantMulti(bool wantMulti);

        bool IsWantMulti() const;

        void Select(const Window window,
                    ElementNodeIterator &elementNodeRef,
                    std::vector<Widget> &visitWidgets,
                    std::vector<int> &targetWidgets) const;

    private:
        std::vector<WidgetMatchModel> GetSelfMatchers() const;
        std::unique_ptr<SelectStrategy> ConstructSelectStrategy() const;
        std::vector<WidgetMatchModel> selfMatchers_;
        std::vector<WidgetSelector> frontLocators_;
        std::vector<WidgetSelector> rearLocators_;
        std::vector<WidgetSelector> parentLocators_;
        string appLocator_ = "";
        bool wantMulti_ = false;
    };
} // namespace OHOS::uitest

#endif