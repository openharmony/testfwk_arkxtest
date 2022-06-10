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

#include "widget_matcher.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    string GetRuleName(ValueMatchPattern rule)
    {
        switch (rule) {
            case EQ:
                return "=";
            case CONTAINS:
                return "contains";
            case STARTS_WITH:
                return "startsWith";
            case ENDS_WITH:
                return "endsWith";
        }
    }

    bool ValueMatcher::Matches(string_view testedValue) const
    {
        switch (rule_) {
            case EQ:
                return testedValue == testValue_;
            case CONTAINS:
                return testedValue.find(testValue_) != string::npos;
            case STARTS_WITH:
                return testedValue.find(testValue_) == 0;
            case ENDS_WITH:
                return (testedValue.find(testValue_) == (testedValue.length() - testValue_.length()));
        }
    }

    string ValueMatcher::Describe() const
    {
        stringstream ss;
        ss << GetRuleName(rule_);
        ss << " '" << testValue_ << "'";
        return ss.str();
    }

    WidgetAttrMatcher::WidgetAttrMatcher(string_view attr, string_view testValue, ValueMatchPattern rule)
        : attrName_(attr), testVal_(testValue), matchRule_(rule) {}

    bool WidgetAttrMatcher::Matches(const Widget &widget) const
    {
        if (!widget.HasAttr(attrName_)) {
            return false;
        }
        auto attrValue = widget.GetAttr(attrName_, "");
        auto valueMatcher = ValueMatcher(testVal_, matchRule_);
        return valueMatcher.Matches(attrValue);
    };

    string WidgetAttrMatcher::Describe() const
    {
        auto valueMatcher = ValueMatcher(testVal_, matchRule_);
        return "$" + string(attrName_) + " " + valueMatcher.Describe();
    }

    All::All(const WidgetAttrMatcher &ma, const WidgetAttrMatcher &mb)
    {
        matchers_.emplace_back(ma);
        matchers_.emplace_back(mb);
    }

    All::All(const vector<WidgetAttrMatcher> &matchers)
    {
        for (auto &ref:matchers) {
            this->matchers_.emplace_back(ref);
        }
    }

    bool All::Matches(const Widget &testedValue) const
    {
        bool match = true;
        for (auto &matcher:matchers_) {
            match = match && matcher.Matches(testedValue);
            if (!match) {
                break;
            }
        }
        return match;
    }

    string All::Describe() const
    {
        stringstream desc;
        uint32_t index = 0;
        for (auto &matcher:matchers_) {
            if (index > 0) {
                desc << " AND ";
            }
            desc << "(" << matcher.Describe() << ")";
            index++;
        }
        return desc.str();
    }

    bool RootMatcher::Matches(const Widget &widget) const
    {
        return WidgetTree::IsRootWidgetHierarchy(widget.GetHierarchy());
    }

    string RootMatcher::Describe() const
    {
        return "the root widget on the tree";
    }

    void MatchedWidgetCollector::Visit(const Widget &widget)
    {
        if (matcher_.Matches(widget)) {
            receiver_.emplace_back(widget);
        }
    }

    Any::Any(const WidgetAttrMatcher &ma, const WidgetAttrMatcher &mb)
    {
        matchers_.emplace_back(ma);
        matchers_.emplace_back(mb);
    }

    Any::Any(const vector<WidgetAttrMatcher> &matchers)
    {
        for (auto &ref:matchers) {
            this->matchers_.emplace_back(ref);
        }
    }

    bool Any::Matches(const Widget &testedValue) const
    {
        bool match = false;
        for (auto &matcher:matchers_) {
            match = match || matcher.Matches(testedValue);
            if (match) break;
        }
        return match;
    }

    string Any::Describe() const
    {
        stringstream desc;
        uint32_t index = 0;
        for (auto &matcher:matchers_) {
            if (index > 0) {
                desc << " OR ";
            }
            desc << "(" << matcher.Describe() << ")";
            index++;
        }
        return desc.str();
    }
} // namespace uitest