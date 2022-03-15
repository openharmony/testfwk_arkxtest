/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#include <cmath>
#include "ui_action.h"
#include "gtest/gtest.h"

using namespace OHOS::uitest;
using namespace std;

class UiActionTest : public testing::Test {
public:
    ~UiActionTest() override = default;
protected:
    void SetUp() override
    {
        Test::SetUp();
        // use custom UiOperationOptions
        customOptions_.clickHoldMs_ = 100;
        customOptions_.longClickHoldMs_ = 200;
        customOptions_.doubleClickIntervalMs_ = 300;
        customOptions_.keyHoldMs_ = 400;
        customOptions_.swipeVelocityPps_ = 500;
        customOptions_.uiSteadyThresholdMs_ = 600;
        customOptions_.waitUiSteadyMaxMs_ = 700;
    }
    UiDriveOptions customOptions_;
};

TEST_F(UiActionTest, computeClickAction)
{
    GenericClick action(PointerOp::CLICK_P);
    Point point{100, 200};
    vector<TouchEvent> events;
    action.Decompose(events, point, customOptions_);
    ASSERT_EQ(2, events.size()); // up & down
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    ASSERT_EQ(point.px_, event1.point_.px_);
    ASSERT_EQ(point.py_, event1.point_.py_);
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(0, event1.downTimeOffsetMs_);

    ASSERT_EQ(point.px_, event2.point_.px_);
    ASSERT_EQ(point.py_, event2.point_.py_);
    ASSERT_EQ(ActionStage::UP, event2.stage_);
    ASSERT_EQ(customOptions_.clickHoldMs_, event2.downTimeOffsetMs_);
}

TEST_F(UiActionTest, computeLongClickAction)
{
    GenericClick action(PointerOp::LONG_CLICK_P);
    Point point{100, 200};
    vector<TouchEvent> events;
    action.Decompose(events, point, customOptions_);
    ASSERT_EQ(2, events.size()); // up & down
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    ASSERT_EQ(point.px_, event1.point_.px_);
    ASSERT_EQ(point.py_, event1.point_.py_);
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(0, event1.downTimeOffsetMs_);
    // there should be a proper pause after touch down to make long-click
    ASSERT_EQ(customOptions_.longClickHoldMs_, event1.holdMs_);

    ASSERT_EQ(point.px_, event2.point_.px_);
    ASSERT_EQ(point.py_, event2.point_.py_);
    ASSERT_EQ(ActionStage::UP, event2.stage_);
    ASSERT_EQ(customOptions_.longClickHoldMs_, event2.downTimeOffsetMs_);
}

TEST_F(UiActionTest, computeDoubleClickAction)
{
    GenericClick action(PointerOp::DOUBLE_CLICK_P);
    Point point{100, 200};
    vector<TouchEvent> events;
    action.Decompose(events, point, customOptions_);
    ASSERT_EQ(4, events.size()); // up-down-interval-up-down
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    auto event3 = *(events.begin() + 2);
    auto event4 = *(events.begin() + 3);
    ASSERT_EQ(point.px_, event1.point_.px_);
    ASSERT_EQ(point.py_, event1.point_.py_);
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(0, event1.downTimeOffsetMs_);

    ASSERT_EQ(point.px_, event2.point_.px_);
    ASSERT_EQ(point.py_, event2.point_.py_);
    ASSERT_EQ(ActionStage::UP, event2.stage_);
    ASSERT_EQ(customOptions_.clickHoldMs_, event2.downTimeOffsetMs_);
    // there should be a proper pause after first click
    ASSERT_EQ(customOptions_.doubleClickIntervalMs_, event2.holdMs_);

    ASSERT_EQ(point.px_, event3.point_.px_);
    ASSERT_EQ(point.py_, event3.point_.py_);
    ASSERT_EQ(ActionStage::DOWN, event3.stage_);
    ASSERT_EQ(0, event3.downTimeOffsetMs_);

    ASSERT_EQ(point.px_, event4.point_.px_);
    ASSERT_EQ(point.py_, event4.point_.py_);
    ASSERT_EQ(ActionStage::UP, event4.stage_);
    ASSERT_EQ(customOptions_.clickHoldMs_, event4.downTimeOffsetMs_);
}

