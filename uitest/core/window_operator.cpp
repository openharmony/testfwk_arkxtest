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
        std::string_view buttonId;
        std::string_view message;
    };

    static constexpr Operational OPERATIONS[] = {
        {MOVETO, FULLSCREEN, false, "", "Fullscreen window can not move"},
        {MOVETO, SPLIT_PRIMARY, false, "", "SPLIT_PRIMARY window can not move"},
        {MOVETO, SPLIT_SECONDARY, false, "", "SPLIT_SECONDARY window can not move"},
        {MOVETO, FLOATING, true, "", ""},
        {RESIZE, FULLSCREEN, false, "", "Fullscreen window can not resize"},
        {RESIZE, SPLIT_PRIMARY, true, "", ""},
        {RESIZE, SPLIT_SECONDARY, true, "", ""},
        {RESIZE, FLOATING, true, "", ""},
        {SPLIT, FULLSCREEN, true, "", ""},
        {SPLIT, SPLIT_PRIMARY, false, "", "SPLIT_PRIMARY can not split again"},
        {SPLIT, SPLIT_SECONDARY, true, "", ""},
        {SPLIT, FLOATING, true, "", ""},
        {MAXIMIZE, FULLSCREEN, false, "", "Fullscreen window is already maximized"},
        {MAXIMIZE, SPLIT_PRIMARY, true, "EnhanceMaximizeBtn", ""},
        {MAXIMIZE, SPLIT_SECONDARY, true, "EnhanceMaximizeBtn", ""},
        {MAXIMIZE, FLOATING, true, "EnhanceMaximizeBtn", ""},
        {RESUME, FULLSCREEN, true, "EnhanceMaximizeBtn", ""},
        {RESUME, SPLIT_PRIMARY, true, "EnhanceMaximizeBtn", ""},
        {RESUME, SPLIT_SECONDARY, true, "EnhanceMaximizeBtn", ""},
        {RESUME, FLOATING, true, "EnhanceMaximizeBtn", ""},
        {MINIMIZE, FULLSCREEN, true, "EnhanceMinimizeBtn", ""},
        {MINIMIZE, SPLIT_PRIMARY, true, "EnhanceMinimizeBtn", ""},
        {MINIMIZE, SPLIT_SECONDARY, true, "EnhanceMinimizeBtn", ""},
        {MINIMIZE, FLOATING, true, "EnhanceMinimizeBtn", ""},
        {CLOSE, FULLSCREEN, true, "EnhanceCloseBtn", ""},
        {CLOSE, SPLIT_PRIMARY, true, "EnhanceCloseBtn", ""},
        {CLOSE, SPLIT_SECONDARY, true, "EnhanceCloseBtn", ""},
        {CLOSE, FLOATING, true, "EnhanceCloseBtn", ""}};

    static bool CheckOperational(WindowAction action, WindowMode mode, ApiReplyInfo &out, string &buttonId)
    {
        for (unsigned long index = 0; index < sizeof(OPERATIONS) / sizeof(Operational); index++) {
            if (OPERATIONS[index].action == action && OPERATIONS[index].windowMode == mode) {
                if (OPERATIONS[index].support) {
                    buttonId = OPERATIONS[index].buttonId;
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

    void WindowOperator::CallBar(ApiReplyInfo &out)
    {
        Focus(out);
        if (window_.mode_ == WindowMode::FLOATING) {
            return;
        }
        auto rect = window_.bounds_;
        static constexpr uint32_t step1 = 10;
        static constexpr uint32_t step2 = 40;
        Point from(rect.GetCenterX(), rect.top_ + step1);
        Point to(rect.GetCenterX(), rect.top_ + step2);
        from.displayId_ = window_.displayId_;
        to.displayId_ = window_.displayId_;
        auto touch = GenericSwipe(TouchOp::DRAG, from, to);
        driver_.PerformTouch(touch, options_, out.exception_);
        driver_.DelayMs(options_.uiSteadyThresholdMs_);
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
        string targetBtnId;
        if (!CheckOperational(MOVETO, window_.mode_, out, targetBtnId)) {
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
        string targetBtnId;
        if (!CheckOperational(RESIZE, window_.mode_, out, targetBtnId)) {
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

    void WindowOperator::Split(ApiReplyInfo &out)
    {
        string targetBtnId;
        if (!CheckOperational(SPLIT, window_.mode_, out, targetBtnId)) {
            return;
        }
        // call split bar.
        auto selector = WidgetSelector();
        auto attrMatcher = WidgetMatchModel(UiAttr::KEY, "EnhanceMaximizeBtn", EQ);
        auto windowMatcher = WidgetMatchModel(UiAttr::HOST_WINDOW_ID, to_string(window_.id_), EQ);
        selector.AddMatcher(attrMatcher);
        selector.AddMatcher(windowMatcher);
        selector.AddAppLocator(window_.bundleName_);
        selector.SetWantMulti(false);
        vector<unique_ptr<Widget>> widgets;
        driver_.FindWidgets(selector, widgets, out.exception_);
        if (out.exception_.code_ != NO_ERROR) {
            return;
        }
        if (widgets.empty()) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto rect = widgets[0]->GetBounds();
        Point widgetCenter(rect.GetCenterX(), rect.GetCenterY(), widgets[0]->GetDisplayId());
        auto touch1 = MouseMoveTo(widgetCenter);
        driver_.PerformMouseAction(touch1, options_, out.exception_);
        constexpr auto focusTime = 3000;
        driver_.DelayMs(focusTime);
        // find split btn and click.
        auto selector2 = WidgetSelector();
        auto attrMatcher2 = WidgetMatchModel(UiAttr::KEY, "EnhanceMenuScreenLeftRow", EQ);
        selector2.AddMatcher(attrMatcher2);
        selector2.SetWantMulti(false);
        vector<unique_ptr<Widget>> widgets2;
        driver_.FindWidgets(selector2, widgets2, out.exception_);
        if (widgets2.empty()) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto rect2 = widgets2[0]->GetBounds();
        Point widgetCenter2(rect2.GetCenterX(), rect2.GetCenterY(), widgets2[0]->GetDisplayId());
        auto touch2 = GenericClick(TouchOp::CLICK, widgetCenter2);
        driver_.PerformTouch(touch2, options_, out.exception_);
    }

    void  WindowOperator::Maximize(ApiReplyInfo &out)
    {
        string targetBtnId;
        if (!CheckOperational(MAXIMIZE, window_.mode_, out, targetBtnId)) {
            return;
        }
        BarAction(targetBtnId, out);
    }

    void WindowOperator::Resume(ApiReplyInfo &out)
    {
        string targetBtnId;
        if (!CheckOperational(RESUME, window_.mode_, out, targetBtnId)) {
            return;
        }
        BarAction(targetBtnId, out);
    }

    void WindowOperator::Minimize(ApiReplyInfo &out)
    {
        string targetBtnId;
        if (!CheckOperational(MINIMIZE, window_.mode_, out, targetBtnId)) {
            return;
        }
        BarAction(targetBtnId, out);
    }

    void WindowOperator::Close(ApiReplyInfo &out)
    {
        string targetBtnId;
        if (!CheckOperational(CLOSE, window_.mode_, out, targetBtnId)) {
            return;
        }
        BarAction(targetBtnId, out);
    }

    void WindowOperator::BarAction(string_view buttonId, ApiReplyInfo &out)
    {
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
        if (widgets.empty()) {
            CallBar(out);
            driver_.FindWidgets(selector, widgets, out.exception_);
        }
        if (widgets.empty() || out.exception_.code_ != NO_ERROR) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "this device can not support this action");
            return;
        }
        auto rect = widgets[0]->GetBounds();
        Point widgetCenter(rect.GetCenterX(), rect.GetCenterY(), widgets[0]->GetDisplayId());
        auto touch = GenericClick(TouchOp::CLICK, widgetCenter);
        driver_.PerformTouch(touch, options_, out.exception_);
    }
} // namespace OHOS::uitest
