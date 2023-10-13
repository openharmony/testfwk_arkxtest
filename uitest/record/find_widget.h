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

#ifndef FIND_WIDGET_H
#define FIND_WIDGET_H

#include "ui_driver.h"
#include "ui_model.h"
#include "widget_operator.h"
#include "widget_selector.h"
#include "widget_matcher.h"
#include "frontend_api_defines.h"
namespace OHOS::uitest {
    const Widget FindWidget(UiDriver &driver, float x, float y);
    class WidgetMatcherByCoord final : public WidgetMatcher {
    public:
        WidgetMatcherByCoord() = delete;

        explicit WidgetMatcherByCoord(float x, float y)
        {
            x_ = x;
            y_ = y;
        }
        std::string Describe() const override;

        bool Matches(const Widget &widget) const override;

    private:
        float x_;
        float y_;
    };

    class WidgetCollector : public WidgetVisitor {
    public:
        WidgetCollector(const WidgetMatcherByCoord &matcher, std::map<float, Widget> &recv, Point down)
            : matcher_(matcher), receiver_(recv), downPoint_(down) {}

        ~WidgetCollector() {}

        void Visit(const Widget &widget) override;

        int32_t GetDept(const Widget &widget) const;
        
        const Widget GetMaxDepWidget()
        {
            Widget widget("");
            if (receiver_.find(maxDep) != receiver_.end()) {
                return receiver_.find(maxDep)->second;
            } else {
                LOG_W("MaxDep widget not found.");
                return widget;
            }
        }

    private:
        const WidgetMatcherByCoord &matcher_;
        std::map<float, Widget> &receiver_;
        int32_t maxDep = 0;
        Point downPoint_;
    };
} // namespace OHOS::uitest

#endif // FIND_WIDGET_H