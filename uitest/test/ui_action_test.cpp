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

#include <cmath>
#include "gtest/gtest.h"
#include "ui_action.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr uint32_t CUSTOM_CLICK_HOLD_MS = 100;
static constexpr uint32_t CUSTOM_LONGCLICK_HOLD_MS = 200;
static constexpr uint32_t CUSTOM_DOUBLECLICK_HOLD_MS = 300;
static constexpr uint32_t CUSTOM_KEY_HOLD_MS = 400;
static constexpr uint32_t CUSTOM_SWIPE_VELOCITY_PPS = 500;
static constexpr uint32_t CUSTOM_UI_STEADY_MS = 600;
static constexpr uint32_t CUSTOM_WAIT_UI_STEADY_MS = 700;

class UiActionTest : public testing::Test {
public:
    ~UiActionTest() override = default;
protected:
    void SetUp() override
    {
        Test::SetUp();
        // use custom UiOperationOptions
        customOptions_.clickHoldMs_ = CUSTOM_CLICK_HOLD_MS;
        customOptions_.longClickHoldMs_ = CUSTOM_LONGCLICK_HOLD_MS;
        customOptions_.doubleClickIntervalMs_ = CUSTOM_DOUBLECLICK_HOLD_MS;
        customOptions_.keyHoldMs_ = CUSTOM_KEY_HOLD_MS;
        customOptions_.swipeVelocityPps_ = CUSTOM_SWIPE_VELOCITY_PPS;
        customOptions_.uiSteadyThresholdMs_ = CUSTOM_UI_STEADY_MS;
        customOptions_.waitUiSteadyMaxMs_ = CUSTOM_WAIT_UI_STEADY_MS;
    }
    UiOpArgs customOptions_;
};

TEST_F(UiActionTest, computeClickAction)
{
    Point point {100, 200};
    GenericClick action(TouchOp::CLICK, point);
    PointerMatrix events;
    action.Decompose(events, customOptions_);
    ASSERT_EQ(2, events.GetSize()); // up & down
    auto &event1 = events.At(0, 0);
    auto &event2 = events.At(0, 1);
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
    Point point {100, 200};
    GenericClick action(TouchOp::LONG_CLICK, point);
    PointerMatrix events;
    action.Decompose(events, customOptions_);
    ASSERT_EQ(2, events.GetSize()); // up & down
    auto &event1 = events.At(0, 0);
    auto &event2 = events.At(0, 1);
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
    Point point {100, 200};
    GenericClick action(TouchOp::DOUBLE_CLICK_P, point);
    PointerMatrix events;
    action.Decompose(events, customOptions_);
    ASSERT_EQ(4, events.GetSize()); // up-down-interval-up-down
    auto &event1 = events.At(0, 0);
    auto &event2 = events.At(0, 1);
    auto &event3 = events.At(0, 2);
    auto &event4 = events.At(0, 3);
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
    UiOpArgs opt {};
    opt.swipeVelocityPps_ = 50; // specify the swipe velocity
    Point point0(0, 0);
    Point point1(100, 200);
    GenericSwipe action(TouchOp::SWIPE, point0, point1);
    PointerMatrix events;
    action.Decompose(events, opt);
    // there should be more than 1 touches
    const int32_t steps = events.GetSize() - 1;
    ASSERT_TRUE(steps > 1);

    const int32_t disX = point1.px_ - point0.px_;
    const int32_t disY = point1.py_ - point0.py_;
    const uint32_t distance = sqrt(disX * disX + disY * disY);
    const uint32_t totalCostMs = distance * 1000 / opt.swipeVelocityPps_;

    uint32_t step = 0;
    // check the TouchEvent of each step
    for (uint32_t event = 0; event < events.GetSize(); event++) {
        int32_t expectedPointerX = point0.px_ + (disX * step) / steps;
        int32_t expectedPointerY = point0.py_ + (disY * step) / steps;
        uint32_t expectedTimeOffset = (totalCostMs * step) / steps;
        ASSERT_NEAR(expectedPointerX, events.At(0, event).point_.px_, 5);
        ASSERT_NEAR(expectedPointerY, events.At(0, event).point_.py_, 5);
        ASSERT_NEAR(expectedTimeOffset, events.At(0, event).downTimeOffsetMs_, 5);
        if (step == 0) {
            // should start with Action.DOWN
            ASSERT_EQ(ActionStage::DOWN, events.At(0, event).stage_);
        } else if (step == events.GetSize() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, events.At(0, event).stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, events.At(0, event).stage_);
        }
        step++;
    }
}

