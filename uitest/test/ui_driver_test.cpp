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
#include "ui_driver.h"
#include "ui_model.h"
#include "mock_element_node_iterator.h"
#include "mock_controller.h"

using namespace OHOS::uitest;
using namespace std;

// test fixture

class UiDriverTest : public testing::Test {
protected:
    void SetUp() override
    {
        auto mockController = make_unique<MockController>();
        controller_ = mockController.get();
        UiDriver::RegisterController(move(mockController));
        driver_ = make_unique<UiDriver>();
    }

    void TearDown() override
    {
        controller_ = nullptr;
    }

    MockController *controller_ = nullptr;
    unique_ptr<UiDriver> driver_ = nullptr;
    UiOpArgs opt_;

    ~UiDriverTest() override = default;
};

TEST_F(UiDriverTest, normalInteraction)
{
    std::string nodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test1",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                    "attributes":{
                    "windowId":"12",
                    "bundleName":"test1",
                    "componentType":"Text",
                    "accessibilityId":"100",
                    "content":"USB",
                    "rectInScreen":"30,60,10,20"
                    },
                    "children":[]
                }
            ]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles = MockElementNodeIterator::ConstructIteratorByJson(nodeJson)->elementInfoLists_;

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};

    controller_->AddWindowsAndNode(w1, eles);

    auto error = ApiCallErr(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "USB", EQ);
    selector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    selector.SetWantMulti(false);
    driver_->FindWidgets(selector, widgets, error, true);

    ASSERT_EQ(1, widgets.size());
    // perform interactions
    auto key = Back();
    driver_->TriggerKey(key, opt_, error);
    ASSERT_EQ(NO_ERROR, error.code_);
}

TEST_F(UiDriverTest, retrieveWidget)
{
    std::string nodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test1",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                    "attributes":{
                    "windowId":"12",
                    "bundleName":"test1",
                    "componentType":"Text",
                    "accessibilityId":"100",
                    "content":"USB",
                    "rectInScreen":"30,60,10,20"
                    },
                    "children":[]
                }
            ]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles =
        MockElementNodeIterator::ConstructIteratorByJson(nodeJson)->elementInfoLists_;

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};

    controller_->AddWindowsAndNode(w1, eles);

    auto error = ApiCallErr(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "USB", EQ);
    selector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    selector.SetWantMulti(false);
    driver_->FindWidgets(selector, widgets, error, true);

    ASSERT_EQ(1, widgets.size());

    // mock another dom on which the target widget is missing, and perform click

    std::vector<MockAccessibilityElementInfo> eles_ =
        MockElementNodeIterator::ConstructIteratorByJson(nodeJson)->elementInfoLists_;
    controller_->AddWindowsAndNode(w1, eles_);
    error = ApiCallErr(NO_ERROR);
    auto ret = driver_->RetrieveWidget(*widgets.at(0), error, true);

    // retrieve widget failed should be marked as exception
    ASSERT_EQ(NO_ERROR, error.code_);
    ASSERT_TRUE(ret->GetAttr(UiAttr::TEXT) == "USB");
}

TEST_F(UiDriverTest, retrieveWidgetFailure)
{
    std::string beforeNodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test1",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                    "attributes":{
                    "windowId":"12",
                    "bundleName":"test1",
                    "componentType":"Text",
                    "accessibilityId":"100",
                    "content":"USB",
                    "rectInScreen":"30,60,10,20"
                    },
                    "children":[]
                }
            ]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles =
        MockElementNodeIterator::ConstructIteratorByJson(beforeNodeJson)->elementInfoLists_;

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};

    controller_->AddWindowsAndNode(w1, eles);

    auto error = ApiCallErr(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher = WidgetMatchModel(UiAttr::TEXT, "USB", EQ);
    selector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    selector.SetWantMulti(false);
    driver_->FindWidgets(selector, widgets, error, true);

    ASSERT_EQ(1, widgets.size());

    // mock another dom on which the target widget is missing, and perform click
    std::string afterNodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test1",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                    "attributes":{
                    "windowId":"12",
                    "bundleName":"test1",
                    "componentType":"Text",
                    "accessibilityId":"101",
                    "content":"USB",
                    "rectInScreen":"30,60,10,20"
                    },
                    "children":[]
                }
            ]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles_ =
        MockElementNodeIterator::ConstructIteratorByJson(afterNodeJson)->elementInfoLists_;

    controller_->RemoveWindowsAndNode(w1);
    controller_->AddWindowsAndNode(w1, eles_);
    error = ApiCallErr(NO_ERROR);
    auto ret = driver_->RetrieveWidget(*widgets.at(0), error, true);

    // retrieve widget failed should be marked as exception
    ASSERT_EQ(ERR_COMPONENT_LOST, error.code_);
    ASSERT_TRUE(error.message_.find(selector.Describe()) != string::npos)
        << "Error message should contains the widget selection description";
}

TEST_F(UiDriverTest, FindWindowByBundleName)
{
    std::string window1NodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test12",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles =
        MockElementNodeIterator::ConstructIteratorByJson(window1NodeJson)->elementInfoLists_;

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.bounds_ = Rect{0, 100, 0, 120};
    controller_->AddWindowsAndNode(w1, eles);
    std::string window2NodeJson = R"(
        {
            "attributes":{
                "windowId":"123",
				"bundleName":"test123",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles_2 =
        MockElementNodeIterator::ConstructIteratorByJson(window2NodeJson)->elementInfoLists_;

    Window w2{123};
    w2.windowLayer_ = 4;
    w2.bounds_ = Rect{0, 100, 0, 120};

    controller_->AddWindowsAndNode(w2, eles_2);

    auto error = ApiCallErr(NO_ERROR);

    json filterBundle;
    filterBundle["bundleName"] = "test12";
    auto windowMatcher = [&filterBundle](const Window &window) -> bool {
        bool match = true;
        match = match && window.bundleName_ == filterBundle["bundleName"].get<string>();
        return match;
    };
    auto winPtr = driver_->FindWindow(windowMatcher, true, error);

    ASSERT_TRUE(winPtr != nullptr);
    ASSERT_EQ(winPtr->id_, 12);
}

TEST_F(UiDriverTest, FindWindowByActived)
{
    std::string window1NodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test12",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles =
        MockElementNodeIterator::ConstructIteratorByJson(window1NodeJson)->elementInfoLists_;

    Window w1{12};
    w1.windowLayer_ = 2;
    w1.actived_ = false;
    w1.bounds_ = Rect{0, 100, 0, 120};
    controller_->AddWindowsAndNode(w1, eles);
    std::string window2NodeJson = R"(
        {
            "attributes":{
                "windowId":"123",
				"bundleName":"test123",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[]
        }
    )";

    std::vector<MockAccessibilityElementInfo> eles_2 =
        MockElementNodeIterator::ConstructIteratorByJson(window1NodeJson)->elementInfoLists_;

    Window w2{123};
    w2.windowLayer_ = 4;
    w2.actived_ = true;
    w2.bounds_ = Rect{0, 100, 0, 120};

    controller_->AddWindowsAndNode(w2, eles_2);

    auto error = ApiCallErr(NO_ERROR);

    json filterActive;
    filterActive["actived"] = true;

    auto windowMatcher = [&filterActive](const Window &window) -> bool {
        bool match = true;
        match = match && window.actived_ == filterActive["actived"].get<bool>();

        return match;
    };
    auto winPtr = driver_->FindWindow(windowMatcher, false, error);

    ASSERT_TRUE(winPtr != nullptr);
    ASSERT_EQ(winPtr->id_, 123);
}