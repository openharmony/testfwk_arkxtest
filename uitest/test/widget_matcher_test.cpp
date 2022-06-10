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

#include "gtest/gtest.h"
#include "widget_matcher.h"
#include "ui_model.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr auto ATTR_TEXT = "text";

TEST(ValueMatcherTest, eqMatcher)
{
    auto matcher = ValueMatcher("wyz", EQ);
    ASSERT_TRUE(matcher.Matches("wyz"));
    ASSERT_FALSE(matcher.Matches("yz"));
    ASSERT_FALSE(matcher.Matches("wy"));
    ASSERT_FALSE(matcher.Matches(""));
    ASSERT_FALSE(matcher.Matches("ab"));
}

TEST(ValueMatcherTest, containsMatcher)
{
    auto matcher = ValueMatcher("wyz", CONTAINS);
    ASSERT_TRUE(matcher.Matches("wyz"));
    ASSERT_TRUE(matcher.Matches("awyzb"));
    ASSERT_TRUE(matcher.Matches("wyzab"));
    ASSERT_FALSE(matcher.Matches(""));
    ASSERT_FALSE(matcher.Matches("ab"));
}

TEST(ValueMatcherTest, startsWithMatcher)
{
    auto matcher = ValueMatcher("wyz", STARTS_WITH);
    ASSERT_TRUE(matcher.Matches("wyz"));
    ASSERT_FALSE(matcher.Matches("yz"));
    ASSERT_TRUE(matcher.Matches("wyza"));
    ASSERT_TRUE(matcher.Matches("wyzyzw"));
    ASSERT_FALSE(matcher.Matches("ab"));
}

TEST(ValueMatcherTest, endsWithMatcher)
{
    auto matcher = ValueMatcher("wyz", ENDS_WITH);
    ASSERT_TRUE(matcher.Matches("wyz"));
    ASSERT_TRUE(matcher.Matches("wawyz"));
    ASSERT_FALSE(matcher.Matches("wyza"));
    ASSERT_FALSE(matcher.Matches("wyzz"));
}

TEST(ValueMatcherTest, matcherDesc)
{
    const auto testValue = "wyz";
    const auto rules = {EQ, CONTAINS, STARTS_WITH, ENDS_WITH};
    for (auto &rule:rules) {
        auto matcher = ValueMatcher(testValue, rule);
        auto desc = matcher.Describe();
        // desc should contains the test_value
        ASSERT_TRUE(desc.find(testValue) != string::npos);
        // desc should contains the match rule
        ASSERT_TRUE(desc.find(GetRuleName(rule)) != string::npos);
    }
}

TEST(WidgetMatcherTest, widgetMatcher)
{
    Widget widget("");
    widget.SetAttr(ATTR_TEXT, "wyz");

    auto matcherA = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    ASSERT_TRUE(matcherA.Matches(widget));
    widget.SetAttr(ATTR_TEXT, "wlj");
    ASSERT_FALSE(matcherA.Matches(widget));
}

TEST(WidgetMatcherTest, widgetMatcherDesc)
{
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto desc = matcher.Describe();
    // desc should contains the target attribute name
    ASSERT_TRUE(desc.find(ATTR_TEXT) != string::npos);
    // desc should contains the match rule
    ASSERT_TRUE(desc.find(GetRuleName(EQ)) != string::npos);
    // desc should contains the test attribute value
    ASSERT_TRUE(desc.find("wyz") != string::npos);
}

TEST(WidgetMatcherTest, rootMatcher)
{
    Widget widget0("ROOT");
    Widget widget1("123");
    auto matcher = RootMatcher();

    ASSERT_TRUE(matcher.Matches(widget0));
    ASSERT_FALSE(matcher.Matches(widget1));
    auto desc = matcher.Describe();
    ASSERT_TRUE(desc.find("root") != string::npos);
}

TEST(WidgetMatcherTest, allWidgetAttrMatcher)
{
    Widget widget("");
    widget.SetAttr(ATTR_TEXT, "wyz");

    auto matcherA = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto matcherB = WidgetAttrMatcher(ATTR_TEXT, "yz", CONTAINS);
    auto matcherC = WidgetAttrMatcher(ATTR_TEXT, "yz", STARTS_WITH);

    auto allMatcher0 = All(matcherA, matcherB);
    auto allMatcher1 = All(matcherA, matcherC);

    ASSERT_TRUE(allMatcher0.Matches(widget));
    ASSERT_FALSE(allMatcher1.Matches(widget));
}

