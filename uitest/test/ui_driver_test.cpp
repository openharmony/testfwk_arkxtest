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

using namespace OHOS::uitest;
using namespace std;

static constexpr auto ATTR_TEXT = "text";
// record the triggered touch events.
static vector<TouchEvent> touch_event_records;

class MockController : public UiController {
public:
    explicit MockController() : UiController("mock_controller") {}

    ~MockController() = default;

    void SetDomFrame(string_view domFrame)
    {
        mockDomFrame_ = domFrame;
    }

    void GetUiHierarchy(vector<pair<Window, nlohmann::json>>& out) override
    {
        auto winInfo = Window(0);
        auto  dom = nlohmann::json::parse(mockDomFrame_);
        out.push_back(make_pair(move(winInfo), move(dom)));
    }

    bool IsWorkable() const override
    {
        return true;
    }

private:
    string mockDomFrame_;
};

// test fixture
class UiDriverTest : public testing::Test {
protected:
    void SetUp() override
    {
        touch_event_records.clear();
        auto mockController = make_unique<MockController>();
        controller_ = mockController.get();
        UiController::RegisterController(move(mockController), Priority::MEDIUM);
        driver_ = make_unique<UiDriver>();
    }

    void TearDown() override
    {
        controller_ = nullptr;
        UiController::RemoveAllControllers();
    }

    MockController *controller_ = nullptr;
    unique_ptr<UiDriver> driver_ = nullptr;
    UiOpArgs opt_;

    ~UiDriverTest() override = default;
};

TEST_F(UiDriverTest, internalError)
{
    // give no UiController, should cause internal error
    UiController::RemoveAllControllers();
    auto error = ApiCallErr(NO_ERROR);
    auto key = Back();
    driver_->TriggerKey(key, opt_, error);
    ASSERT_EQ(ERR_INTERNAL, error.code_);
}

TEST_F(UiDriverTest, normalInteraction)
{
    constexpr auto mockDom0 = R"({
"attributes": {
"index": "0",
"resource-id": "id1",
"bounds": "[0,0][100,100]",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"bounds": "[0,0][50,50]",
"text": "USB",
"type": "Text"
},
"children": []
}
]
}
)";
    controller_->SetDomFrame(mockDom0);

    auto error = ApiCallErr(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    selector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(selector, widgets, error);

    ASSERT_EQ(1, widgets.size());
    ASSERT_EQ("NONE", widgets.at(0)->GetHostTreeId()); // should return dettached widget
    // perform interactions
    auto key = Back();
    driver_->TriggerKey(key, opt_, error);
    ASSERT_EQ(NO_ERROR, error.code_);
}

TEST_F(UiDriverTest, retrieveWidgetFailure)
{
    constexpr auto mockDom0 = R"({
"attributes": {
"text": "",
"bounds": "[0,0][100,100]"
},
"children": [
{
"attributes": {
"text": "USB",
"bounds": "[0,0][50,50]"
},
"children": []
}
]
}
)";
    constexpr auto mockDom1 = R"({
"attributes": {
"text": "",
"bounds": "[0,0][100,100]"
},
"children": [
{
"attributes": {
"text": "WYZ",
"bounds": "[0,0][50,50]"
},
"children": []
}
]
}
)";
    controller_->SetDomFrame(mockDom0);

    auto error = ApiCallErr(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    selector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(selector, widgets, error);

    ASSERT_EQ(1, widgets.size());

    // mock another dom on which the target widget is missing, and perform click
    controller_->SetDomFrame(mockDom1);
    error = ApiCallErr(NO_ERROR);
    driver_->RetrieveWidget(*widgets.at(0), error, true);

    // retrieve widget failed should be marked as exception
    ASSERT_EQ(ERR_COMPONENT_LOST, error.code_);
    ASSERT_TRUE(error.message_.find(selector.Describe()) != string::npos)
                                << "Error message should contains the widget selection description";
}

TEST_F(UiDriverTest, retrieveWidget)
{
    constexpr auto mockDom0 = R"({
"attributes": {"bounds": "[0,0][100,100]", "text": ""},
"children": [
{
"attributes": {
"bounds": "[0,0][50,50]",
"hashcode": "12345",
"text": "USB","type": "Text"},
"children": []}]})";
    controller_->SetDomFrame(mockDom0);

    auto error = ApiCallErr(NO_ERROR);
    auto selector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_NAMES[UiAttr::HASHCODE], "12345", EQ);
    selector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(selector, widgets, error);
    ASSERT_EQ(1, widgets.size());
    ASSERT_EQ("USB", widgets.at(0)->GetAttr(ATTR_TEXT, ""));

    // mock new UI
    constexpr auto mockDom1 = R"({
"attributes": {"bounds": "[0,0][100,100]", "text": ""},
"children": [
{
"attributes": {
"bounds": "[0,0][50,50]",
"hashcode": "12345",
"text": "WYZ","type": "Text"},
"children": []}]})";
    controller_->SetDomFrame(mockDom1);
    // we should be able to refresh WidgetImage on the new UI
    auto snapshot = driver_->RetrieveWidget(*widgets.at(0), error);
    ASSERT_EQ(NO_ERROR, error.code_);
    ASSERT_NE(nullptr, snapshot);
    ASSERT_EQ("WYZ", snapshot->GetAttr(ATTR_TEXT, "")); // attribute should be updated to new value
    // mock new UI
    constexpr auto mockDom2 = R"({
"attributes": {"bounds": "[0,0][100,100]", "text": ""},
"children": [
{
"attributes": {
"bounds": "[0,0][50,50]",
"hashcode": "23456",
"text": "ZL", "type":"Button"},
"children": []}]})";
    controller_->SetDomFrame(mockDom2);
    // we should not be able to refresh WidgetImage on the new UI since its gone (hashcode and attributes changed)
    snapshot = driver_->RetrieveWidget(*widgets.at(0), error);
    ASSERT_EQ(ERR_COMPONENT_LOST, error.code_);
    ASSERT_EQ(nullptr, snapshot);
}