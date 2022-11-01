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

using namespace OHOS::uitest;
using namespace std;

static constexpr auto ATTR_TEXT = "text";
// record the triggered touch events.

static std::unique_ptr<PointerMatrix> touch_event_records = nullptr;

class MockController2 : public UiController {
public:
    explicit MockController2() : UiController("mock_controller") {}

    ~MockController2() = default;

    void SetDomFrame(string_view domFrame)
    {
        mockDomFrames_.clear();
        mockDomFrames_.emplace_back(string(domFrame));
        frameIndex_ = 0;
    }

    void SetDomFrames(vector<string> domFrames)
    {
        mockDomFrames_.clear();
        mockDomFrames_ = move(domFrames);
        frameIndex_ = 0;
    }

    uint32_t GetConsumedDomFrameCount() const
    {
        return frameIndex_;
    }

    void GetUiHierarchy(vector<pair<Window, nlohmann::json>>& out) override
    {
        uint32_t newIndex = frameIndex_;
        frameIndex_++;
        auto winInfo = Window(0);
        auto dom = nlohmann::json();
        if (newIndex >= mockDomFrames_.size()) {
            dom = nlohmann::json::parse(mockDomFrames_.at(mockDomFrames_.size() - 1));
        } else {
            dom = nlohmann::json::parse(mockDomFrames_.at((newIndex)));
        }
        out.push_back(make_pair(move(winInfo), move(dom)));
    }

    bool IsWorkable() const override
    {
        return true;
    }

    void InjectTouchEventSequence(const PointerMatrix &events) const override
    {
        touch_event_records = std::make_unique<PointerMatrix>(events.GetFingers(), events.GetSteps());
        for (uint32_t step = 0; step < events.GetSteps(); step++) {
            for (uint32_t finger = 0; finger < events.GetFingers(); finger++) {
                touch_event_records->PushAction(events.At(finger, step));
            }
        }
    }

private:
    vector<string> mockDomFrames_;
    mutable uint32_t frameIndex_ = 0;
};

// test fixture
class WidgetOperatorTest : public testing::Test {
protected:
    void SetUp() override
    {
        touch_event_records.reset(nullptr);
        auto mockController = make_unique<MockController2>();
        controller_ = mockController.get();
        UiController::RegisterController(move(mockController), Priority::MEDIUM);
        driver_ = make_unique<UiDriver>();
    }

    void TearDown() override
    {
        controller_ = nullptr;
        UiController::RemoveAllControllers();
    }

    MockController2 *controller_ = nullptr;
    unique_ptr<UiDriver> driver_ = nullptr;
    UiOpArgs opt_;

    ~WidgetOperatorTest() override = default;
};

TEST_F(WidgetOperatorTest, scrollSearchRetrieveSubjectWidgetFailed)
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
"text": "USB"
},
"children": []
}
]
})";
    constexpr auto mockDom1 = R"({"attributes":{"bounds":"[0,0][100,100]"},"children":[]})";
    controller_->SetDomFrame(mockDom0);

    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(scrollWidgetSelector, widgets, error);

    ASSERT_EQ(1, widgets.size());

    // mock another dom on which the scroll-widget is missing, and perform scroll-search
    controller_->SetDomFrame(mockDom1);
    error = ApiCallErr(NO_ERROR);
    auto targetWidgetSelector = WidgetSelector();
    auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
    // retrieve scroll widget failed should be marked as exception
    ASSERT_EQ(ERR_COMPONENT_LOST, error.code_);
    ASSERT_TRUE(error.message_.find(scrollWidgetSelector.Describe()) != string::npos)
                                << "Error message should contains the scroll-widget selection description";
}

TEST_F(WidgetOperatorTest, scrollSearchTargetWidgetNotExist)
{
    constexpr auto mockDom = R"({
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
"text": "USB"
},
"children": []
}
]
}
)";
    controller_->SetDomFrame(mockDom);

    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(scrollWidgetSelector, widgets, error);

    ASSERT_EQ(1, widgets.size());

    error = ApiCallErr(NO_ERROR);
    auto targetWidgetMatcher = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);
    auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
}