TEST_F(UiActionTest, computeFlingAction)
{
    UiOpArgs opt {};
    opt.swipeVelocityPps_ = 800; // specify the swipe velocity
    Point point0(0, 0);
    Point point1(200, 200);

    const uint32_t stepLen = 2;
    const int32_t disX = point1.px_ - point0.px_;
    const int32_t disY = point1.py_ - point0.py_;
    const uint32_t distance = sqrt(disX * disX + disY * disY);
    const uint32_t totalCostMs = distance * 1000 / opt.swipeVelocityPps_;
    opt.swipeStepsCounts_ = distance / stepLen;

    GenericSwipe action(TouchOp::SWIPE, point0, point1);
    PointerMatrix events;
    action.Decompose(events, opt);
    // there should be more than 1 touches
    const uint16_t steps = opt.swipeStepsCounts_;
    ASSERT_TRUE(steps > 1);
    ASSERT_EQ(steps, 141);

    uint32_t step = 0;
    // check the TouchEvent of each step
    for (uint32_t event = 0; event < events.GetSize(); event++) {
        int32_t expectedPointerX = point0.px_ + (disX * step) / steps;
        int32_t expectedPointerY = point0.py_ + (disY * step) / steps;
        uint32_t expectedTimeOffset = (totalCostMs * step) / steps;
        ASSERT_NEAR(expectedPointerX, events.At(0, event).point_.px_, 5);
        ASSERT_NEAR(expectedPointerY, events.At(0, event).point_.py_, 5);
        ASSERT_NEAR(expectedTimeOffset, events.At(0, event).downTimeOffsetMs_, 5);
        if (step == 0) {
            // should start with Action.DOWN
            ASSERT_EQ(ActionStage::DOWN, events.At(0, event).stage_);
        } else if (step == events.GetSize() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, events.At(0, event).stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, events.At(0, event).stage_);
        }
        step++;
    }
}

TEST_F(UiActionTest, computePinchInAction)
{
    UiOpArgs opt {};
    opt.swipeVelocityPps_ = 50; // specify the swipe velocity
    Rect rect(210, 510, 30, 330);
    float_t scale = 0.5;
    GenericPinch action(rect, scale);
    PointerMatrix events;
    action.Decompose(events, opt);
    // there should be more than 1 touches
    const int32_t steps = events.GetSteps() - 1;
    ASSERT_TRUE(steps > 1);
    ASSERT_EQ(102, events.GetSize());
    ASSERT_EQ(51, events.GetSteps());

    const int32_t disX0 = abs(rect.left_ - rect.GetCenterX()) * abs(scale - 1);
    ASSERT_EQ(75, disX0);

    uint32_t step = 0;
    // check the TouchEvent of each step
    for (uint32_t eventStep = 0; eventStep < events.GetSteps(); eventStep++) {
        uint32_t eventFinger = 0;
        int32_t expectedPointerX0 = rect.left_ + (disX0 * step) / steps;
        uint32_t x0 = events.At(eventFinger, eventStep).point_.px_;
        ASSERT_NEAR(expectedPointerX0, x0, 20);
        if (eventStep == 0) {
        // should start with Action.DOWN
        ASSERT_EQ(ActionStage::DOWN, events.At(eventFinger, eventStep).stage_);
        ASSERT_EQ(0, events.At(eventFinger, eventStep).downTimeOffsetMs_);
        } else if (eventStep == events.GetSteps() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, events.At(eventFinger, eventStep).stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, events.At(eventFinger, eventStep).stage_);
        }
        step++;
    }
    step = 0;
    for (uint32_t eventStep = 0; eventStep < events.GetSteps(); eventStep++) {
        uint32_t eventFinger = 1;
        int32_t expectedPointerX0 = rect.right_ - (disX0 * step) / steps;
        ASSERT_NEAR(expectedPointerX0, events.At(eventFinger, eventStep).point_.px_, 20);
        if (eventStep == 0) {
        // should start with Action.DOWN
        ASSERT_EQ(ActionStage::DOWN, events.At(eventFinger, eventStep).stage_);
        } else if (eventStep == events.GetSteps() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, events.At(eventFinger, eventStep).stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, events.At(eventFinger, eventStep).stage_);
        }
        step++;
    }
}

