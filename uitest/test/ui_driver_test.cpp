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
    driver_->RetrieveWidget(*widgets.at(0), error, true);

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
    w1.bundleName_ = "test12";
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
    w2.bundleName_ = "test123";
    controller_->AddWindowsAndNode(w2, eles_2);

    auto error = ApiCallErr(NO_ERROR);

    json filterBundle;
    filterBundle["bundleName"] = "test12";
    auto windowMatcher = [&filterBundle](const Window &window) -> bool {
        bool match = true;
        match = match && window.bundleName_ == filterBundle["bundleName"].get<string>();
        return match;
    };
    auto winPtr = driver_->FindWindow(windowMatcher, error);

    ASSERT_TRUE(winPtr != nullptr);
    ASSERT_EQ(winPtr->id_, 12);

    json filterBundleNotFind;
    filterBundleNotFind["bundleName"] = "test12456";
    auto windowMatcher2 = [&filterBundleNotFind](const Window &window) -> bool {
        bool match = true;
        match = match && window.bundleName_ == filterBundleNotFind["bundleName"].get<string>();
        return match;
    };
    auto winPtrNull = driver_->FindWindow(windowMatcher2, error);
    ASSERT_TRUE(winPtrNull == nullptr);
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
    w1.bundleName_ = "test12";
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
    w2.actived_ = true;
    w2.bounds_ = Rect{0, 100, 0, 120};
    w2.bundleName_ = "test123";
    controller_->AddWindowsAndNode(w2, eles_2);

    auto error = ApiCallErr(NO_ERROR);

    json filterActive;
    filterActive["actived"] = true;

    auto windowMatcher = [&filterActive](const Window &window) -> bool {
        bool match = true;
        match = match && window.actived_ == filterActive["actived"].get<bool>();

        return match;
    };
    auto winPtr = driver_->FindWindow(windowMatcher, error);

    ASSERT_TRUE(winPtr != nullptr);
    ASSERT_EQ(winPtr->id_, 123);
}

TEST_F(UiDriverTest, DumpUI)
{
    std::string window2NodeJson = R"(
        {
            "attributes":{
                "windowId":"123",
				"bundleName":"test123",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"List",
                "rectInScreen":"0,200,100,200"
            },
            "children":[
                {
                    "attributes":{
                        "windowId":"123",
                        "bundleName":"test123",
                        "componentType":"Text",
                        "accessibilityId":"2",
                        "content":"Text List",
                        "rectInScreen":"0,200,100,200"
                    },
                    "children":[]
                }
            ]
        }
    )";
    std::vector<MockAccessibilityElementInfo> eles_2 =
        MockElementNodeIterator::ConstructIteratorByJson(window2NodeJson)->elementInfoLists_;
    Window w2{123};
    w2.windowLayer_ = 4;
    w2.actived_ = true;
    w2.bounds_ = Rect{0, 200, 100, 200};
    w2.bundleName_ = "test123";
    controller_->AddWindowsAndNode(w2, eles_2);
    std::string window1NodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test12",
                "componentType":"List",
                "accessibilityId":"4",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                    "attributes":{
                        "windowId":"12",
                        "bundleName":"test12",
                        "componentType":"Text",
                        "accessibilityId":"6",
                        "content":"Text Test",
                        "rectInScreen":"30,60,10,120"
                    },
                    "children":[]
                }
            ]
        }
    )";
    std::vector<MockAccessibilityElementInfo> eles =
        MockElementNodeIterator::ConstructIteratorByJson(window1NodeJson)->elementInfoLists_;
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.invisibleBoundsVec_.emplace_back(w2.bounds_);
    w1.actived_ = false;
    w1.bounds_ = Rect{0, 100, 0, 120};
    w1.bundleName_ = "test12";
    controller_->AddWindowsAndNode(w1, eles);
    auto error = ApiCallErr(NO_ERROR);
    nlohmann::json out;
    DumpOption option;
    driver_->DumpUiHierarchy(out, option, error);
    ASSERT_EQ(out["children"].size(), 2);
    auto window1Json = out["children"][0];
    ASSERT_EQ(window1Json["attributes"]["accessibilityId"], "1");
    ASSERT_EQ(window1Json["attributes"]["text"], "List");
    ASSERT_EQ(window1Json["attributes"]["bundleName"], "test123");
    ASSERT_EQ(window1Json["children"].size(), 1);
    ASSERT_EQ(window1Json["children"][0]["attributes"]["accessibilityId"], "2");
    auto window2Json = out["children"][1];
    ASSERT_EQ(window2Json["attributes"]["accessibilityId"], "4");
    ASSERT_EQ(window2Json["attributes"]["type"], "List");
    ASSERT_EQ(window2Json["attributes"]["bundleName"], "test12");
    ASSERT_EQ(window2Json["attributes"]["text"], "Text List");
    ASSERT_EQ(window2Json["attributes"]["origBounds"], "[30,10][60,120]");
    ASSERT_EQ(window2Json["attributes"]["bounds"], "[30,10][60,100]");
    ASSERT_EQ(window2Json["children"].size(), 1);
    ASSERT_EQ(window2Json["children"][0]["attributes"]["accessibilityId"], "6");
    ASSERT_EQ(window2Json["children"][0]["attributes"]["text"], "Text Test");
    ASSERT_EQ(window2Json["children"][0]["attributes"]["origBounds"], "[30,10][60,120]");
    ASSERT_EQ(window2Json["children"][0]["attributes"]["bounds"], "[30,10][60,100]");
}