TEST_F(WidgetOperatorTest, scrollSearchCheckSubjectWidget)
{
    constexpr auto mockDom = R"({
"attributes": {
"bounds": "[0,0][1200,2000]",
"text": ""
},
"children": [
{
"attributes": {
"bounds": "[0,200][600,1000]",
"text": "USB",
"type": "List"
},
"children": []
}
]
}
)";
    controller_->SetDomFrame(mockDom);

    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> images;
    driver_->FindWidgets(scrollWidgetSelector, images, error);

    ASSERT_EQ(1, images.size());

    error = ApiCallErr(NO_ERROR);
    auto targetWidgetMatcher = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);
    opt_.scrollWidgetDeadZone_ = 0; // set deadzone to 0 for easy computation
    auto wOp = WidgetOperator(*driver_, *images.at(0), opt_);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
    // check the scroll action events, should be acted on the subject node specified by WidgetMatcher
    ASSERT_TRUE(!touch_event_records->Empty());
    auto &firstEvent = touch_event_records->At(0, 0);
    auto fin = touch_event_records->GetFingers() - 1;
    auto ste = touch_event_records->GetSteps() - 1;
    auto &lastEvent = touch_event_records->At(fin, ste);
    // check scroll event pointer_x
    int32_t subjectCx = (0 + 600) / 2;
    ASSERT_NEAR(firstEvent.point_.px_, subjectCx, 5);
    ASSERT_NEAR(lastEvent.point_.px_, subjectCx, 5);

    // check scroll event pointer_y
    constexpr int32_t subjectWidgetHeight = 1000 - 200;
    int32_t maxCy = 0;
    int32_t minCy = 1E5;
    for (uint32_t finger = 0; finger < touch_event_records->GetFingers(); finger++) {
        for (uint32_t step = 0; step < touch_event_records->GetSteps(); step++) {
            if (touch_event_records->At(finger, step).point_.py_ > maxCy) {
            maxCy = touch_event_records->At(finger, step).point_.py_;
            }
            if (touch_event_records->At(finger, step).point_.py_ < minCy) {
            minCy = touch_event_records->At(finger, step).point_.py_;
            }
        }
    }

    int32_t scrollDistanceY = maxCy - minCy;
    ASSERT_TRUE(abs(scrollDistanceY - subjectWidgetHeight) < 5);
}

TEST_F(WidgetOperatorTest, scrollSearchCheckDirection)
{
    constexpr auto mockDom = R"({
"attributes": {
"bounds": "[0,0][100,100]",
"text": ""
},
"children": [
{
"attributes": {
"bounds": "[0,0][50,50]",
"text": "USB",
"type": "List"
},
"children": []
}]})";
    controller_->SetDomFrame(mockDom);

    auto error = ApiCallErr(NO_ERROR);
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(scrollWidgetSelector, widgets, error);
    ASSERT_EQ(1, widgets.size());

    error = ApiCallErr(NO_ERROR);
    auto targetWidgetMatcher = WidgetAttrMatcher(ATTR_TEXT, "wyz", EQ);
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);
    opt_.scrollWidgetDeadZone_ = 0; // set deadzone to 0 for easy computation
    auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
    ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
    // check the scroll action events, should be acted on the specified node
    ASSERT_TRUE(!touch_event_records->Empty());
    // should scroll-search upward (cy_from<cy_to) then downward (cy_from>cy_to)
    uint32_t maxCyEventIndex = 0;
    uint32_t index = 0;
    for (uint32_t event = 0; event < touch_event_records->GetSize() - 1; event++) {
        if (touch_event_records->At(0, event).point_.py_ > touch_event_records->At(0, maxCyEventIndex).point_.py_) {
            maxCyEventIndex = index;
        }
        index++;
    }

    for (uint32_t idx = 0; idx < touch_event_records->GetSize() - 1; idx++) {
        if (idx < maxCyEventIndex) {
            ASSERT_LT(touch_event_records->At(0, idx).point_.py_, touch_event_records->At(0, idx + 1).point_.py_);
        } else if (idx > maxCyEventIndex) {
            ASSERT_GT(touch_event_records->At(0, idx).point_.py_, touch_event_records->At(0, idx + 1).point_.py_);
        }
    }
}

