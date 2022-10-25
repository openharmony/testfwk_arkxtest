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

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static constexpr auto NEST_USAGE_ERROR = "Nesting By usage like 'BY.before(BY.after(...))' is not supported";

    void WidgetSelector::AddMatcher(const WidgetAttrMatcher &matcher)
    {
        selfMatchers_.emplace_back(matcher);
    }

    void WidgetSelector::AddFrontLocator(const WidgetSelector &selector, ApiCallErr &error)
    {
        if (!selector.rearLocators_.empty() || !selector.frontLocators_.empty()) {
            error = ApiCallErr(ERR_INVALID_INPUT, NEST_USAGE_ERROR);
            return;
        }
        frontLocators_.emplace_back(selector);
    }

    void WidgetSelector::AddRearLocator(const WidgetSelector &selector, ApiCallErr &error)
    {
        if (!selector.rearLocators_.empty() || !selector.frontLocators_.empty()) {
            error = ApiCallErr(ERR_INVALID_INPUT, NEST_USAGE_ERROR);
            return;
        }
        rearLocators_.emplace_back(selector);
    }

    static bool CheckHasLocator(const WidgetTree &tree, const Widget &widget, const WidgetMatcher &matcher, bool front)
    {
        vector<reference_wrapper<const Widget>> locators;
        locators.clear();
        MatchedWidgetCollector collector(matcher, locators);
        if (front) {
            tree.DfsTraverseFronts(collector, widget);
        } else {
            tree.DfsTraverseRears(collector, widget);
        }
        return !locators.empty();
    }

    void WidgetSelector::Select(const WidgetTree &tree, vector<std::reference_wrapper<const Widget>> &results) const
    {
        auto allSelfMatcher = All(selfMatchers_);
        MatchedWidgetCollector selfCollector(allSelfMatcher, results);
        tree.DfsTraverse(selfCollector);
        if (results.empty()) {
            LOG_W("Self node not found matching:%{public}s", allSelfMatcher.Describe().c_str());
            return;
        }

        vector<uint32_t> discardWidgetOffsets;
        // check locators at each direction and filter out unsatisfied widgets
        for (size_t idx = 0; idx < INDEX_TWO; idx++) {
            discardWidgetOffsets.clear();
            uint32_t offset = 0;
            for (auto &result:results) {
                const bool front = idx == 0;
                const auto &locators = front ? frontLocators_ : rearLocators_;
                for (auto &locator:locators) {
                    auto locatorMatcher = All(locator.selfMatchers_);
                    if (!CheckHasLocator(tree, result.get(), locatorMatcher, front)) {
                        // this means not all the required front locator are found, exclude this candidate
                        discardWidgetOffsets.emplace_back(offset);
                        break;
                    }
                }
                offset++;
            }
            // remove unsatisfied candidates, remove from last to first
            reverse(discardWidgetOffsets.begin(), discardWidgetOffsets.end());
            for (auto &off:discardWidgetOffsets) {
                results.erase(results.begin() + off);
            }
            if (results.empty()) {
                break;
            }
        }
    }

    string WidgetSelector::Describe() const
    {
        stringstream ss;
        auto allSelfMatcher = All(selfMatchers_);
        ss << "{";
        ss << "selfMatcher=[" << allSelfMatcher.Describe() << "]";
        if (!frontLocators_.empty()) {
            ss << "; frontMatcher=";
            for (auto &locator:frontLocators_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        if (!rearLocators_.empty()) {
            ss << "; rearMatcher=";
            for (auto &locator:rearLocators_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        ss << "}";
        return ss.str();
    }
}