TEST(WidgetMatcherTest, allWidgetMatcherDesc)
{
    auto matcherA = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto matcherB = WidgetAttrMatcher(ATTR_TEXT, "yz", CONTAINS);
    auto allMatcher = All(matcherA, matcherB);

    auto matcherADesc = matcherA.Describe();
    auto matcherBDesc = matcherB.Describe();
    auto allMatcherDesc = allMatcher.Describe();

    // desc should contains the description of each leaf matchers
    ASSERT_TRUE(allMatcherDesc.find(matcherADesc) != string::npos);
    ASSERT_TRUE(allMatcherDesc.find(matcherBDesc) != string::npos);
    // desc should contains the 'AND' operand
    ASSERT_TRUE(allMatcherDesc.find("AND") != string::npos);
}

TEST(WidgetMatcherTest, anyWidgetAttrMatcher)
{
    Widget widget("");
    widget.SetAttr(ATTR_TEXT, "wyz");

    auto matcherA = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto matcherB = WidgetAttrMatcher(ATTR_TEXT, "yz", CONTAINS);
    auto matcherC = WidgetAttrMatcher(ATTR_TEXT, "yz", STARTS_WITH);
    auto matcherD = WidgetAttrMatcher(ATTR_TEXT, "wy", ENDS_WITH);

    auto anyMatcher0 = Any(matcherA, matcherB);
    auto anyMatcher1 = Any(matcherA, matcherC);
    auto anyMatcher2 = Any(matcherC, matcherD);
    ASSERT_TRUE(anyMatcher0.Matches(widget));
    ASSERT_TRUE(anyMatcher1.Matches(widget));
    ASSERT_FALSE(anyMatcher2.Matches(widget));
}

// test the vector-parameter version
TEST(WidgetMatcherTest, anyWidgetAttrMatcher2)
{
    Widget widget("");
    widget.SetAttr(ATTR_TEXT, "wyz");

    auto matcherA = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto matcherB = WidgetAttrMatcher(ATTR_TEXT, "yz", CONTAINS);
    auto matcherC = WidgetAttrMatcher(ATTR_TEXT, "yz", STARTS_WITH);
    auto matcherD = WidgetAttrMatcher(ATTR_TEXT, "wy", ENDS_WITH);
    vector<WidgetAttrMatcher> vec0;
    vector<WidgetAttrMatcher> vec1;
    vector<WidgetAttrMatcher> vec2;
    vec0.emplace_back(matcherA);
    vec0.emplace_back(matcherB);
    vec1.emplace_back(matcherA);
    vec1.emplace_back(matcherC);
    vec2.emplace_back(matcherC);
    vec2.emplace_back(matcherD);

    auto anyMatcher0 = Any(vec0);
    auto anyMatcher1 = Any(vec1);
    auto anyMatcher2 = Any(vec2);
    ASSERT_TRUE(anyMatcher0.Matches(widget));
    ASSERT_TRUE(anyMatcher1.Matches(widget));
    ASSERT_FALSE(anyMatcher2.Matches(widget));
}

TEST(WidgetMatcherTest, anyWidgetMatcherDesc)
{
    auto matcherA = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto matcherB = WidgetAttrMatcher(ATTR_TEXT, "yz", CONTAINS);
    auto anyMatcher = Any(matcherA, matcherB);

    auto matcherADesc = matcherA.Describe();
    auto matcherBDesc = matcherB.Describe();
    auto anyMatcherDesc = anyMatcher.Describe();

    // desc should contains the description of each leaf matchers
    ASSERT_TRUE(anyMatcherDesc.find(matcherADesc) != string::npos);
    ASSERT_TRUE(anyMatcherDesc.find(matcherBDesc) != string::npos);
    // desc should contains the 'AND' operand
    ASSERT_TRUE(anyMatcherDesc.find("OR") != string::npos);
}