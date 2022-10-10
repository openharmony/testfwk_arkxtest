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
#include "widget_selector.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr char DOM_TEXT[] = R"({
"attributes": {
"rotation": "0"
},
"children": [
{
"attributes": {
"resource-id": "id1",
"text": ""
},
"children": [
{
"attributes": {
"resource-id": "id2",
"text": ""
},
"children": [
{
"attributes": {
"resource-id": "id3",
"text": ""
},
"children": [
{
"attributes": {
"resource-id": "id4",
"text": "Use USB to"
},
"children": []
}
]
}
]
},
{
"attributes": {
"resource-id": "id5",
"text": "wyz"
},
"children": [
{
"attributes": {
"resource-id": "id6",
"text": ""
},
"children": [
{
"attributes": {
"resource-id": "id7",
"text": ""
},
"children": [
{
"attributes": {
"resource-id": "id8",
"text": "Transfer photos"
},
"children": []
}
]
},
{
"attributes": {
"resource-id": "id9",
"text": ""
},
"children": [
{
"attributes": {
"resource-id": "id10",
"text": "Transfer files"
},
"children": []
}
]
}
]
}
]
}
]
}
]
}
)";

static constexpr auto ATTR_TEXT = "text";

class WidgetSelectorTest : public testing::Test {
protected:
    WidgetTree tree_ = WidgetTree("dummy_tree");

    void SetUp() override
    {
        auto data = nlohmann::json::parse(DOM_TEXT);
        tree_.ConstructFromDom(data, false);
    }
};

TEST_F(WidgetSelectorTest, signleMatcherWithoutLocatorAndExistsNoTarget)
{
    auto selector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "WLJ", EQ);
    selector.AddMatcher(matcher);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    ASSERT_EQ(0, receiver.size());
}

TEST_F(WidgetSelectorTest, signleMatcherWithoutLocatorAndExistsSingleTarget)
{
    auto selector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    selector.AddMatcher(matcher);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    ASSERT_EQ(1, receiver.size());
    // check the result widget
    ASSERT_EQ("id5", receiver.at(0).get().GetAttr("resource-id", ""));
}

TEST_F(WidgetSelectorTest, multiMatcherWithoutLocatorAndExistsSingleTarget)
{
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    auto matcher1 = WidgetAttrMatcher("resource-id", "8", CONTAINS);
    selector.AddMatcher(matcher0);
    selector.AddMatcher(matcher1);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    ASSERT_EQ(1, receiver.size());
    // check the result widget
    ASSERT_EQ("id8", receiver.at(0).get().GetAttr("resource-id", ""));
}

