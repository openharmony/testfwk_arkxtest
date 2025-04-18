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

#include "widget_selector.h"
#include "climits"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static constexpr auto NEST_USAGE_ERROR = "Nesting By usage like 'BY.before(BY.after(...))' is not supported";

    WidgetSelector::WidgetSelector(bool addVisibleMatcher)
    {
        if (!addVisibleMatcher) {
            return;
        }
        auto visibleMatcher = WidgetMatchModel(UiAttr::VISIBLE, "true", EQ);
        selfMatchers_.emplace_back(visibleMatcher);
    }

    void WidgetSelector::AddMatcher(const WidgetMatchModel &matcher)
    {
        selfMatchers_.emplace_back(matcher);
    }

    void WidgetSelector::AddFrontLocator(const WidgetSelector &selector, ApiCallErr &error)
    {
        if (!selector.rearLocators_.empty() || !selector.frontLocators_.empty() || !selector.parentLocators_.empty()) {
            error = ApiCallErr(ERR_INVALID_INPUT, NEST_USAGE_ERROR);
            return;
        }
        frontLocators_.emplace_back(selector);
    }

    void WidgetSelector::AddRearLocator(const WidgetSelector &selector, ApiCallErr &error)
    {
        if (!selector.rearLocators_.empty() || !selector.frontLocators_.empty() || !selector.parentLocators_.empty()) {
            error = ApiCallErr(ERR_INVALID_INPUT, NEST_USAGE_ERROR);
            return;
        }
        rearLocators_.emplace_back(selector);
    }

    void WidgetSelector::AddParentLocator(const WidgetSelector &selector, ApiCallErr &error)
    {
        if (!selector.rearLocators_.empty() || !selector.frontLocators_.empty() || !selector.parentLocators_.empty()) {
            error = ApiCallErr(ERR_INVALID_INPUT, NEST_USAGE_ERROR);
            return;
        }
        parentLocators_.emplace_back(selector);
    }

    void WidgetSelector::AddAppLocator(string app)
    {
        appLocator_ = app;
    }

    std::string WidgetSelector::GetAppLocator() const
    {
        return appLocator_;
    }

    void WidgetSelector::AddDisplayLocator(int32_t displayId)
    {
        displayLocator_ = displayId;
    }

    std::int32_t WidgetSelector::GetDisplayLocator() const
    {
        return displayLocator_;
    }

    string WidgetSelector::Describe() const
    {
        stringstream ss;
        ss << "{";
        ss << "selfMatcher=[";
        for (auto &match : selfMatchers_) {
            ss << match.Describe() << ",";
        }
        ss << "]";
        if (!frontLocators_.empty()) {
            ss << "; frontMatcher=";
            for (auto &locator : frontLocators_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        if (!rearLocators_.empty()) {
            ss << "; rearMatcher=";
            for (auto &locator : rearLocators_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        if (!parentLocators_.empty()) {
            ss << "; parentMatcher=";
            for (auto &locator : parentLocators_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        ss << "}";
        return ss.str();
    }

    void WidgetSelector::SetWantMulti(bool wantMulti)
    {
        wantMulti_ = wantMulti;
    }

    bool WidgetSelector::IsWantMulti() const
    {
        return wantMulti_;
    }

    void WidgetSelector::Select(const Window window,
                                ElementNodeIterator &elementNodeRef,
                                std::vector<Widget> &visitWidgets,
                                std::vector<int> &targetWidgets) const
    {
        std::unique_ptr<SelectStrategy> visitStrategy = ConstructSelectStrategy();
        LOG_D("Do Select, select strategy is %{public}d", visitStrategy->GetStrategyType());
        DumpOption option;
        visitStrategy->LocateNode(window, elementNodeRef, visitWidgets, targetWidgets, option);
    }

    std::vector<WidgetMatchModel> WidgetSelector::GetSelfMatchers() const
    {
        return selfMatchers_;
    }

    std::unique_ptr<SelectStrategy> WidgetSelector::ConstructSelectStrategy() const
    {
        StrategyBuildParam buildParam;
        buildParam.myselfMatcher = selfMatchers_;
        if (!frontLocators_.empty()) {
            for (const auto &frontLocator : frontLocators_) {
                buildParam.afterAnchorMatcherVec.emplace_back(frontLocator.GetSelfMatchers());
            }
        }
        if (!rearLocators_.empty()) {
            for (const auto &beforeLocator : rearLocators_) {
                buildParam.beforeAnchorMatcherVec.emplace_back(beforeLocator.GetSelfMatchers());
            }
        }
        if (!parentLocators_.empty()) {
            for (const auto &withInLocator : parentLocators_) {
                buildParam.withInAnchorMatcherVec.emplace_back(withInLocator.GetSelfMatchers());
            }
        }
        return SelectStrategy::BuildSelectStrategy(buildParam, wantMulti_);
    }
} // namespace OHOS::uitest