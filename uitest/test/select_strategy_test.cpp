/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "mock_element_node_iterator.h"
#define private public
#define protected public
#include "select_strategy.h"
#undef private
#undef protected
#include "ui_controller.h"
#include "ui_action.h"

using namespace OHOS::uitest;
using namespace std;

TEST(SelectStrategyTest, refreshWidgetBoundsParentAndChild)
{
    std::vector<WidgetMatchModel> emptyMatcher;
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = emptyMatcher;
    std::unique_ptr<SelectStrategy> plain = SelectStrategy::BuildSelectStrategy(buildParam, false);
    Rect selectWindow{10, 100, 10, 300};
    Rect layer1{10, 50, 10, 40};
    Rect layer2{10, 20, 50, 100};
    std::vector<Rect> overlay;
    overlay.emplace_back(layer1);
    overlay.emplace_back(layer2);
    plain->SetAndCalcSelectWindowRect(selectWindow, overlay);
    Widget widget{"test"};
    Rect widOriBounds{30, 30, 30, 60};
    widget.SetBounds(widOriBounds);
    widget.SetAttr(UiAttr::VISIBLE, "true");
    widget.SetAttr(UiAttr::TYPE, "Scroll");
    plain->RefreshWidgetBounds(widget);
    ASSERT_EQ(widget.GetAttr(UiAttr::VISIBLE), "false");
    auto rect = widget.GetBounds();
    ASSERT_EQ(rect.left_, 0);
    ASSERT_EQ(rect.right_, 0);
    ASSERT_EQ(rect.top_, 0);
    ASSERT_EQ(rect.bottom_, 0);
}

TEST(SelectStrategyTest, refreshWidgetBoundsOver)
{
    std::vector<WidgetMatchModel> emptyMatcher;
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = emptyMatcher;
    std::unique_ptr<SelectStrategy> plain = SelectStrategy::BuildSelectStrategy(buildParam, false);
    Rect selectWindow{10, 100, 10, 300};
    Rect layer1{10, 50, 10, 40};
    Rect layer2{10, 20, 50, 100};
    std::vector<Rect> overlay;
    overlay.emplace_back(layer1);
    overlay.emplace_back(layer2);
    plain->SetAndCalcSelectWindowRect(selectWindow, overlay);
    Widget widget{"test"};
    Rect widOriBounds{20, 50, 30, 40};
    widget.SetBounds(widOriBounds);
    plain->RefreshWidgetBounds(widget);
    ASSERT_EQ(widget.GetAttr(UiAttr::VISIBLE), "false");
    auto rect = widget.GetBounds();
    ASSERT_EQ(rect.left_, 0);
    ASSERT_EQ(rect.right_, 0);
    ASSERT_EQ(rect.top_, 0);
    ASSERT_EQ(rect.bottom_, 0);
}

TEST(SelectStrategyTest, refreshWidgetBoundsPartOver)
{
    std::vector<WidgetMatchModel> emptyMatcher;
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = emptyMatcher;
    std::unique_ptr<SelectStrategy> plain = SelectStrategy::BuildSelectStrategy(buildParam, false);
    Rect selectWindow{10, 100, 10, 300};
    Rect layer1{10, 50, 10, 40};
    Rect layer2{10, 20, 50, 100};
    std::vector<Rect> overlay;
    overlay.emplace_back(layer1);
    overlay.emplace_back(layer2);
    plain->SetAndCalcSelectWindowRect(selectWindow, overlay);
    Widget widget{"test"};
    Rect widOriBounds{20, 50, 30, 50};
    widget.SetBounds(widOriBounds);
    widget.SetAttr(UiAttr::VISIBLE, "true");
    plain->RefreshWidgetBounds(widget);
    ASSERT_EQ(widget.GetAttr(UiAttr::VISIBLE), "true");
    auto rect = widget.GetBounds();
    ASSERT_EQ(rect.left_, 20);
    ASSERT_EQ(rect.right_, 50);
    ASSERT_EQ(rect.top_, 40);
    ASSERT_EQ(rect.bottom_, 50);
}

