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

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

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

    WindowOperator::WindowOperator(UiDriver &driver, const Window &window, UiOpArgs &options)
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
        auto rect = window_.bounds_;
        static constexpr uint32_t step = 40;
        Point from(rect.left_ + step, rect.top_ + step);
        Point to(endX + step, endY + step);
        from.displayId_ = window_.displayId_;
        to.displayId_ = window_.displayId_;
        auto touch = GenericSwipe(TouchOp::DRAG, from, to);
        driver_.PerformTouch(touch, options_, out.exception_);
    }

    void WindowOperator::Resize(int32_t width, int32_t highth, ResizeDirection direction, ApiReplyInfo &out)
    {
        Focus(out);
        if (!CheckOperational(RESIZE, window_.mode_, out)) {
            return;
        }
        Point from;
        Point to;
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
        driver_.PerformTouch(GenericSwipe(TouchOp::DRAG, from, to), options_, out.exception_);
    }

#ifdef ARKXTEST_WATCH_FEATURE_ENABLE
    void WindowOperator::BarAction(string_view buttonId, ApiReplyInfo &out)
    {
        if (!window_.decoratorEnabled_) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        Focus(out);
        auto selector = WidgetSelector();
        auto attrMatcher = WidgetMatchModel(UiAttr::KEY, std::string(buttonId), EQ);
        auto windowMatcher = WidgetMatchModel(UiAttr::HOST_WINDOW_ID, to_string(window_.id_), EQ);
        selector.AddMatcher(attrMatcher);
        selector.AddMatcher(windowMatcher);
        selector.AddAppLocator(window_.bundleName_);
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
    }
#endif

    void WindowOperator::Split(ApiReplyInfo &out)
    {
        if (!CheckOperational(SPLIT, window_.mode_, out)) {
            return;
        }
        driver_.ChangeWindowMode(window_.id_, WindowMode::SPLIT_PRIMARY);
    }

    void  WindowOperator::Maximize(ApiReplyInfo &out)
    {
        if (!CheckOperational(MAXIMIZE, window_.mode_, out)) {
            return;
        }
#ifdef ARKXTEST_WATCH_FEATURE_ENABLE
        const auto maximizeBtnId = "EnhanceMaximizeBtn";
        BarAction(maximizeBtnId, out);
        return;
#endif
        driver_.ChangeWindowMode(window_.id_, WindowMode::FULLSCREEN);
    }

    void WindowOperator::Resume(ApiReplyInfo &out)
    {
        if (!CheckOperational(RESUME, window_.mode_, out)) {
            return;
        }
#ifdef ARKXTEST_WATCH_FEATURE_ENABLE
        const auto resumeBtnId = "EnhanceMaximizeBtn";
        BarAction(resumeBtnId, out);
        return;
#endif
        if (window_.mode_ == WindowMode::FULLSCREEN) {
            driver_.ChangeWindowMode(window_.id_, WindowMode::FLOATING);
        } else {
            driver_.ChangeWindowMode(window_.id_, WindowMode::FULLSCREEN);
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