TEST_F(UiActionTest, computeSwipeAction)
{
    UiDriveOptions opt{};
    opt.swipeVelocityPps_ = 50; // specify the swipe velocity
    Point point0{0, 0}, point1{100, 200};
    GenericSwipe action(PointerOp::SWIPE_P);
    vector<TouchEvent> events;
    action.Decompose(events, point0, point1, opt);
    // there should be more than 1 touches
    const int32_t steps = events.size() - 1;
    ASSERT_TRUE(steps > 1);

    const int32_t disX = point1.px_ - point0.px_;
    const int32_t disY = point1.py_ - point0.py_;
    const uint32_t distance = sqrt(disX * disX + disY * disY);
    const uint32_t totalCostMs = distance * 1000 / opt.swipeVelocityPps_;

    uint32_t step = 0;
    // check the TouchEvent of each step
    for (auto &event:events) {
        int32_t expectedPointerX = point0.px_ + (disX * step) / steps;
        int32_t expectedPointerY = point0.py_ + (disY * step) / steps;
        uint32_t expectedTimeOffset = (totalCostMs * step) / steps;
        ASSERT_NEAR(expectedPointerX, event.point_.px_, 5);
        ASSERT_NEAR(expectedPointerY, event.point_.py_, 5);
        ASSERT_NEAR(expectedTimeOffset, event.downTimeOffsetMs_, 5);
        if (step == 0) {
            // should start with Action.DOWN
            ASSERT_EQ(ActionStage::DOWN, event.stage_);
        } else if (step == events.size() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, event.stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, event.stage_);
        }
        step++;
    }
}

TEST_F(UiActionTest, computeDragAction)
{
    UiDriveOptions opt{};
    opt.longClickHoldMs_ = 2000; // specify the long-click duration
    Point point0{0, 0}, point1{100, 200};
    GenericSwipe swipeAction(PointerOp::SWIPE_P);
    GenericSwipe dragAction(PointerOp::DRAG_P);
    vector<TouchEvent> swipeEvents;
    vector<TouchEvent> dragEvents;
    swipeAction.Decompose(swipeEvents, point0, point1, opt);
    dragAction.Decompose(dragEvents, point0, point1, opt);

    ASSERT_TRUE(swipeEvents.size() > 1);\
    ASSERT_EQ(swipeEvents.size(), dragEvents.size());

    // check the hold time of each event
    for (auto step = 0; step < swipeEvents.size(); step++) {
        auto &swipeEvent = swipeEvents.at(step);
        auto &dragEvent = dragEvents.at(step);
        ASSERT_EQ(swipeEvent.stage_, dragEvent.stage_);
        // drag needs longPressDown firstly, the downOffSet of following event should be delayed
        if (step == 0) {
            ASSERT_EQ(swipeEvent.downTimeOffsetMs_, dragEvent.downTimeOffsetMs_);
            ASSERT_EQ(swipeEvent.holdMs_ + opt.longClickHoldMs_, dragEvent.holdMs_);
        } else {
            ASSERT_EQ(swipeEvent.downTimeOffsetMs_ + opt.longClickHoldMs_, dragEvent.downTimeOffsetMs_);
            ASSERT_EQ(swipeEvent.holdMs_, dragEvent.holdMs_);
        }
    }
}

TEST_F(UiActionTest, computeBackKeyAction)
{
    Back keyAction;
    vector<KeyEvent> events;
    keyAction.ComputeEvents(events, customOptions_);
    ASSERT_EQ(2, events.size()); // up & down
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    ASSERT_EQ(KEYCODE_BACK, event1.code_);
    ASSERT_EQ(KEYCODE_NONE, event1.ctrlKey_); // no control-key
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(0, event1.downTimeOffsetMs_);

    ASSERT_EQ(KEYCODE_BACK, event2.code_);
    ASSERT_EQ(KEYCODE_NONE, event1.ctrlKey_); // no control-key
    ASSERT_EQ(ActionStage::UP, event2.stage_);
    ASSERT_EQ(customOptions_.keyHoldMs_, event2.downTimeOffsetMs_);
    ASSERT_EQ(KEYNAME_BACK, keyAction.Describe()); // test the description
}

