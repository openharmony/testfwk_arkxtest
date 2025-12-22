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

#include "window_operator.h"
#include <map>
#include <thread>

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    const uint32_t WIN_OP_SPEED = 6000;
    enum WindowAction : uint8_t {
        FOCUS,
        MOVETO,
        RESIZE,
        SPLIT,
        MAXIMIZE,
        RESUME,
        MINIMIZE,
        CLOSE
    };

    struct Operational {
        WindowAction action;
        WindowMode windowMode;
        bool support;
        std::string_view message;
    };

    static constexpr Operational OPERATIONS[] = {
        {MOVETO, FULLSCREEN, false, "Fullscreen window can not move"},
        {MOVETO, SPLIT_PRIMARY, false, "SPLIT_PRIMARY window can not move"},
        {MOVETO, SPLIT_SECONDARY, false, "SPLIT_SECONDARY window can not move"},
        {MOVETO, FLOATING, true, ""},
        {RESIZE, FULLSCREEN, false, "Fullscreen window can not resize"},
        {RESIZE, SPLIT_PRIMARY, true, ""},
        {RESIZE, SPLIT_SECONDARY, true, ""},
        {RESIZE, FLOATING, true, ""},
        {SPLIT, FULLSCREEN, true, ""},
        {SPLIT, SPLIT_PRIMARY, false, "SPLIT_PRIMARY can not split again"},
        {SPLIT, SPLIT_SECONDARY, true, ""},
        {SPLIT, FLOATING, true, ""},
        {MAXIMIZE, FULLSCREEN, false, "Fullscreen window is already maximized"},
        {MAXIMIZE, SPLIT_PRIMARY, true, ""},
        {MAXIMIZE, SPLIT_SECONDARY, true, ""},
        {MAXIMIZE, FLOATING, true, ""},
        {RESUME, FULLSCREEN, true, ""},
        {RESUME, SPLIT_PRIMARY, true, ""},
        {RESUME, SPLIT_SECONDARY, true, ""},
        {RESUME, FLOATING, true, ""},
        {MINIMIZE, FULLSCREEN, true, ""},
        {MINIMIZE, SPLIT_PRIMARY, true, ""},
        {MINIMIZE, SPLIT_SECONDARY, true, ""},
        {MINIMIZE, FLOATING, true, ""},
        {CLOSE, FULLSCREEN, true, ""},
        {CLOSE, SPLIT_PRIMARY, true, ""},
        {CLOSE, SPLIT_SECONDARY, true, ""},
        {CLOSE, FLOATING, true, ""}};

    static bool CheckOperational(WindowAction action, WindowMode mode, ApiReplyInfo &out)
    {
        for (unsigned long index = 0; index < sizeof(OPERATIONS) / sizeof(Operational); index++) {
            if (OPERATIONS[index].action == action && OPERATIONS[index].windowMode == mode) {
                if (OPERATIONS[index].support) {
                    return true;
                } else {
                    out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, OPERATIONS[index].message);
                    return false;
                }
            }
        }
        out.exception_ = ApiCallErr(ERR_INTERNAL, "No such window mode-action combination registered");
        return false;
    }

    WindowOperator::WindowOperator(UiDriver &driver, Window &window, UiOpArgs &options)
        : driver_(driver), window_(window), options_(options)
    {
    }

    void WindowOperator::Focus(ApiReplyInfo &out)
    {
        if (window_.focused_) {
            return;
        } else {
            auto rect = window_.visibleBounds_;
            static constexpr uint32_t step = 10;
            Point focus(rect.GetCenterX(), rect.top_ + step, window_.displayId_);
            auto touch = GenericClick(TouchOp::CLICK, focus);
            driver_.PerformTouch(touch, options_, out.exception_);
        }
    }

    void WindowOperator::MoveTo(uint32_t endX, uint32_t endY, ApiReplyInfo &out)
    {
        Focus(out);
        if (!CheckOperational(MOVETO, window_.mode_, out)) {
            return;
        }
        auto from = Point(0, 0);
        auto to = Point(0, 0);
        auto isPcWindowMode = driver_.IsPcWindowMode();
        if (!isPcWindowMode) {
            auto selector = WidgetSelector();
            auto attrMatcher = WidgetMatchModel(UiAttr::KEY, std::string("SCBFloatTitleButton"), EQ);
            selector.AddMatcher(attrMatcher);
            selector.SetWantMulti(false);
            vector<unique_ptr<Widget>> widgets;
            driver_.FindWidgets(selector, widgets, out.exception_, false);
            if (widgets.empty() || out.exception_.code_ != NO_ERROR) {
                out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
                return;
            }
            auto rect = widgets[0]->GetBounds();
            from = Point(rect.GetCenterX(), rect.GetCenterY(), window_.displayId_);
            to = Point(endX + window_.bounds_.GetWidth() / TWO, endY + rect.GetHeight() / TWO, window_.displayId_);
        } else {
            auto rect = window_.bounds_;
            constexpr uint32_t step = 40;
            from = Point(rect.left_ + step, rect.top_ + step, window_.displayId_);
            to = Point(endX + step, endY + step, window_.displayId_);
        }
        auto touch = GenericSwipe(TouchOp::DRAG, from, to);
        driver_.PerformTouch(touch, options_, out.exception_);
    }

    void WindowOperator::CreateResizePoint(int32_t width, int32_t highth, ResizeDirection direction, Point &from,
        Point &to)
    {
        switch (direction) {
            case (LEFT):
                from = Point(window_.bounds_.left_, window_.bounds_.GetCenterY());
                to = Point((window_.bounds_.right_ - width), window_.bounds_.GetCenterY());
                break;
            case (RIGHT):
                from = Point(window_.bounds_.right_, window_.bounds_.GetCenterY());
                to = Point((window_.bounds_.left_ + width), window_.bounds_.GetCenterY());
                break;
            case (D_UP):
                from = Point(window_.bounds_.GetCenterX(), window_.bounds_.top_);
                to = Point(window_.bounds_.GetCenterX(), window_.bounds_.bottom_ - highth);
                break;
            case (D_DOWN):
                from = Point(window_.bounds_.GetCenterX(), window_.bounds_.bottom_);
                to = Point(window_.bounds_.GetCenterX(), window_.bounds_.top_ + highth);
                break;
            case (LEFT_UP):
                from = Point(window_.bounds_.left_, window_.bounds_.top_);
                to = Point(window_.bounds_.right_ - width, window_.bounds_.bottom_ - highth);
                break;
            case (LEFT_DOWN):
                from = Point(window_.bounds_.left_, window_.bounds_.bottom_);
                to = Point(window_.bounds_.right_ - width, window_.bounds_.top_ + highth);
                break;
            case (RIGHT_UP):
                from = Point(window_.bounds_.right_, window_.bounds_.top_);
                to = Point(window_.bounds_.left_ + width, window_.bounds_.bottom_ - highth);
                break;
            case (RIGHT_DOWN):
                from = Point(window_.bounds_.right_, window_.bounds_.bottom_);
                to = Point(window_.bounds_.left_ + width, window_.bounds_.top_ + highth);
                break;
            default:
                break;
        }
        from.displayId_ = window_.displayId_;
        to.displayId_ = window_.displayId_;
    }

    void WindowOperator::Resize(int32_t width, int32_t highth, ResizeDirection direction, ApiReplyInfo &out)
    {
        Focus(out);
        if (!CheckOperational(RESIZE, window_.mode_, out)) {
            return;
        }
        if (!driver_.IsPcWindowMode()) {
            if (direction == D_UP || direction == LEFT_UP || direction == RIGHT_UP) {
                out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
                return;
            }
        }
        Point from;
        Point to;
        CreateResizePoint(width, highth, direction, from, to);
        driver_.PerformTouch(GenericSwipe(TouchOp::DRAG, from, to), options_, out.exception_);
    }

    void WindowOperator::CallBar(ApiReplyInfo &out)
    {
        Focus(out);
        if (window_.mode_ == WindowMode::FLOATING) {
            return;
        }
        auto rect = window_.bounds_;
        constexpr int32_t step1 = 10;
        constexpr int32_t step2 = 100;
        constexpr int32_t waitMs = 100;
        Point from(rect.GetCenterX(), rect.top_ + step1, window_.displayId_);
        Point to(rect.GetCenterX(), rect.top_ + step2, window_.displayId_);
        driver_.PerformTouch(GenericSwipe(TouchOp::DRAG, from, to), options_, out.exception_);
        this_thread::sleep_for(chrono::milliseconds(waitMs));
    }

    void WindowOperator::BarAction(string_view buttonId, ApiReplyInfo &out)
    {
        Focus(out);
        auto selector = WidgetSelector();
        auto attrMatcher = WidgetMatchModel(UiAttr::KEY, std::string(buttonId), EQ);
        selector.AddMatcher(attrMatcher);
        selector.SetWantMulti(false);
        vector<unique_ptr<Widget>> widgets;
        driver_.FindWidgets(selector, widgets, out.exception_);
        if (widgets.empty() || out.exception_.code_ != NO_ERROR) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto rect = widgets[0]->GetBounds();
        Point widgetCenter(rect.GetCenterX(), rect.GetCenterY(), widgets[0]->GetDisplayId());
        auto touch = GenericClick(TouchOp::CLICK, widgetCenter);
        driver_.PerformTouch(touch, options_, out.exception_);
        constexpr auto waitMs = 1000;
        this_thread::sleep_for(chrono::milliseconds(waitMs));
    }

    void WindowOperator::FloatWindowInPhoneMode(ApiReplyInfo &out)
    {
        if (window_.mode_ != WindowMode::FULLSCREEN) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto screenSize = driver_.GetDisplaySize(out.exception_, window_.displayId_);
        if (out.exception_.code_ != NO_ERROR || screenSize.px_ == 0 || screenSize.py_ == 0) {
            LOG_E("Get screenSize failed.");
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto to = Point(screenSize.px_, 1, window_.displayId_);
        constexpr auto bottomZone = 20;
        constexpr auto waitMs = 1000;
        auto from = Point(window_.bounds_.GetCenterX(), window_.bounds_.bottom_ - bottomZone, window_.displayId_);
        options_.swipeVelocityPps_ = WIN_OP_SPEED;
        auto drag = GenericSwipe(TouchOp::SWIPE, from, to);
        driver_.PerformTouch(drag, options_, out.exception_);
        this_thread::sleep_for(chrono::milliseconds(waitMs));
        auto win = driver_.RetrieveWindow(window_, out.exception_);
        if (win == nullptr || out.exception_.code_ != NO_ERROR) {
            return;
        }
        auto center = Point(win->bounds_.GetCenterX(), win->bounds_.GetCenterY(), win->displayId_);
        auto click = GenericClick(TouchOp::CLICK, center);
        driver_.PerformTouch(click, options_, out.exception_);
        this_thread::sleep_for(chrono::milliseconds(waitMs));
    }

    void WindowOperator::SplitWindowInPhoneMode(ApiReplyInfo &out)
    {
        if (window_.mode_ != WindowMode::FULLSCREEN) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto to = Point(1, 1, window_.displayId_);
        constexpr auto bottomZone = 20;
        constexpr auto waitMs = 1000;
        auto from = Point(window_.bounds_.GetCenterX(), window_.bounds_.bottom_ - bottomZone, window_.displayId_);
        options_.swipeVelocityPps_ = WIN_OP_SPEED;
        auto drag = GenericSwipe(TouchOp::SWIPE, from, to);
        driver_.PerformTouch(drag, options_, out.exception_);
        this_thread::sleep_for(chrono::milliseconds(waitMs));
    }

    void WindowOperator::Split(ApiReplyInfo &out)
    {
        if (!CheckOperational(SPLIT, window_.mode_, out)) {
            return;
        }
#ifdef ARKXTEST_PC_FEATURE_ENABLE
        driver_.ChangeWindowMode(window_.id_, WindowMode::SPLIT_PRIMARY);
        return;
#endif
        auto isPcWindowMode = driver_.IsPcWindowMode();
        if (isPcWindowMode) {
            driver_.ChangeWindowMode(window_.id_, WindowMode::SPLIT_PRIMARY);
            return;
        }
        if (window_.mode_ != WindowMode::FULLSCREEN) {
            Maximize(out);
            auto win = driver_.RetrieveWindow(window_, out.exception_);
            if (win != nullptr || out.exception_.code_ != NO_ERROR) {
                window_ = *win;
            } else {
                return;
            }
        }
        SplitWindowInPhoneMode(out);
    }

    void WindowOperator::MaximizeSplitWindow(ApiReplyInfo &out)
    {
        auto selector = WidgetSelector();
        auto attrMatcher = WidgetMatchModel(UiAttr::KEY, "SCBDividerFlex1", EQ);
        selector.AddMatcher(attrMatcher);
        selector.SetWantMulti(false);
        vector<unique_ptr<Widget>> widgets;
        driver_.FindWidgets(selector, widgets, out.exception_);
        if (widgets.empty() || out.exception_.code_ != NO_ERROR) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto rect = widgets[0]->GetBounds();
        Point from(rect.GetCenterX(), rect.GetCenterY(), window_.displayId_);
        auto to = Point(0, 0);
        auto screenSize = driver_.GetDisplaySize(out.exception_, window_.displayId_);
        auto screenOrientation = driver_.GetScreenOrientation(out.exception_, window_.displayId_);
        if (screenOrientation == ONE || screenOrientation == THREE) {
            to = (window_.mode_ == WindowMode::SPLIT_PRIMARY) ?
                Point(from.px_, screenSize.py_, window_.displayId_) : Point(from.px_, 0, window_.displayId_);
        } else if (screenOrientation == TWO || screenOrientation == FOUR) {
            to = (window_.mode_ == WindowMode::SPLIT_PRIMARY) ?
                Point(screenSize.px_, from.py_, window_.displayId_) : Point(0, from.py_, window_.displayId_);
        } else {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto touch = GenericSwipe(TouchOp::SWIPE, from, to);
        options_.swipeVelocityPps_ = WIN_OP_SPEED;
        driver_.PerformTouch(touch, options_, out.exception_);
    }

    void  WindowOperator::Maximize(ApiReplyInfo &out)
    {
        if (!CheckOperational(MAXIMIZE, window_.mode_, out)) {
            return;
        }
        Focus(out);
#ifdef ARKXTEST_PC_FEATURE_ENABLE
        driver_.ChangeWindowMode(window_.id_, WindowMode::FULLSCREEN);
        return;
#endif
        auto isPcWindowMode = driver_.IsPcWindowMode();
        if (isPcWindowMode) {
            const auto maximizeBtnId = "EnhanceMaximizeBtn";
            BarAction(maximizeBtnId, out);
            auto win = driver_.RetrieveWindow(window_, out.exception_);
            if (win != nullptr || out.exception_.code_ != NO_ERROR) {
                window_ = *win;
            } else {
                return;
            }
            if (window_.mode_ != WindowMode::FULLSCREEN) {
                BarAction(maximizeBtnId, out);
            }
            return;
        }
        if (window_.mode_ == WindowMode::SPLIT_PRIMARY || window_.mode_ == WindowMode::SPLIT_SECONDARY) {
            MaximizeSplitWindow(out);
        } else {
            const auto maximizeBtnId = "floatingButtonMaximize";
            BarAction(maximizeBtnId, out);
        }
        constexpr auto waitMs = 1000;
        this_thread::sleep_for(chrono::milliseconds(waitMs));
    }

    void WindowOperator::Resume(ApiReplyInfo &out)
    {
        if (!CheckOperational(RESUME, window_.mode_, out)) {
            return;
        }
#ifdef ARKXTEST_PC_FEATURE_ENABLE
        if (window_.mode_ == WindowMode::FULLSCREEN) {
            driver_.ChangeWindowMode(window_.id_, WindowMode::FLOATING);
        } else {
            driver_.ChangeWindowMode(window_.id_, WindowMode::FULLSCREEN);
        }
        return;
#endif
        auto isPcWindowMode = driver_.IsPcWindowMode();
        if (isPcWindowMode) {
            const auto maximizeBtnId = "EnhanceMaximizeBtn";
            if (window_.mode_ == WindowMode::FULLSCREEN) {
                CallBar(out);
            }
            BarAction(maximizeBtnId, out);
            return;
        }
        if (window_.mode_ == WindowMode::SPLIT_PRIMARY || window_.mode_ == WindowMode::SPLIT_SECONDARY) {
            Maximize(out);
            auto win = driver_.RetrieveWindow(window_, out.exception_);
            if (win != nullptr || out.exception_.code_ != NO_ERROR) {
                window_ = *win;
            } else {
                return;
            }
            FloatWindowInPhoneMode(out);
            return;
        }
        if (window_.mode_ == WindowMode::FLOATING) {
            const auto maximizeBtnId = "floatingButtonMaximize";
            BarAction(maximizeBtnId, out);
            return;
        }
        if (window_.mode_ == WindowMode::FULLSCREEN) {
            FloatWindowInPhoneMode(out);
            return;
        }
    }

    void WindowOperator::Minimize(ApiReplyInfo &out)
    {
        if (!CheckOperational(MINIMIZE, window_.mode_, out)) {
            return;
        }
        driver_.ChangeWindowMode(window_.id_, WindowMode::MINIMIZED);
    }

    void WindowOperator::Close(ApiReplyInfo &out)
    {
        if (!CheckOperational(CLOSE, window_.mode_, out)) {
            return;
        }
        driver_.ChangeWindowMode(window_.id_, WindowMode::CLOSED);
    }
} // namespace OHOS::uitest