TEST_F(UiDriverTest, DumpUI_I)
{
    std::string window2NodeJson = R"(
        {
            "attributes":{
                "windowId":"123",
				"bundleName":"test123",
                "componentType":"List",
                "accessibilityId":"1",
                "content":"List",
                "rectInScreen":"0,200,100,200"
            },
            "children":[
                {
                    "attributes":{
                        "windowId":"123",
                        "bundleName":"test123",
                        "componentType":"Text",
                        "accessibilityId":"2",
                        "content":"Text List",
                        "rectInScreen":"0,200,100,200"
                    },
                    "children":[]
                }
            ]
        }
    )";
    std::vector<MockAccessibilityElementInfo> eles_2 =
        MockElementNodeIterator::ConstructIteratorByJson(window2NodeJson)->elementInfoLists_;
    Window w2{123};
    w2.windowLayer_ = 4;
    w2.actived_ = true;
    w2.bounds_ = Rect{0, 200, 100, 200};
    w2.bundleName_ = "test123";
    controller_->AddWindowsAndNode(w2, eles_2);
    std::string window1NodeJson = R"(
        {
            "attributes":{
                "windowId":"12",
				"bundleName":"test12",
                "componentType":"List",
                "accessibilityId":"4",
                "content":"Text List",
                "rectInScreen":"30,60,10,120"
            },
            "children":[
                {
                    "attributes":{
                        "windowId":"12",
                        "bundleName":"test12",
                        "componentType":"Text",
                        "accessibilityId":"6",
                        "content":"Text Test",
                        "rectInScreen":"30,60,10,120"
                    },
                    "children":[]
                }
            ]
        }
    )";
    std::vector<MockAccessibilityElementInfo> eles =
        MockElementNodeIterator::ConstructIteratorByJson(window1NodeJson)->elementInfoLists_;
    Window w1{12};
    w1.windowLayer_ = 2;
    w1.invisibleBoundsVec_.emplace_back(w2.bounds_);
    w1.actived_ = false;
    w1.bounds_ = Rect{0, 100, 0, 120};
    w1.bundleName_ = "test12";
    controller_->AddWindowsAndNode(w1, eles);
    auto error = ApiCallErr(NO_ERROR);
    nlohmann::json out;
    DumpOption option;
    option.listWindows_ = true;
    driver_->DumpUiHierarchy(out, option, error);
    ASSERT_EQ(out.size(), 2);
    auto window1Json = out[0];
    ASSERT_EQ(window1Json["attributes"]["accessibilityId"], "1");
    ASSERT_EQ(window1Json["attributes"]["text"], "List");
    ASSERT_EQ(window1Json["attributes"]["bundleName"], "test123");
    ASSERT_EQ(window1Json["children"].size(), 1);
    ASSERT_EQ(window1Json["children"][0]["attributes"]["accessibilityId"], "2");
    auto window2Json = out[1];
    ASSERT_EQ(window2Json["attributes"]["accessibilityId"], "4");
    ASSERT_EQ(window2Json["attributes"]["type"], "List");
    ASSERT_EQ(window2Json["attributes"]["bundleName"], "test12");
    ASSERT_EQ(window2Json["attributes"]["text"], "Text List");
    ASSERT_EQ(window2Json["attributes"]["origBounds"], "[30,10][60,120]");
    ASSERT_EQ(window2Json["attributes"]["bounds"], "[30,10][60,120]");
    ASSERT_EQ(window2Json["children"].size(), 1);
    ASSERT_EQ(window2Json["children"][0]["attributes"]["accessibilityId"], "6");
    ASSERT_EQ(window2Json["children"][0]["attributes"]["text"], "Text Test");
    ASSERT_EQ(window2Json["children"][0]["attributes"]["origBounds"], "[30,10][60,120]");
    ASSERT_EQ(window2Json["children"][0]["attributes"]["bounds"], "[30,10][60,120]");
}