TEST_F(UiActionTest, computePasteAction)
{
    Paste keyAction;
    vector<KeyEvent> events;
    keyAction.ComputeEvents(events, customOptions_);
    ASSERT_EQ(2, events.size()); // up & down
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    // key: ctrl+v
    ASSERT_EQ(KEYCODE_V, event1.code_);
    ASSERT_EQ(KEYCODE_CTRL, event1.ctrlKey_);
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(0, event1.downTimeOffsetMs_);
    // key: ctrl+v
    ASSERT_EQ(KEYCODE_V, event2.code_);
    ASSERT_EQ(KEYCODE_CTRL, event1.ctrlKey_);
    ASSERT_EQ(ActionStage::UP, event2.stage_);
    ASSERT_EQ(customOptions_.keyHoldMs_, event2.downTimeOffsetMs_);
    ASSERT_EQ(KEYNAME_PASTE, keyAction.Describe());
}

TEST_F(UiActionTest, anonymousSignleKey)
{
    static constexpr uint32_t keyCode = 1234;
    static const string expectedDesc = "key_" + to_string(keyCode);
    AnonymousSingleKey anonymousKey(keyCode);
    vector<KeyEvent> events;
    anonymousKey.ComputeEvents(events, customOptions_);
    ASSERT_EQ(2, events.size()); // up & down
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    ASSERT_EQ(keyCode, event1.code_);
    ASSERT_EQ(KEYCODE_NONE, event1.ctrlKey_); // no control-key
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(0, event1.downTimeOffsetMs_);

    ASSERT_EQ(keyCode, event2.code_);
    ASSERT_EQ(KEYCODE_NONE, event1.ctrlKey_); // no control-key
    ASSERT_EQ(ActionStage::UP, event2.stage_);
    ASSERT_EQ(customOptions_.keyHoldMs_, event2.downTimeOffsetMs_);
    ASSERT_EQ(expectedDesc, anonymousKey.Describe());
}

TEST_F(UiActionTest, computeCharsTypingEvents)
{
    vector<pair<int32_t, int32_t>> keyCodes;
    vector<KeyEvent> events;
    keyCodes.emplace_back(make_pair('a', KEYCODE_NONE));
    keyCodes.emplace_back(make_pair('b', 'c'));
    keyCodes.emplace_back(make_pair('d', KEYCODE_NONE));
    ComputeCharsTypingEvents(keyCodes, events);
    ASSERT_EQ(6, events.size());
    ASSERT_EQ('a', events.at(0).code_);
    ASSERT_EQ(ActionStage::DOWN, events.at(0).stage_);
    ASSERT_EQ('a', events.at(1).code_);
    ASSERT_EQ(ActionStage::UP, events.at(1).stage_);
    ASSERT_EQ('b', events.at(2).code_);
    ASSERT_EQ(ActionStage::DOWN, events.at(2).stage_);
    ASSERT_EQ('c', events.at(2).ctrlKey_);
    ASSERT_EQ(ActionStage::DOWN, events.at(2).stage_);
    ASSERT_EQ('b', events.at(3).code_);
    ASSERT_EQ(ActionStage::UP, events.at(3).stage_);
    ASSERT_EQ('c', events.at(3).ctrlKey_);
    ASSERT_EQ(ActionStage::UP, events.at(3).stage_);
    ASSERT_EQ('d', events.at(4).code_);
    ASSERT_EQ(ActionStage::DOWN, events.at(4).stage_);
    ASSERT_EQ('d', events.at(5).code_);
    ASSERT_EQ(ActionStage::UP, events.at(5).stage_);
}