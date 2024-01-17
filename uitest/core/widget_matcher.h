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

#ifndef WIDGET_MATCHER_H
#define WIDGET_MATCHER_H

#include <string>
#include <vector>
#include "ui_model.h"

namespace OHOS::uitest {
    /**Enumerates the supported string value match rules.*/
    enum ValueMatchPattern : uint8_t { EQ, CONTAINS, STARTS_WITH, ENDS_WITH };

    /** get the readable name of the ValueMatchPattern value.*/
    std::string GetRuleName(ValueMatchPattern rule);

    class ValueMatcher {
    public:
        explicit ValueMatcher(std::string testValue, ValueMatchPattern rule = EQ)
            : testValue_(std::move(testValue)), rule_(rule) {}

        virtual ~ValueMatcher() {}

        bool Matches(std::string_view testedValue) const;

        std::string Describe() const;

    private:
        const std::string testValue_;
        const ValueMatchPattern rule_;
    };

    /**Base type of all widget matchers, test on a single Widget and returns true if it's
     * matched, false otherwise. */
    class WidgetMatcher {
    public:
        virtual bool Matches(const Widget &widget) const = 0;

        virtual std::string Describe() const = 0;
    };

    /**match the root widget node.*/
    class RootMatcher : public WidgetMatcher {
    public:
        bool Matches(const Widget &widget) const override;

        std::string Describe() const override;
    };

    /**
     * Matcher to test a single widget attribute.
     * */
    class WidgetAttrMatcher final : public WidgetMatcher {
    public:
        WidgetAttrMatcher() = delete;

        explicit WidgetAttrMatcher(std::string_view attr, std::string_view testValue, ValueMatchPattern rule);

        bool Matches(const Widget &widget) const override;

        std::string Describe() const override;

    private:
        std::string attrName_;
        std::string testVal_;
        ValueMatchPattern matchRule_;
    };

    /**
     * Matcher that evaluates several widget-attr-matchers and combine the results with operational rule <b>AND</b>.
     **/
    class All final : public WidgetMatcher {
    public:
        /**
         * Create a <code>All</code> widget matcher.
         * @param ma the plain widget matcher a.
         * @param mb the plain widget matcher b.
         * */
        All(const WidgetAttrMatcher &ma, const WidgetAttrMatcher &mb);

        /**
         * Create a <code>All</code> widget matcher.
         * @param matchers the leaf matcher list.
         * */
        explicit All(const std::vector<WidgetAttrMatcher> &matchers);

        bool Matches(const Widget &testedValue) const override;

        std::string Describe() const override;

    private:
        // hold the leaf matchers
        std::vector<WidgetAttrMatcher> matchers_;
    };

    /**
     * Matcher that evaluates several widget-attr-matchers and combine the results with operational rule <b>OR</b>.
     **/
    class Any final : public WidgetMatcher {
    public:
        /**
         * Create a <code>Any</code> widget matcher.
         * @param ma the plain widget matcher a.
         * @param mb the plain widget matcher b.
         * */
        Any(const WidgetAttrMatcher &ma, const WidgetAttrMatcher &mb);

        /**
         * Create a <code>Any</code> widget matcher.
         * @param matchers the leaf matcher list.
         * */
        explicit Any(const std::vector<WidgetAttrMatcher> &matchers);

        bool Matches(const Widget &testedValue) const override;

        std::string Describe() const override;

    private:
        // hold the leaf matchers
        std::vector<WidgetAttrMatcher> matchers_;
    };

    /**
    * Common helper visitor to find and collect matched Widget nodes by WidgetMatcher. Matcher widgets are
     * arranged in the given receiver container in visiting order.
    * */
    class MatchedWidgetCollector : public WidgetVisitor {
    public:
        MatchedWidgetCollector(const WidgetMatcher &matcher, std::vector<std::reference_wrapper<const Widget>> &recv)
            : matcher_(matcher), receiver_(recv) {}

        ~MatchedWidgetCollector() {}

        void Visit(const Widget &widget) override;

    private:
        const WidgetMatcher &matcher_;
        std::vector<std::reference_wrapper<const Widget>> &receiver_;
    };
} // namespace uitest

#endif