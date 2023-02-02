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
        size_t index;
        std::string_view message;
    };

    static constexpr Operational OPERATIONS[] = {
        {MOVETO, FULLSCREEN, false, INDEX_ZERO, "Fullscreen window can not move"},
        {MOVETO, SPLIT_PRIMARY, false, INDEX_ZERO, "SPLIT_PRIMARY window can not move"},
        {MOVETO, SPLIT_SECONDARY, false, INDEX_ZERO, "SPLIT_SECONDARY window can not move"},
        {MOVETO, FLOATING, true, INDEX_ZERO, ""},
        {RESIZE, FULLSCREEN, false, INDEX_ZERO, "Fullscreen window can not resize"},
        {RESIZE, SPLIT_PRIMARY, true, INDEX_ZERO, ""},
        {RESIZE, SPLIT_SECONDARY, true, INDEX_ZERO, ""},
        {RESIZE, FLOATING, true, INDEX_ZERO, ""},
        {SPLIT, FULLSCREEN, true, INDEX_ONE, ""},
        {SPLIT, SPLIT_PRIMARY, false, INDEX_ONE, "SPLIT_PRIMARY can not split again"},
        {SPLIT, SPLIT_SECONDARY, true, INDEX_ONE, ""},
        {SPLIT, FLOATING, true, INDEX_ONE, ""},
        {MAXIMIZE, FULLSCREEN, false, INDEX_TWO, "Fullscreen window is already maximized"},
        {MAXIMIZE, SPLIT_PRIMARY, true, INDEX_ONE, ""},
        {MAXIMIZE, SPLIT_SECONDARY, true, INDEX_TWO, ""},
        {MAXIMIZE, FLOATING, true, INDEX_TWO, ""},
        {RESUME, FULLSCREEN, true, INDEX_TWO, ""},
        {RESUME, SPLIT_PRIMARY, true, INDEX_ONE, ""},
        {RESUME, SPLIT_SECONDARY, true, INDEX_TWO, ""},
        {RESUME, FLOATING, true, INDEX_TWO, ""},
        {MINIMIZE, FULLSCREEN, true, INDEX_THREE, ""},
        {MINIMIZE, SPLIT_PRIMARY, true, INDEX_TWO, ""},
        {MINIMIZE, SPLIT_SECONDARY, true, INDEX_THREE, ""},
        {MINIMIZE, FLOATING, true, INDEX_THREE, ""},
        {CLOSE, FULLSCREEN, true, INDEX_FOUR, ""},
        {CLOSE, SPLIT_PRIMARY, true, INDEX_THREE, ""},
        {CLOSE, SPLIT_SECONDARY, true, INDEX_FOUR, ""},
        {CLOSE, FLOATING, true, INDEX_FOUR, ""}
    };

    static bool CheckOperational(WindowAction action, WindowMode mode, ApiReplyInfo &out, size_t &targetIndex)
    {
        for (unsigned long index = 0; index < sizeof(OPERATIONS) / sizeof(Operational); index++) {
            if (OPERATIONS[index].action == action && OPERATIONS[index].windowMode == mode) {
                if (OPERATIONS[index].support) {
                    targetIndex = OPERATIONS[index].index;
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
        if (window_.mode_ == WindowMode::FLOATING) {
            return;
        }
        auto rect = window_.bounds_;
        static constexpr uint32_t step1 = 10;
        static constexpr uint32_t step2 = 40;
        Point from(rect.GetCenterX(), rect.top_ + step1);
        Point to(rect.GetCenterX(), rect.top_ + step2);
        auto touch = GenericSwipe(TouchOp::DRAG, from, to);
        driver_.PerformTouch(touch, options_, out.exception_);
        driver_.DelayMs(options_.uiSteadyThresholdMs_);
    }

    void WindowOperator::Focus(ApiReplyInfo &out)
    {
        if (window_.focused_) {
            return;
        } else {
            auto rect = window_.bounds_;
            static constexpr uint32_t step = 10;
            Point focus(rect.GetCenterX(), rect.top_ + step);
            auto touch = GenericClick(TouchOp::CLICK, focus);
            driver_.PerformTouch(touch, options_, out.exception_);
        }
    }

    void WindowOperator::MoveTo(uint32_t endX, uint32_t endY, ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(MOVETO, window_.mode_, out, index)) {
            return;
        }
        auto rect = window_.bounds_;
        static constexpr uint32_t step = 40;
        Point from(rect.left_ + step, rect.top_ + step);
        Point to(endX + step, endY + step);
        auto touch = GenericSwipe(TouchOp::DRAG, from, to);
        driver_.PerformTouch(touch, options_, out.exception_);
    }

    void WindowOperator::Resize(int32_t width, int32_t highth, ResizeDirection direction, ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(RESIZE, window_.mode_, out, index)) {
            return;
        }
        if ((((direction == LEFT) || (direction == RIGHT)) && highth != window_.bounds_.GetHeight()) ||
            (((direction == D_UP) || (direction == D_DOWN)) && width != window_.bounds_.GetWidth())) {
            out.exception_ = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "Resize cannot be done in this direction");
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
        driver_.PerformTouch(GenericSwipe(TouchOp::DRAG, from, to), options_, out.exception_);
    }

    void WindowOperator::Split(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(SPLIT, window_.mode_, out, index)) {
            return;
        }
        BarAction(index, out);
    }

    void  WindowOperator::Maximize(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(MAXIMIZE, window_.mode_, out, index)) {
            return;
        }
        BarAction(index, out);
    }

    void WindowOperator::Resume(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(RESUME, window_.mode_, out, index)) {
            return;
        }
        BarAction(index, out);
    }

    void WindowOperator::Minimize(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(MINIMIZE, window_.mode_, out, index)) {
            return;
        }
        BarAction(index, out);
    }

    void WindowOperator::Close(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!CheckOperational(CLOSE, window_.mode_, out, index)) {
            return;
        }
        BarAction(index, out);
    }

    void WindowOperator::BarAction(size_t index, ApiReplyInfo &out)
    {
        CallBar(out);
        auto selector = WidgetSelector();
        auto frontLocator = WidgetSelector();
        auto attrMatcher = WidgetAttrMatcher(ATTR_NAMES[UiAttr::TYPE], "Button", EQ);
        auto windowMatcher = WidgetAttrMatcher(ATTR_NAMES[UiAttr::HOST_WINDOW_ID], to_string(window_.id_), EQ);
        auto frontMatcher = WidgetAttrMatcher(ATTR_NAMES[UiAttr::TYPE], "DecorBar", EQ);
        frontLocator.AddMatcher(frontMatcher);
        selector.AddMatcher(attrMatcher);
        selector.AddMatcher(windowMatcher);
        selector.AddFrontLocator(frontLocator, out.exception_);
        vector<unique_ptr<Widget>> widgets;
        driver_.FindWidgets(selector, widgets, out.exception_);
        if (widgets.empty()) {
            auto selectorForJs = WidgetSelector();
            auto attrMatcherForJs = WidgetAttrMatcher(ATTR_NAMES[UiAttr::TYPE], "button", EQ);
            selectorForJs.AddMatcher(attrMatcherForJs);
            selectorForJs.AddMatcher(windowMatcher);
            selectorForJs.AddFrontLocator(frontLocator, out.exception_);
            driver_.FindWidgets(selectorForJs, widgets, out.exception_, false);
        }
        if (out.exception_.code_ != NO_ERROR) {
            return;
        }
        if (widgets.size() < index) {
            LOG_E("Not find target winAction button");
            return;
        }
        auto rect = widgets[index - 1]->GetBounds();
        Point widgetCenter(rect.GetCenterX(), rect.GetCenterY());
        auto touch = GenericClick(TouchOp::CLICK, widgetCenter);
        driver_.PerformTouch(touch, options_, out.exception_);
    }
} // namespace OHOS::uitest
