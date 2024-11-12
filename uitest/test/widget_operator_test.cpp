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
#include "gtest/gtest.h"
#include "widget_operator.h"
#include "mock_element_node_iterator.h"
#include "mock_controller.h"

using namespace OHOS::uitest;
using namespace std;

unique_ptr<PointerMatrix> MockController::touch_event_records_ = nullptr;

class WidgetOperatorTest : public testing::Test {
protected:
    void SetUp() override
    {
        auto mockController = std::make_unique<MockController>();
        controller_ = mockController.get();
        UiDriver::RegisterController(std::move(mockController));
        driver_ = make_unique<UiDriver>();
        ResetWindowAndNode(w1_, eles_);
    }

    // Reset Window and node
    void ResetWindowAndNode(Window &w, std::vector<MockAccessibilityElementInfo> &eles)
    {
        std::string domJson = R"(
            {
                "attributes":{
                    "windowId":"12",
                    "componentType":"List",
                    "accessibilityId":"1",
                    "content":"Text List",
                    "rectInScreen":"0,1000,0,1200"
                },
                "children":[
                    {
                        "attributes":{
                            "windowId":"12",
                            "accessibilityId":"2",
                            "componentType":"Scroll",
                            "content":"Text List Scroll",
                            "rectInScreen":"0,600,0,1000"
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
                            "componentType":"List",
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
        eles_ = MockElementNodeIterator::ConstructIteratorByJson(domJson)->elementInfoLists_;
        w.windowLayer_ = 2;
        w.bounds_ = Rect{0, 1000, 0, 1200};
        w.bundleName_ = "test1";
    }
    void TearDown() override
    {
        controller_ = nullptr;
    }

    MockController *controller_ = nullptr;
    unique_ptr<UiDriver> driver_ = nullptr;
    UiOpArgs opt_;
    Window w1_{12};
    std::vector<MockAccessibilityElementInfo> eles_;
    ~WidgetOperatorTest() override = default;
};

TEST_F(WidgetOperatorTest, scrollSearchNoTarget)
{
    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "Text List Scroll", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    controller_->AddWindowsAndNode(w1_, eles_);
    scrollWidgetSelector.SetWantMulti(false);
    driver_->FindWidgets(scrollWidgetSelector, widgets, error, true);

    ASSERT_EQ(1, widgets.size());

    // mock another dom on which the scroll-widget is missing, and perform scroll-search

    auto targetWidgetSelector = WidgetSelector();
    auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text End", EQ);
    targetWidgetSelector.AddMatcher(matcher0);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
}

TEST_F(WidgetOperatorTest, scrollSearchWithTarget)
{
    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "Text List Scroll", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    controller_->AddWindowsAndNode(w1_, eles_);
    scrollWidgetSelector.SetWantMulti(false);
    driver_->FindWidgets(scrollWidgetSelector, widgets, error, true);

    ASSERT_EQ(1, widgets.size());
    auto targetWidgetSelector = WidgetSelector();
    auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
    auto matcher0 = WidgetMatchModel(UiAttr::TEXT, "Text List12", EQ);
    targetWidgetSelector.AddMatcher(matcher0);
    auto rev = wOp.ScrollFindWidget(targetWidgetSelector, error);
    ASSERT_FALSE(rev == nullptr);
    ASSERT_EQ("5", rev->GetAttr(UiAttr::ACCESSIBILITY_ID));
}

TEST_F(WidgetOperatorTest, scrollSearchCheckSubjectWidget)
{
    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "Text List Scroll", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> images;
    controller_->AddWindowsAndNode(w1_, eles_);
    scrollWidgetSelector.SetWantMulti(false);
    driver_->FindWidgets(scrollWidgetSelector, images, error, true);

    ASSERT_EQ(1, images.size());

    auto targetWidgetMatcher = WidgetMatchModel(UiAttr::TEXT, "Text End", EQ);
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);
    opt_.scrollWidgetDeadZone_ = 0; // set deadzone to 0 for easy computation
    auto wOp = WidgetOperator(*driver_, *images.at(0), opt_);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
    // check the scroll action events, should be acted on the subject node specified by WidgetMatcher
    ASSERT_TRUE(!MockController::touch_event_records_->Empty());
    auto &firstEvent = MockController::touch_event_records_->At(0, 0);
    auto fin = MockController::touch_event_records_->GetFingers() - 1;
    auto ste = MockController::touch_event_records_->GetSteps() - 1;
    auto &lastEvent = MockController::touch_event_records_->At(fin, ste);
    // check scroll event pointer_x
    int32_t subjectCx = (0 + 600) / 2;
    ASSERT_NEAR(firstEvent.point_.px_, subjectCx, 5);
    ASSERT_NEAR(lastEvent.point_.px_, subjectCx, 5);

    // check scroll event pointer_y
    constexpr int32_t subjectWidgetHeight = 1000 - 0;
    int32_t maxCy = 0;
    int32_t minCy = 1E5;
    for (uint32_t finger = 0; finger < MockController::touch_event_records_->GetFingers(); finger++) {
        for (uint32_t step = 0; step < MockController::touch_event_records_->GetSteps(); step++) {
            if (MockController::touch_event_records_->At(finger, step).point_.py_ > maxCy) {
                maxCy = MockController::touch_event_records_->At(finger, step).point_.py_;
            }
            if (MockController::touch_event_records_->At(finger, step).point_.py_ < minCy) {
                minCy = MockController::touch_event_records_->At(finger, step).point_.py_;
            }
        }
    }

    int32_t scrollDistanceY = maxCy - minCy;
    ASSERT_TRUE(abs(scrollDistanceY - subjectWidgetHeight) < 5);
}

TEST_F(WidgetOperatorTest, scrollSearchCheckDirection)
{
    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "Text List Scroll", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    controller_->AddWindowsAndNode(w1_, eles_);
    scrollWidgetSelector.SetWantMulti(false);
    driver_->FindWidgets(scrollWidgetSelector, widgets, error, true);
    ASSERT_EQ(1, widgets.size());

    error = ApiCallErr(NO_ERROR);
    auto targetWidgetMatcher = WidgetMatchModel(UiAttr::TEXT, "Text End", EQ);
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);
    opt_.scrollWidgetDeadZone_ = 0; // set deadzone to 0 for easy computation
    auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
    // check the scroll action events, should be acted on the specified node
    ASSERT_TRUE(!MockController::touch_event_records_->Empty());
    // should scroll-search upward (cy_from<cy_to) then downward (cy_from>cy_to)
    uint32_t maxCyEventIndex = 0;
    uint32_t index = 0;
    for (uint32_t event = 0; event < MockController::touch_event_records_->GetSize() - 1; event++) {
        if (MockController::touch_event_records_->At(0, event).point_.py_ >
            MockController::touch_event_records_->At(0, maxCyEventIndex).point_.py_) {
            maxCyEventIndex = index;
        }
        index++;
    }

    for (uint32_t idx = 0; idx < MockController::touch_event_records_->GetSize() - 1; idx++) {
        if (idx < maxCyEventIndex) {
            ASSERT_LT(MockController::touch_event_records_->At(0, idx).point_.py_,
                      MockController::touch_event_records_->At(0, idx + 1).point_.py_);
        } else if (idx > maxCyEventIndex) {
            ASSERT_GT(MockController::touch_event_records_->At(0, idx).point_.py_,
                      MockController::touch_event_records_->At(0, idx + 1).point_.py_);
        }
    }
}