/**
 * The expected scroll-search count. The search starts upward till the target widget
 * is found or reach the top (DOM snapshot becomes frozen); then search downward till
 * the target widget is found or reach the bottom (DOM snapshot becomes frozen); */
TEST_F(WidgetOperatorTest, scrollSearchCheckCount_targetNotExist)
{
    auto error = ApiCallErr(NO_ERROR);
    // mocked widget text
    const vector<string> domFrameSet[4] = {
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})"
        },
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})"
        },
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WLJ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})"
        },
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WLJ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WLJ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})"
        }
    };

    controller_->SetDomFrames(domFrameSet[0]); // set frame, let the scroll-widget be found firstly
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(scrollWidgetSelector, widgets, error);

    ASSERT_EQ(1, widgets.size());

    auto targetWidgetMatcher = WidgetAttrMatcher(ATTR_TEXT, "xyz", EQ); // widget that will never be found
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);

    const uint32_t expectedSearchCount[] = {3, 4, 5, 5};
    opt_.scrollWidgetDeadZone_ = 0; // set deadzone to 0 for easy computation
    for (size_t index = 0; index < 4; index++) {
        controller_->SetDomFrames(domFrameSet[index]);
        // check search result
        auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
        ASSERT_EQ(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
        // check scroll-search count
        ASSERT_EQ(expectedSearchCount[index], controller_->GetConsumedDomFrameCount()) << index;
    }
}

TEST_F(WidgetOperatorTest, scrollSearchCheckCount_targetExist)
{
    auto error = ApiCallErr(NO_ERROR);
    const vector<string> domFrameSet[4] = {
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})"
        },
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WLJ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"XYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})"
        },
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})"
        },
        {
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"USB"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"XYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WLJ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})",
            R"({"attributes":{"bounds":"[0,0][100,100]","hashcode":"123","type":"List","text":"WYZ"},"children":[]})"
        }
    };

    controller_->SetDomFrames(domFrameSet[1]); // set frame, let the scroll-widget be found firstly
    auto scrollWidgetSelector = WidgetSelector();
    auto matcher = WidgetAttrMatcher(ATTR_TEXT, "USB", EQ);
    scrollWidgetSelector.AddMatcher(matcher);
    vector<unique_ptr<Widget>> widgets;
    driver_->FindWidgets(scrollWidgetSelector, widgets, error);
    ASSERT_EQ(1, widgets.size());

    auto targetWidgetMatcher = WidgetAttrMatcher(ATTR_TEXT, "WYZ", EQ);
    auto targetWidgetSelector = WidgetSelector();
    targetWidgetSelector.AddMatcher(targetWidgetMatcher);

    const uint32_t expectedSearchCount[] = {1, 2, 3, 4};
    for (size_t index = 0; index < 4; index++) {
        controller_->SetDomFrames(domFrameSet[index]);
        // check search result
        auto wOp = WidgetOperator(*driver_, *widgets.at(0), opt_);
        ASSERT_NE(nullptr, wOp.ScrollFindWidget(targetWidgetSelector, error));
        ASSERT_EQ("NONE", widgets.at(0)->GetHostTreeId()); // should return dettached widget
        // check scroll-search count
        ASSERT_EQ(expectedSearchCount[index], controller_->GetConsumedDomFrameCount()) << index;
    }
}