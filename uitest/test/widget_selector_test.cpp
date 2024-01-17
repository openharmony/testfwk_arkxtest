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
#include "mock_element_node_iterator.h"
#include "mock_controller.h"

using namespace OHOS::uitest;
using namespace std;

class WidgetSelectorTest : public testing::Test {
protected:
    void SetUp() override
    {
    std::string domJson = R"(
        {
            "attributes":{
                "windowId":"12",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                     "attributes":{
                        "windowId":"12",
                        "accessibilityId":"2",
                        "componentType":"List",
                        "content":"Text List",
                        "rectInScreen":"30,60,10,120"
                     },
                     "children":[
                        {
                            "attributes":{
                                "windowId":"12",
                                "accessibilityId":"3",
                                "componentType":"Image",
                                "content":"Button",
                                "rectInScreen":"30,40,10,20"
                            },
                            "children":[
                                {
                                    "attributes":{
                                        "windowId":"12",
                                        "accessibilityId":"4",
                                        "componentType":"Text",
                                        "content":"Text One",
                                        "rectInScreen":"30,40,10,20"
                                    },
                                    "children":[]
                                }
                            ]
                        },
                        {
                            "attributes":{
                                "windowId":"12",
                                "componentType":"List",
                                "accessibilityId":"5",
                                "content":"Text List12",
                                "rectInScreen":"40,60,10,20"
                            },
                            "children":[
                                {
                                    "attributes":{
                                        "windowId":"12",
                                        "accessibilityId":"6",
                                        "componentType":"Text",
                                        "content":"Text One",
                                        "rectInScreen":"40,50,10,20"
                                    },
                                    "children":[]
                                },
                                {
                                    "attributes":{
                                        "windowId":"12",
                                        "accessibilityId":"8",
                                        "componentType":"Text",
                                        "content":"Text Two",
                                        "rectInScreen":"50,60,10,20"
                                    },
                                    "children":[]
                                }
                            ]
                        }
                     ]
                },
                {
                     "attributes":{
                        "windowId":"12",
                        "accessibilityId":"10",
                        "componentType":"Image",
                        "content":"Button",
                        "rectInScreen":"10,20,20,100"
                     },
                     "children":[]
                },
                {
                     "attributes":{
                        "windowId":"12",
                        "accessibilityId":"11",
                        "componentType":"Scroll",
                        "content":"Button",
                        "rectInScreen":"20,100,20,100"
                     },
                     "children":[
                        {
                            "attributes":{
                                "windowId":"12",
                                "accessibilityId":"12",
                                "componentType":"Image",
                                "content":"",
                                "rectInScreen":"20,100,20,100"
                            },
                            "children":[]
                        },
                        {
                            "attributes":{
                                "windowId":"12",
                                "accessibilityId":"14",
                                "componentType":"Button",
                                "content":"Text End Button",
                                "rectInScreen":"20,100,20,100"
                            },
                            "children":[
                                {
                                    "attributes":{
                                        "windowId":"12",
                                        "accessibilityId":"15",
                                        "componentType":"Text",
                                        "content":"Text End",
                                        "rectInScreen":"20,100,20,100"
                                    },
                                    "children":[]
                                }
                            ]
                        }
                     ]
                }
            ]
        }
    )";
        iterator = MockElementNodeIterator::ConstructIteratorByJson(domJson);
        w1.windowLayer_ = 2;
        w1.bounds_ = Rect{0, 1000, 0, 1200};
        w1.bundleName_ = "test1";
    }
    Window w1{12};
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ~WidgetSelectorTest() override = default;
};