TEST_F(WidgetSelectorTest, multiMatcherWithoutLocatorAndExistMultiTargets)
{
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    ASSERT_EQ(2, receiver.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("id8", receiver.at(0).get().GetAttr("resource-id", ""));
    ASSERT_EQ("id10", receiver.at(1).get().GetAttr("resource-id", ""));
}

TEST_F(WidgetSelectorTest, singleFrontLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id10", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id10" is not in front of "*Transfer*", so no widget should be selected
    ASSERT_EQ(0, receiver.size());
}

TEST_F(WidgetSelectorTest, singleFrontLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id9", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id9" is not in front of "Transfer files" but not "Transfer photos", so one widget should be selected
    ASSERT_EQ(1, receiver.size());
    ASSERT_EQ("Transfer files", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, singleFrontLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id7", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id9" is not in front of "Transfer files" and "Transfer photos", so two widgets should be selected
    ASSERT_EQ(2, receiver.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("Transfer photos", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
    ASSERT_EQ("Transfer files", receiver.at(1).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, multiFrontLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher0 = WidgetAttrMatcher("resource-id", "id1", EQ);
    auto frontLocator0 = WidgetSelector();
    frontLocator0.AddMatcher(frontMatcher0);
    selector.AddFrontLocator(frontLocator0, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto frontMatcher1 = WidgetAttrMatcher("resource-id", "id10", EQ);
    auto frontLocator1 = WidgetSelector();
    frontLocator1.AddMatcher(frontMatcher1);
    selector.AddFrontLocator(frontLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id1" is in front of "*Transfer*" but "id10" isn't, so no widget should be selected
    ASSERT_EQ(0, receiver.size());
}

TEST_F(WidgetSelectorTest, multiFrontLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher0 = WidgetAttrMatcher("resource-id", "id8", EQ);
    auto frontLocator0 = WidgetSelector();
    frontLocator0.AddMatcher(frontMatcher0);
    selector.AddFrontLocator(frontLocator0, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto frontMatcher1 = WidgetAttrMatcher("resource-id", "id9", EQ);
    auto frontLocator1 = WidgetSelector();
    frontLocator1.AddMatcher(frontMatcher1);
    selector.AddFrontLocator(frontLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // both "id8" and "id9" are in front of "Transfer files" but not "Transfer photos", so one widget should be selected
    ASSERT_EQ(1, receiver.size());
    ASSERT_EQ("Transfer files", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, multiFrontLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher0 = WidgetAttrMatcher("resource-id", "id6", EQ);
    auto frontLocator0 = WidgetSelector();
    frontLocator0.AddMatcher(frontMatcher0);
    selector.AddFrontLocator(frontLocator0, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto frontMatcher1 = WidgetAttrMatcher("resource-id", "id7", EQ);
    auto frontLocator1 = WidgetSelector();
    frontLocator1.AddMatcher(frontMatcher1);
    selector.AddFrontLocator(frontLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    // both "id8" and "id9" are in front of "Transfer files" but not "*Transfer*", two widgets should be selected
    ASSERT_EQ(2, receiver.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("Transfer photos", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
    ASSERT_EQ("Transfer files", receiver.at(1).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, singleRearLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto rearMatcher = WidgetAttrMatcher("resource-id", "id7", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id10" is not in rear of "*Transfer*", so no widget should be selected
    ASSERT_EQ(0, receiver.size());
}

TEST_F(WidgetSelectorTest, singleRearLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto rearMatcher = WidgetAttrMatcher("resource-id", "id9", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id9" is not in rear of "Transfer photos" but not "Transfer files", so one widget should be selected
    ASSERT_EQ(1, receiver.size());
    ASSERT_EQ("Transfer photos", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, singleRearLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "s", CONTAINS);
    selector.AddMatcher(matcher0);

    auto rearMatcher = WidgetAttrMatcher("resource-id", "id10", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // "id10" is rear in front of "Use USB to" and "Transfer photos", so two widgets should be selected
    ASSERT_EQ(2, receiver.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("Use USB to", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
    ASSERT_EQ("Transfer photos", receiver.at(1).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, frontAndRearLocatorsAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id8", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetAttrMatcher("resource-id", "id10", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);
    // no "*Transfer*" is between "id8" and "id10", so no widget should be selected
    ASSERT_EQ(0, receiver.size());
}

TEST_F(WidgetSelectorTest, frontAndRearLocatorsAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "Transfer", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id7", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetAttrMatcher("resource-id", "id10", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    // only "Transfer photos" is between "id7" and "id10", one widget should be selected
    ASSERT_EQ(1, receiver.size());
    ASSERT_EQ("Transfer photos", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, frontAndRearLocatorsAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "s", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id3", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetAttrMatcher("resource-id", "id9", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    vector<reference_wrapper<const Widget>> receiver;
    selector.Select(tree_, receiver);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(2, receiver.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("Use USB to", receiver.at(0).get().GetAttr(ATTR_TEXT, ""));
    ASSERT_EQ("Transfer photos", receiver.at(1).get().GetAttr(ATTR_TEXT, ""));
}

TEST_F(WidgetSelectorTest, nestingUsageDetect)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "s", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id3", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    ASSERT_EQ(NO_ERROR, err.code_);
    frontLocator.AddRearLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    // make the front locator nesting
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(ERR_INVALID_INPUT, err.code_);
}

TEST_F(WidgetSelectorTest, selectorDescription)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetAttrMatcher(ATTR_TEXT, "s", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetAttrMatcher("resource-id", "id3", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetAttrMatcher("resource-id", "id9", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto description = selector.Describe();
    auto pos0 = description.find("selfMatcher");
    auto pos1 = description.find(matcher0.Describe());
    auto pos2 = description.find("frontMatcher");
    auto pos3 = description.find(frontMatcher.Describe());
    auto pos4 = description.find("rearMatcher");
    auto pos5 = description.find(rearMatcher.Describe());
    ASSERT_TRUE(pos0 != string::npos && pos0 > 0);
    ASSERT_TRUE(pos1 != string::npos && pos1 > pos0);
    ASSERT_TRUE(pos2 != string::npos && pos2 > pos1);
    ASSERT_TRUE(pos3 != string::npos && pos3 > pos2);
    ASSERT_TRUE(pos4 != string::npos && pos4 > pos3);
    ASSERT_TRUE(pos5 != string::npos && pos5 > pos4);
}