TEST_F(UiActionTest, computeMultiPointerMatrixAction)
{
    UiOpArgs opt {};
    opt.swipeVelocityPps_ = 50; // specify the swipe velocity
    const uint32_t finger = 2;
    const uint32_t step = 4;
    PointerMatrix pointer(finger, step);
    for (uint32_t stepIndex = 0; stepIndex < step; stepIndex++) {
        pointer.At(0, stepIndex).point_.px_ = 245 + 20 * stepIndex;
        pointer.At(0, stepIndex).point_.py_ = 480;
    }
    for (uint32_t stepIndex = 0; stepIndex < step; stepIndex++) {
        pointer.At(1, stepIndex).point_.px_ = 505 - 20 * stepIndex;
        pointer.At(1, stepIndex).point_.py_ = 480;
    }
    MultiPointerAction action(pointer);
    ASSERT_EQ(2, pointer.GetFingers());
    ASSERT_EQ(4, pointer.GetSteps());
    ASSERT_EQ(245, pointer.At(0, 0).point_.px_);
    PointerMatrix events;
    action.Decompose(events, opt);
    // there should be more than 1 touches
    ASSERT_EQ(8, events.GetSize());
    ASSERT_EQ(4, events.GetSteps());
    for (uint32_t eventStep = 0; eventStep < events.GetSteps(); eventStep++) {
        for (uint32_t eventFinger = 0; eventFinger < events.GetFingers(); eventFinger++) {
            if (eventStep == 0) {
                ASSERT_EQ(ActionStage::DOWN, events.At(eventFinger, eventStep).stage_);
            } else if (eventStep == events.GetSteps() - 1) {
                ASSERT_EQ(ActionStage::UP, events.At(eventFinger, eventStep).stage_);
            } else {
                ASSERT_EQ(ActionStage::MOVE, events.At(eventFinger, eventStep).stage_);
            }
        }
    }
}

TEST_F(UiActionTest, computeMultiPointerMatrixAction1)
{
    UiOpArgs opt {};
    opt.swipeVelocityPps_ = 600; // specify the swipe velocity
    const uint32_t finger = 4;
    const uint32_t step = 4;
    PointerMatrix pointer(finger, step);
    for (uint32_t stepIndex = 0; stepIndex < step; stepIndex++) {
        pointer.At(0, stepIndex).point_.px_ = 245 + 20 * stepIndex;
        pointer.At(0, stepIndex).point_.py_ = 480;
    }
    for (uint32_t stepIndex = 0; stepIndex < step; stepIndex++) {
        pointer.At(1, stepIndex).point_.px_ = 505 - 20 * stepIndex;
        pointer.At(1, stepIndex).point_.py_ = 480;
    }
    for (uint32_t stepIndex = 0; stepIndex < step; stepIndex++) {
        pointer.At(2, stepIndex).point_.px_ = 375;
        pointer.At(2, stepIndex).point_.py_ = 350 + 20 * stepIndex;
    }
    for (uint32_t stepIndex = 0; stepIndex < step; stepIndex++) {
        pointer.At(3, stepIndex).point_.px_ = 375;
        pointer.At(3, stepIndex).point_.py_ = 610 - 20 * stepIndex;
    }
    MultiPointerAction action(pointer);
    ASSERT_EQ(4, pointer.GetFingers());
    ASSERT_EQ(4, pointer.GetSteps());
    ASSERT_EQ(245, pointer.At(0, 0).point_.px_);
    ASSERT_EQ(590, pointer.At(3, 1).point_.py_);
    PointerMatrix events;
    action.Decompose(events, opt);
    // there should be more than 1 touches
    ASSERT_EQ(16, events.GetSize());
    ASSERT_EQ(4, events.GetSteps());
    for (uint32_t eventStep = 0; eventStep < events.GetSteps(); eventStep++) {
        for (uint32_t eventFinger = 0; eventFinger < events.GetFingers(); eventFinger++) {
            if (eventStep == 0) {
                ASSERT_EQ(ActionStage::DOWN, events.At(eventFinger, eventStep).stage_);
            } else if (eventStep == events.GetSteps() - 1) {
                ASSERT_EQ(ActionStage::UP, events.At(eventFinger, eventStep).stage_);
            } else {
                ASSERT_EQ(ActionStage::MOVE, events.At(eventFinger, eventStep).stage_);
            }
        }
    }
}