class WindowCacheCompareGreater {
public:
    bool operator()(const WindowCacheModel &w1, const WindowCacheModel &w2)
    {
        if (w1.window_.actived_) {
            return true;
        }
        if (w2.window_.actived_) {
            return false;
        }
        if (w1.window_.focused_) {
            return true;
        }
        if (w2.window_.focused_) {
            return false;
        }
        return w1.window_.windowLayer_ > w2.window_.windowLayer_;
    }
};

TEST_F(UiDriverTest, SORT_WINDOW)
{
    Window w1{1};
    w1.actived_ = false;
    w1.focused_ = false;
    w1.windowLayer_ = 1;
    Window w2{2};
    w2.actived_ = false;
    w2.focused_ = false;
    w2.windowLayer_ = 2;
    Window w3{3};
    w3.actived_ = true;
    w3.focused_ = true;
    w3.windowLayer_ = 3;
    Window w4{4};
    w4.actived_ = false;
    w4.focused_ = false;
    w4.windowLayer_ = 4;
    Window w5{5};
    w5.actived_ = false;
    w5.focused_ = false;
    w5.windowLayer_ = 5;
    WindowCacheModel winModel1(w1);
    WindowCacheModel winModel2(w2);
    WindowCacheModel winModel3(w3);
    WindowCacheModel winModel4(w4);
    WindowCacheModel winModel5(w5);
    vector<WindowCacheModel> modelVec;
    modelVec.emplace_back(move(winModel5));
    modelVec.emplace_back(move(winModel4));
    modelVec.emplace_back(move(winModel3));
    modelVec.emplace_back(move(winModel2));
    modelVec.emplace_back(move(winModel1));
    std::sort(modelVec.begin(), modelVec.end(), WindowCacheCompareGreater());
    ASSERT_EQ(modelVec.at(0).window_.id_, 3);
    ASSERT_EQ(modelVec.at(1).window_.id_, 5);
    ASSERT_EQ(modelVec.at(2).window_.id_, 4);
    ASSERT_EQ(modelVec.at(3).window_.id_, 2);
    ASSERT_EQ(modelVec.at(4).window_.id_, 1);
}

TEST_F(UiDriverTest, SORT_WINDOW_ACTIVE)
{
    Window w1{1};
    w1.actived_ = false;
    w1.focused_ = false;
    w1.windowLayer_ = 1;
    Window w2{2};
    w2.actived_ = true;
    w2.focused_ = false;
    w2.windowLayer_ = 2;
    Window w3{3};
    w3.actived_ = false;
    w3.focused_ = true;
    w3.windowLayer_ = 3;
    Window w4{4};
    w4.actived_ = false;
    w4.focused_ = false;
    w4.windowLayer_ = 4;
    Window w5{5};
    w5.actived_ = false;
    w5.focused_ = false;
    w5.windowLayer_ = 5;
    WindowCacheModel winModel1{w1};
    WindowCacheModel winModel2{w2};
    WindowCacheModel winModel3{w3};
    WindowCacheModel winModel4{w4};
    WindowCacheModel winModel5{w5};
    vector<WindowCacheModel> modelVec;
    modelVec.emplace_back(move(winModel5));
    modelVec.emplace_back(move(winModel4));
    modelVec.emplace_back(move(winModel3));
    modelVec.emplace_back(move(winModel2));
    modelVec.emplace_back(move(winModel1));
    std::sort(modelVec.begin(), modelVec.end(), WindowCacheCompareGreater());
    ASSERT_EQ(modelVec.at(0).window_.id_, 2);
    ASSERT_EQ(modelVec.at(1).window_.id_, 3);
    ASSERT_EQ(modelVec.at(2).window_.id_, 5);
    ASSERT_EQ(modelVec.at(3).window_.id_, 4);
    ASSERT_EQ(modelVec.at(4).window_.id_, 1);
}