void ConstructIterator(std::unique_ptr<ElementNodeIterator> &iterator)
{
    std::string afterDom = R"(
        {
            "attributes":{
                "windowId":"12",
                "componentType":"List",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                     "attributes":{
                        "windowId":"12",
                        "componentType":"List",
                        "content":"Text List",
                        "rectInScreen":"30,60,10,120"
                     },
                     "children":[
                        {
                            "attributes":{
                                "windowId":"12",
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
                                "accessibilityId":"7",
                                "content":"Text List12",
                                "rectInScreen":"40,60,10,20"
                            },
                            "children":[
                                {
                                    "attributes":{
                                        "windowId":"12",
                                        "accessibilityId":"5",
                                        "componentType":"Text",
                                        "content":"Text One",
                                        "rectInScreen":"40,50,10,20"
                                    },
                                    "children":[]
                                },
                                {
                                    "attributes":{
                                        "windowId":"12",
                                        "accessibilityId":"4",
                                        "componentType":"Text",
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
                        "componentType":"Image",
                        "content":"Button",
                        "rectInScreen":"10,20,20,100"
                     },
                     "children":[]
                },
                {
                     "attributes":{
                        "windowId":"12",
                        "componentType":"Scroll",
                        "content":"Button",
                        "rectInScreen":"20,100,20,100"
                     },
                     "children":[
                        {
                            "attributes":{
                                "windowId":"12",
                                "componentType":"Image",
                                "content":"",
                                "rectInScreen":"20,100,20,100"
                            },
                            "children":[]
                        },
                        {
                            "attributes":{
                                "windowId":"12",
                                "componentType":"Button",
                                "accessibilityId":"11",
                                "content":"Button text2",
                                "focused":"true",
                                "rectInScreen":"20,100,20,100"
                            },
                            "children":[
                                {
                                    "attributes":{
                                        "windowId":"12",
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
    iterator = MockElementNodeIterator::ConstructIteratorByJson(afterDom);
}

TEST(SelectStrategyTest, afterStrategyForAnchorIsBeforeTarget)
{
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel anchor1{UiAttr::TEXT, "test", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor1);
    WidgetMatchModel sef1{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef1);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> afterStrategy = SelectStrategy::BuildSelectStrategy(buildParam, false);
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    afterStrategy->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(visits.size(), 12);
    ASSERT_EQ(targets.size(), 0);
}
TEST(SelectStrategyTest, afterStrategyForAnchorIsAfterSingleTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    WidgetMatchModel anchor2{UiAttr::TEXT, "Text List12", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor2);
    WidgetMatchModel sef2{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef2);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> afterStrategy2 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    afterStrategy2->LocateNode(w1, *iterator.get(), visits2, targets2);
    ASSERT_EQ(targets2.size(), 1);
    ASSERT_EQ(visits2[targets2[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "5");
}

TEST(SelectStrategyTest, afterStrategyForAnchorIsAfterMultiTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel anchor3{UiAttr::TEXT, "Text List", ValueMatchPattern::EQ};
    WidgetMatchModel sef3{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor3);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> afterStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    afterStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3);
    ASSERT_EQ(visits3.size(), 12);
    ASSERT_EQ(targets3.size(), 2);
    ASSERT_EQ(visits3[targets3[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "4");
    ASSERT_EQ(visits3[targets3[1]].GetAttr(UiAttr::ACCESSIBILITY_ID), "5");
}

TEST(SelectStrategyTest, beforeStrategyForAnchorIsAfterTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel anchor1{UiAttr::TEXT, "test", ValueMatchPattern::EQ};
    WidgetMatchModel sef1{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor1);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef1);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.beforeAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> beforeStrategy = SelectStrategy::BuildSelectStrategy(buildParam, false);
    beforeStrategy->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(visits.size(), 12);
    ASSERT_EQ(targets.size(), 0);
}
TEST(SelectStrategyTest, beforeStrategyForAnchorIsBeforeSingleTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    WidgetMatchModel anchor2{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    WidgetMatchModel sef2{UiAttr::TEXT, "Text List12", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor2);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef2);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.beforeAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> beforeStrategy2 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    beforeStrategy2->LocateNode(w1, *iterator.get(), visits2, targets2);
    ASSERT_EQ(targets2.size(), 1);
    ASSERT_EQ(visits2[targets2[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "7");
}
TEST(SelectStrategyTest, beforeStrategyForAnchorIsBeforeMultiTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel anchor3{UiAttr::TEXT, "Text End", ValueMatchPattern::EQ};
    WidgetMatchModel sef3{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor3);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.beforeAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> beforeStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    beforeStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3, false);
    ASSERT_EQ(visits3.size(), 12);
    ASSERT_EQ(targets3.size(), 2);
    ASSERT_EQ(visits3[targets3[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "4");
    ASSERT_EQ(visits3[targets3[1]].GetAttr(UiAttr::ACCESSIBILITY_ID), "5");
}

TEST(SelectStrategyTest, beforeStrategyForAnchorIsTargetForMultiTargets)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel anchor3{UiAttr::TEXT, "Text End", ValueMatchPattern::EQ};
    WidgetMatchModel sef3{UiAttr::TYPE, "Text", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor3);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.beforeAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> beforeStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    beforeStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3, false);
    ASSERT_EQ(visits3.size(), 12);
    ASSERT_EQ(targets3.size(), 3);
    ASSERT_EQ(visits3[targets3[0]].GetAttr(UiAttr::TEXT), "Text One");
    ASSERT_EQ(visits3[targets3[1]].GetAttr(UiAttr::TEXT), "Text One");
    ASSERT_EQ(visits3[targets3[2]].GetAttr(UiAttr::TEXT), "");
}

TEST(SelectStrategyTest, beforeStrategyForAnchorIsTargetForSingleTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel anchor3{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    WidgetMatchModel sef3{UiAttr::TYPE, "Image", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor3);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.beforeAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> beforeStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    beforeStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3, false);
    ASSERT_EQ(visits3.size(), 12);
    ASSERT_EQ(targets3.size(), 1);
    ASSERT_EQ(visits3[targets3[0]].GetAttr(UiAttr::TEXT), "Button");
}

TEST(SelectStrategyTest, withInStrategyForAnchorIsOutTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel anchor1{UiAttr::TEXT, "test", ValueMatchPattern::EQ};
    WidgetMatchModel sef1{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor1);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef1);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.withInAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> withInStrategy = SelectStrategy::BuildSelectStrategy(buildParam, false);

    withInStrategy->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(visits.size(), 12);
    ASSERT_EQ(targets.size(), 0);
}
TEST(SelectStrategyTest, withInStrategyForOneAnchorIsInTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    WidgetMatchModel anchor2{UiAttr::TEXT, "Text List12", ValueMatchPattern::EQ};
    WidgetMatchModel sef2{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor2);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef2);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.withInAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> withInStrategy2 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    withInStrategy2->LocateNode(w1, *iterator.get(), visits2, targets2);
    ASSERT_EQ(targets2.size(), 1);
    ASSERT_EQ(visits2[targets2[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "5");
}
TEST(SelectStrategyTest, withInStrategyFoTwoAnchorIsInTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel anchor3{UiAttr::TEXT, "Button", ValueMatchPattern::EQ};
    WidgetMatchModel sef3{UiAttr::TEXT, "Button text2", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor3);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.withInAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> withInStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    withInStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3);
    ASSERT_EQ(targets3.size(), 1);
    ASSERT_EQ(visits3[targets3[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "11");
}

TEST(SelectStrategyTest, withInStrategyFoTwoAnchorIsInMultiTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel anchor3{UiAttr::TEXT, "Text List", ValueMatchPattern::EQ};
    WidgetMatchModel sef3{UiAttr::TEXT, "Button", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    anchorMatchers.emplace_back(anchor3);
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.withInAnchorMatcherVec.emplace_back(anchorMatchers);
    std::unique_ptr<SelectStrategy> withInStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    withInStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3);
    ASSERT_EQ(targets3.size(), 3);
    ASSERT_EQ(visits3[targets3[0]].GetAttr(UiAttr::TYPE), "Image");
    ASSERT_EQ(visits3[targets3[1]].GetAttr(UiAttr::TYPE), "Image");
    ASSERT_EQ(visits3[targets3[2]].GetAttr(UiAttr::TYPE), "Scroll");
}

TEST(SelectStrategyTest, plainStrategyForOnlyOneTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef1{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef1);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    std::unique_ptr<SelectStrategy> plainStrategy = SelectStrategy::BuildSelectStrategy(buildParam, false);
    plainStrategy->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(visits[targets[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "4");
}
TEST(SelectStrategyTest, plainStrategyForMultiTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<Widget> visits2;
    std::vector<int> targets2;
    WidgetMatchModel sef2{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef2);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    std::unique_ptr<SelectStrategy> plainStrategy2 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    plainStrategy2->LocateNode(w1, *iterator.get(), visits2, targets2);
    ASSERT_EQ(targets2.size(), 2);
    ASSERT_EQ(visits2[targets2[0]].GetAttr(UiAttr::ACCESSIBILITY_ID), "4");
    ASSERT_EQ(visits2[targets2[1]].GetAttr(UiAttr::ACCESSIBILITY_ID), "5");
}
TEST(SelectStrategyTest, plainStrategyForNoneTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits3;
    std::vector<int> targets3;
    WidgetMatchModel sef3{UiAttr::TEXT, "Button text2112", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> anchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef3);
    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    std::unique_ptr<SelectStrategy> plainStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    plainStrategy3->LocateNode(w1, *iterator.get(), visits3, targets3);
    ASSERT_EQ(visits3.size(), 12);
    ASSERT_EQ(targets3.size(), 0);
}

TEST(SelectStrategyTest, complexStrategyForAfterAndBeforeNoneTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef{UiAttr::TEXT, "Text End", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> beforeAnchorMatchers;
    std::vector<WidgetMatchModel> afterAnchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef);
    WidgetMatchModel beforeAnchor{UiAttr::ACCESSIBILITY_ID, "11", ValueMatchPattern::EQ};
    WidgetMatchModel afterAnchor{UiAttr::ACCESSIBILITY_ID, "7", ValueMatchPattern::EQ};
    beforeAnchorMatchers.emplace_back(beforeAnchor);
    afterAnchorMatchers.emplace_back(afterAnchor);

    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(afterAnchorMatchers);
    buildParam.beforeAnchorMatcherVec.emplace_back(beforeAnchorMatchers);
    std::unique_ptr<SelectStrategy> plainStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    plainStrategy3->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 0);
}

TEST(SelectStrategyTest, complexStrategyForAfterAndBeforeSingleTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef{UiAttr::TEXT, "Button", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> beforeAnchorMatchers;
    std::vector<WidgetMatchModel> afterAnchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef);
    WidgetMatchModel afterAnchor{UiAttr::ACCESSIBILITY_ID, "7", ValueMatchPattern::EQ};
    WidgetMatchModel beforeAnchor{UiAttr::ACCESSIBILITY_ID, "11", ValueMatchPattern::EQ};
    beforeAnchorMatchers.emplace_back(beforeAnchor);
    afterAnchorMatchers.emplace_back(afterAnchor);

    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(afterAnchorMatchers);
    buildParam.beforeAnchorMatcherVec.emplace_back(beforeAnchorMatchers);
    std::unique_ptr<SelectStrategy> plainStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    plainStrategy3->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(visits.at(targets.at(0)).GetAttr(UiAttr::TYPE), "Image");
}

TEST(SelectStrategyTest, complexStrategyForAfterAndBeforeMultiTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef{UiAttr::TEXT, "Button", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> beforeAnchorMatchers;
    std::vector<WidgetMatchModel> afterAnchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef);
    WidgetMatchModel afterAnchor{UiAttr::ACCESSIBILITY_ID, "7", ValueMatchPattern::EQ};
    WidgetMatchModel beforeAnchor{UiAttr::ACCESSIBILITY_ID, "11", ValueMatchPattern::EQ};
    beforeAnchorMatchers.emplace_back(beforeAnchor);
    afterAnchorMatchers.emplace_back(afterAnchor);

    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(afterAnchorMatchers);
    buildParam.beforeAnchorMatcherVec.emplace_back(beforeAnchorMatchers);
    std::unique_ptr<SelectStrategy> plainStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, true);
    plainStrategy3->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 2);
    ASSERT_EQ(visits.at(targets.at(0)).GetAttr(UiAttr::TYPE), "Image");
    ASSERT_EQ(visits.at(targets.at(1)).GetAttr(UiAttr::TYPE), "Scroll");
}

TEST(SelectStrategyTest, complexStrategyForAfterAndWithInNoneTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef{UiAttr::TEXT, "Text End", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> withInAnchorMatchers;
    std::vector<WidgetMatchModel> afterAnchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef);
    WidgetMatchModel withInAnchor{UiAttr::ACCESSIBILITY_ID, "7", ValueMatchPattern::EQ};
    WidgetMatchModel afterAnchor{UiAttr::ACCESSIBILITY_ID, "11", ValueMatchPattern::EQ};
    withInAnchorMatchers.emplace_back(withInAnchor);
    afterAnchorMatchers.emplace_back(afterAnchor);

    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(afterAnchorMatchers);
    buildParam.withInAnchorMatcherVec.emplace_back(withInAnchorMatchers);
    std::unique_ptr<SelectStrategy> plainStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    plainStrategy3->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 0);
}

TEST(SelectStrategyTest, complexStrategyForAfterAndWithInSingleTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef{UiAttr::TYPE, "Text", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> withInAnchorMatchers;
    std::vector<WidgetMatchModel> afterAnchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef);
    WidgetMatchModel withInAnchor{UiAttr::ACCESSIBILITY_ID, "7", ValueMatchPattern::EQ};
    WidgetMatchModel afterAnchor{UiAttr::ACCESSIBILITY_ID, "5", ValueMatchPattern::EQ};
    withInAnchorMatchers.emplace_back(withInAnchor);
    afterAnchorMatchers.emplace_back(afterAnchor);

    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(afterAnchorMatchers);
    buildParam.withInAnchorMatcherVec.emplace_back(withInAnchorMatchers);
    std::unique_ptr<SelectStrategy> plainStrategy3 = SelectStrategy::BuildSelectStrategy(buildParam, false);
    plainStrategy3->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID), "4");
}

TEST(SelectStrategyTest, complexStrategyForAfterAndWithInMultiTarget)
{
    std::unique_ptr<ElementNodeIterator> iterator = nullptr;
    ConstructIterator(iterator);

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    std::vector<Widget> visits;
    std::vector<int> targets;
    WidgetMatchModel sef{UiAttr::TYPE, "Text", ValueMatchPattern::EQ};
    std::vector<WidgetMatchModel> withInAnchorMatchers;
    std::vector<WidgetMatchModel> afterAnchorMatchers;
    std::vector<WidgetMatchModel> myselfMatchers;
    myselfMatchers.emplace_back(sef);
    WidgetMatchModel withInAnchor{UiAttr::ACCESSIBILITY_ID, "7", ValueMatchPattern::EQ};
    WidgetMatchModel afterAnchor{UiAttr::TEXT, "Text One", ValueMatchPattern::EQ};
    withInAnchorMatchers.emplace_back(withInAnchor);
    afterAnchorMatchers.emplace_back(afterAnchor);

    StrategyBuildParam buildParam;
    buildParam.myselfMatcher = myselfMatchers;
    buildParam.afterAnchorMatcherVec.emplace_back(afterAnchorMatchers);
    buildParam.withInAnchorMatcherVec.emplace_back(withInAnchorMatchers);
    std::unique_ptr<SelectStrategy> complexStrategy = SelectStrategy::BuildSelectStrategy(buildParam, true);
    complexStrategy->LocateNode(w1, *iterator.get(), visits, targets);
    ASSERT_EQ(targets.size(), 2);
    ASSERT_EQ(visits.at(targets.at(0)).GetAttr(UiAttr::ACCESSIBILITY_ID), "5");
    ASSERT_EQ(visits.at(targets.at(1)).GetAttr(UiAttr::ACCESSIBILITY_ID), "4");
}