TEST_F(UiActionTest, computePinchOutAction)
{
    UiOpArgs opt {};
    opt.swipeVelocityPps_ = 50; // specify the swipe velocity
    Rect rect(210, 510, 30, 330);
    float_t scale = 1.5;
    GenericPinch action(rect, scale);
    PointerMatrix events;
    action.Decompose(events, opt);
    // there should be more than 1 touches
    const int32_t steps = events.GetSteps() - 1;
    ASSERT_TRUE(steps > 1);
    ASSERT_EQ(102, events.GetSize());
    ASSERT_EQ(51, events.GetSteps());

    const int32_t disX0 = abs(rect.left_ - rect.GetCenterX()) * abs(scale - 1);
    ASSERT_EQ(75, disX0);

    uint32_t step = 0;
    // check the TouchEvent of each step
    for (uint32_t eventStep = 0; eventStep < events.GetSteps(); eventStep++) {
        uint32_t eventFinger = 0;
        int32_t expectedPointerX0 = rect.GetCenterX() - (disX0 * step) / steps;
        uint32_t x0 = events.At(eventFinger, eventStep).point_.px_;
        ASSERT_NEAR(expectedPointerX0, x0, 20);
        if (eventStep == 0) {
        // should start with Action.DOWN
        ASSERT_EQ(ActionStage::DOWN, events.At(eventFinger, eventStep).stage_);
        ASSERT_EQ(0, events.At(eventFinger, eventStep).downTimeOffsetMs_);
        } else if (eventStep == events.GetSteps() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, events.At(eventFinger, eventStep).stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, events.At(eventFinger, eventStep).stage_);
        }
        step++;
    }
    step = 0;
    for (uint32_t eventStep = 0; eventStep < events.GetSteps(); eventStep++) {
        uint32_t eventFinger = 1;
        int32_t expectedPointerX0 = rect.GetCenterX() + (disX0 * step) / steps;
        ASSERT_NEAR(expectedPointerX0, events.At(eventFinger, eventStep).point_.px_, 20);
        if (eventStep == 0) {
        // should start with Action.DOWN
        ASSERT_EQ(ActionStage::DOWN, events.At(eventFinger, eventStep).stage_);
        } else if (eventStep == events.GetSteps() - 1) {
            // should end with Action.UP
            ASSERT_EQ(ActionStage::UP, events.At(eventFinger, eventStep).stage_);
        } else {
            // middle events should all be action-MOVE
            ASSERT_EQ(ActionStage::MOVE, events.At(eventFinger, eventStep).stage_);
        }
        step++;
    }
}

TEST_F(UiActionTest, computeDragAction)
{
    UiOpArgs opt {};
    opt.longClickHoldMs_ = 2000; // specify the long-click duration
    Point point0(0, 0);
    Point point1(100, 200);
    GenericSwipe swipeAction(TouchOp::SWIPE, point0, point1);
    GenericSwipe dragAction(TouchOp::DRAG, point0, point1);
    PointerMatrix swipeEvents;
    PointerMatrix dragEvents;
    swipeAction.Decompose(swipeEvents, opt);
    dragAction.Decompose(dragEvents, opt);

    ASSERT_TRUE(swipeEvents.GetSize() > 1);
    ASSERT_EQ(swipeEvents.GetSize(), dragEvents.GetSize());

    // check the hold time of each event
    for (uint32_t step = 0; step < swipeEvents.GetSize(); step++) {
        auto &swipeEvent = swipeEvents.At(0, step);
        auto &dragEvent = dragEvents.At(0, step);
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
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);

    ASSERT_EQ(KEYCODE_BACK, event2.code_);
    ASSERT_EQ(ActionStage::UP, event2.stage_);
}

TEST_F(UiActionTest, computePasteAction)
{
    Paste keyAction;
    vector<KeyEvent> events;
    keyAction.ComputeEvents(events, customOptions_);
    ASSERT_EQ(4, events.size()); // ctrl_down/key_down/key_up/ctrl_up
    auto event1 = *events.begin();
    auto event2 = *(events.begin() + 1);
    auto event3 = *(events.begin() + 2);
    auto event4 = *(events.begin() + 3);

    ASSERT_EQ(KEYCODE_CTRL, event1.code_);
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);
    ASSERT_EQ(KEYCODE_V, event2.code_);
    ASSERT_EQ(ActionStage::DOWN, event2.stage_);

    ASSERT_EQ(KEYCODE_V, event3.code_);
    ASSERT_EQ(ActionStage::UP, event3.stage_);
    ASSERT_EQ(KEYCODE_CTRL, event4.code_);
    ASSERT_EQ(ActionStage::UP, event4.stage_);
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
    ASSERT_EQ(ActionStage::DOWN, event1.stage_);

    ASSERT_EQ(keyCode, event2.code_);
    ASSERT_EQ(ActionStage::UP, event2.stage_);
}