TEST_F(WidgetSelectorTest, signleMatcherWithoutLocatorAndExistsNoTarget)
{
    auto selector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "WLJ", EQ);
    selector.AddMatcher(matcher);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, signleMatcherWithoutLocatorAndExistsSingleTarget)
{
    auto selector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(1, targets.size());
    // check the result widget
    ASSERT_EQ("4", visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, multiMatcherWithoutLocatorAndExistsSingleTarget)
{
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    auto matcher1 = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "6", EQ);
    selector.AddMatcher(matcher0);
    selector.AddMatcher(matcher1);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(1, targets.size());
    // check the result widget
    ASSERT_EQ("6", visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, multiMatcherWithoutLocatorAndExistMultiTargets)
{
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);
    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(2, targets.size());
    // check the result widgets and the order, should arranges in DFS order

    ASSERT_EQ("4", visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("6", visits[targets[1]].GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, singleFrontLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text List", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id10" is not in front of "*Transfer*", so no widget should be selected
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, singleFrontLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id10" is not in front of "*Transfer*", so no widget should be selected
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("6", visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, singleFrontLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", STARTS_WITH);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    selector.SetWantMulti(true);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id9" is not in front of "Transfer files" and "Transfer photos", so two widgets should be selected
    ASSERT_EQ(2, targets.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("14", visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("15", visits[targets[1]].GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, multiFrontLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text List", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher0 = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator0 = WidgetSelector();
    frontLocator0.AddMatcher(frontMatcher0);
    selector.AddFrontLocator(frontLocator0, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto frontMatcher1 = WidgetMatchModel(UiAttr::TYPE, "Image", EQ);
    auto frontLocator1 = WidgetSelector();
    frontLocator1.AddMatcher(frontMatcher1);
    selector.AddFrontLocator(frontLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id1" is in front of "*Transfer*" but "id10" isn't, so no widget should be selected
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, multiFrontLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text Two", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher0 = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator0 = WidgetSelector();
    frontLocator0.AddMatcher(frontMatcher0);
    selector.AddFrontLocator(frontLocator0, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto frontMatcher1 = WidgetMatchModel(UiAttr::TYPE, "Image", EQ);
    auto frontLocator1 = WidgetSelector();
    frontLocator1.AddMatcher(frontMatcher1);
    selector.AddFrontLocator(frontLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id1" is in front of "*Transfer*" but "id10" isn't, so no widget should be selected
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("8", visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, multiFrontLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", STARTS_WITH);
    selector.AddMatcher(matcher0);

    auto frontMatcher0 = WidgetMatchModel(UiAttr::TYPE, "Image", EQ);
    auto frontLocator0 = WidgetSelector();
    frontLocator0.AddMatcher(frontMatcher0);
    selector.AddFrontLocator(frontLocator0, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto frontMatcher1 = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator1 = WidgetSelector();
    frontLocator1.AddMatcher(frontMatcher1);
    selector.AddFrontLocator(frontLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // both "id8" and "id9" are in front of "Transfer files" but not "*Transfer*", two widgets should be selected
    ASSERT_EQ(4, targets.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(ACCESSIBILITY_ID));
    ASSERT_EQ("8", visits.at(targets.at(1)).GetAttr(ACCESSIBILITY_ID));
    ASSERT_EQ("14", visits.at(targets.at(2)).GetAttr(ACCESSIBILITY_ID));
    ASSERT_EQ("15", visits.at(targets.at(3)).GetAttr(ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, singleRearLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text End", EQ);
    selector.AddMatcher(matcher0);

    auto rearMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id10" is not in rear of "*Transfer*", so no widget should be selected
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, singleRearLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text Two", EQ);
    selector.AddMatcher(matcher0);

    auto rearMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id9" is not in rear of "Transfer photos" but not "Transfer files", so one widget should be selected
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("8", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, singleRearLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher0);

    auto rearMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // "id9" is not in rear of "Transfer photos" but not "Transfer files", so one widget should be selected
    ASSERT_EQ(2, targets.size());
    ASSERT_EQ("4", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("6", visits.at(targets.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, frontAndRearLocatorsAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text Two", EQ);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    // no "*Transfer*" is between "id8" and "id10", so no widget should be selected
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, frontAndRearLocatorsAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // only "Transfer photos" is between "id7" and "id10", one widget should be selected
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, frontAndRearLocatorsAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetMatchModel(UiAttr::TYPE, "Button", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(2, targets.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("8", visits.at(targets.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));

    selector.SetWantMulti(false);
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    selector.Select(w1, *iterator.get(), visits2, targets2);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(1, targets2.size());
    // check the result widgets and the order, should arranges in DFS order
    ASSERT_EQ("6", visits2.at(targets2.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, nestingUsageDetect)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
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
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    auto rearMatcher = WidgetMatchModel(UiAttr::TYPE, "Button", EQ);
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

TEST_F(WidgetSelectorTest, singleParentLocatorAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, singleParentLocatorAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text End", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("15", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, singleParentLocatorAndExistMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    selector.SetWantMulti(true);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(2, targets.size());
    ASSERT_EQ("14", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("15", visits.at(targets.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, singleParentLocatorAndExistMultiAnchor)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::TYPE, "Image", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    selector.SetWantMulti(true);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    // both of "Use USB to" and "Transfer photos" are between "id3" and "id9", so two widgets should be selected
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("4", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, parentAndParentLocatorsAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text End", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto parentMatcher1 = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "5", EQ);
    auto parentLocator1 = WidgetSelector();
    parentLocator1.AddMatcher(parentMatcher1);
    selector.AddParentLocator(parentLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, parentAndParentLocatorsAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto parentMatcher1 = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "5", EQ);
    auto parentLocator1 = WidgetSelector();
    parentLocator1.AddMatcher(parentMatcher1);
    selector.AddParentLocator(parentLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, parentAndParentLocatorsAndExistsMultiTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto parentMatcher1 = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "5", EQ);
    auto parentLocator1 = WidgetSelector();
    parentLocator1.AddMatcher(parentMatcher1);
    selector.AddParentLocator(parentLocator1, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(2, targets.size());
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("8", visits.at(targets.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, frontAndParentLocatorsAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto frontMatcher = WidgetMatchModel(UiAttr::TYPE, "Scroll", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    selector.SetWantMulti(true);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);

    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, frontAndParentLocatorsAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, frontAndParentLocatorsAndExistsMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto frontMatcher = WidgetMatchModel(UiAttr::TYPE, "Image", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    selector.SetWantMulti(true);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(2, targets.size());
    ASSERT_EQ("4", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("6", visits.at(targets.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, rearAndParentLocatorsAndExistsNoTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "11", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, rearAndParentLocatorsAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("4", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, rearAndParentLocatorsAndExistsMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", CONTAINS);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text Two", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);
    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("4", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));

    selector.SetWantMulti(true);
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    selector.Select(w1, *iterator.get(), visits2, targets2);
    ASSERT_EQ(2, targets2.size());
    ASSERT_EQ("4", visits2.at(targets2.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("6", visits2.at(targets2.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, frontAndrearAndParentLocatorsAndExistsNoTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "4", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text Two", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(0, targets.size());
}

TEST_F(WidgetSelectorTest, frontAndrearAndParentLocatorsAndExistsSingleTarget)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text Two", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto frontMatcher = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("6", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetSelectorTest, frontAndrearAndParentLocatorsAndExistsMultiTargets)
{
    ApiCallErr err(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text One", EQ);
    selector.AddMatcher(matcher0);

    auto parentMatcher = WidgetMatchModel(UiAttr::ACCESSIBILITY_ID, "2", EQ);
    auto parentLocator = WidgetSelector();
    parentLocator.AddMatcher(parentMatcher);
    selector.AddParentLocator(parentLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto rearMatcher = WidgetMatchModel(UiAttr::TEXT, "Text Two", EQ);
    auto rearLocator = WidgetSelector();
    rearLocator.AddMatcher(rearMatcher);
    selector.AddRearLocator(rearLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    auto frontMatcher = WidgetMatchModel(UiAttr::TYPE, "Image", EQ);
    auto frontLocator = WidgetSelector();
    frontLocator.AddMatcher(frontMatcher);
    selector.AddFrontLocator(frontLocator, err);
    ASSERT_EQ(NO_ERROR, err.code_);

    std::vector<Widget> visits;
    std::vector<int> targets;
    selector.Select(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(1, targets.size());
    ASSERT_EQ("4", visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));

    selector.SetWantMulti(true);
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    selector.Select(w1, *iterator.get(), visits2, targets2);
    ASSERT_EQ(2, targets2.size());
    ASSERT_EQ("4", visits2.at(targets2.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID));
    ASSERT_EQ("6", visits2.at(targets2.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID));
}