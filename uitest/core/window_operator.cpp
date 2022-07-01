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
        FOCUSE,
        MOVETO,
        RESIZE,
        SPLIT,
        MAXIMIZE,
        RESUME,
        MINIMIZE,
        CLOSE
    };

    struct Operational {
        bool support;
        size_t index;
        std::string_view message;
    };

    static const std::map<std::pair<WindowAction, WindowMode>, Operational> operationalMap {
        {std::pair(MOVETO, FULLSCREEN), {false, 0, "Fullscreen window can not move"}},
        {std::pair(MOVETO, SPLIT_PRIMARY), {true, 0, ""}},
        {std::pair(MOVETO, SPLIT_SECONDARY), {true, 0, ""}},
        {std::pair(MOVETO, FLOATING), {true, 0, ""}},
        {std::pair(RESIZE, FULLSCREEN), {false, 0, "Fullscreen window can not resize"}},
        {std::pair(RESIZE, SPLIT_PRIMARY), {true, 0, ""}},
        {std::pair(RESIZE, SPLIT_SECONDARY), {true, 0, ""}},
        {std::pair(RESIZE, FLOATING), {true, 0, ""}},
        {std::pair(SPLIT, FULLSCREEN), {true, INDEX_ONE, ""}},
        {std::pair(SPLIT, SPLIT_PRIMARY), {true, INDEX_ONE, ""}},
        {std::pair(SPLIT, SPLIT_SECONDARY), {true, INDEX_ONE, ""}},
        {std::pair(SPLIT, FLOATING), {true, INDEX_ONE, ""}},
        {std::pair(MAXIMIZE, FULLSCREEN), {false, INDEX_TWO, "Fullscreen window is already maximized"}},
        {std::pair(MAXIMIZE, SPLIT_PRIMARY), {true, INDEX_TWO, ""}},
        {std::pair(MAXIMIZE, SPLIT_SECONDARY), {true, INDEX_TWO, ""}},
        {std::pair(MAXIMIZE, FLOATING), {true, INDEX_TWO, ""}},
        {std::pair(RESUME, FULLSCREEN), {true, INDEX_TWO, ""}},
        {std::pair(RESUME, SPLIT_PRIMARY), {true, INDEX_TWO, ""}},
        {std::pair(RESUME, SPLIT_SECONDARY), {true, INDEX_TWO, ""}},
        {std::pair(RESUME, FLOATING), {true, INDEX_TWO, ""}},
        {std::pair(MINIMIZE, FULLSCREEN), {true, INDEX_THREE, ""}},
        {std::pair(MINIMIZE, SPLIT_PRIMARY), {true, INDEX_THREE, ""}},
        {std::pair(MINIMIZE, SPLIT_SECONDARY), {true, INDEX_THREE, ""}},
        {std::pair(MINIMIZE, FLOATING), {true, INDEX_THREE, ""}},
        {std::pair(CLOSE, FULLSCREEN), {true, INDEX_FOUR, ""}},
        {std::pair(CLOSE, SPLIT_PRIMARY), {true, INDEX_FOUR, ""}},
        {std::pair(CLOSE, SPLIT_SECONDARY), {true, INDEX_FOUR, ""}},
        {std::pair(CLOSE, FLOATING), {true, INDEX_FOUR, ""}},
    };

    static bool IsOperational(const WindowAction action, const WindowMode mode, ApiReplyInfo &out, size_t &index)
    {
        std::pair<WindowAction, WindowMode> key (action, mode);
        if (operationalMap.find(key) == operationalMap.end()) {
            out.exception_.code_ = ErrCode::INTERNAL_ERROR;
            return false;
        }
        if (!operationalMap.find(key)->second.support) {
            out.exception_.message_ = operationalMap.find(key)->second.message;
            out.exception_.code_ = ErrCode::USAGE_ERROR;
            return false;
        }
        index = operationalMap.find(key)->second.index;
        return true;
    }

    WindowOperator::WindowOperator(UiDriver &driver, const Window &Window, UiOpArgs &options)
        : driver_(driver), window_(Window), options_(options)
    {
    }

    void WindowOperator::CallBar(ApiReplyInfo &out)
    {
        auto rect = window_.bounds_;
        static constexpr uint32_t step1 = 10;
        static constexpr uint32_t step2 = 40;
        Point from(rect.GetCenterX(), rect.top_ + step1);
        Point to(rect.GetCenterX(), rect.top_ + step2);
        driver_.PerformSwipe(TouchOp::DRAG, from, to, options_, out.exception_);
    }

    bool WindowOperator::Focuse(ApiReplyInfo &out)
    {
        if (window_.focused_) {
            return true;
        } else {
            auto rect = window_.bounds_;
            Point windowCenter(rect.GetCenterX(), rect.GetCenterY());
            driver_.PerformClick(TouchOp::CLICK, windowCenter, options_, out.exception_);
            return (out.exception_.code_ == ErrCode::NO_ERROR);
        }
    }

    bool WindowOperator::MoveTo(uint32_t endX, uint32_t endY, ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(MOVETO, window_.mode_, out, index)) {
            return false;
        }
        auto rect = window_.bounds_;
        static constexpr uint32_t step = 30;
        Point from(rect.left_ + step, rect.top_ + step);
        Point to(endX, endY);
        driver_.PerformSwipe(TouchOp::DRAG, from, to, options_, out.exception_);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    bool WindowOperator::Resize(int32_t width, int32_t highth, ResizeDirection direction, ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(RESIZE, window_.mode_, out, index)) {
            return false;
        }
        auto rect = window_.bounds_;
        if ((((direction == LEFT) || (direction == RIGHT))&& highth != rect.GetHeight()) ||
            (((direction == UP_) || (direction == DOWN_))&& width != rect.GetWidth())) {
                LOG_W("The operation cannot be done in this direction");
                return false;
            }
        Point from, to;
        switch (direction) {
            case (LEFT):
                from = Point(rect.left_, rect.GetCenterY());
                to = Point((rect.right_ - width), rect.GetCenterY());
                break;
            case (RIGHT):
                from = Point(rect.right_, rect.GetCenterY());
                to = Point((rect.left_ + width), rect.GetCenterY());
                break;
            case (UP_):
                from = Point(rect.GetCenterX(), rect.top_);
                to = Point(rect.GetCenterX(), rect.bottom_ - highth);
                break;
            case (DOWN_):
                from = Point(rect.GetCenterX(), rect.bottom_);
                to = Point(rect.GetCenterX(), rect.top_ + highth);
                break;
            case (LEFT_UP):
                from = Point(rect.left_, rect.top_);
                to = Point(rect.right_ - width, rect.bottom_ - highth);
                break;
            case (LEFT_DOWN):
                from = Point(rect.left_, rect.bottom_);
                to = Point(rect.right_ - width, rect.top_ + highth);
                break;
            case (RIGHT_UP):
                from = Point(rect.right_, rect.top_);
                to = Point(rect.left_ + width, rect.bottom_ - highth);
                break;
            case (RIGHT_DOWN):
                from = Point(rect.right_, rect.bottom_);
                to = Point(rect.left_ + width, rect.top_ + highth);
                break;
            default:
                break;
        }
        driver_.PerformSwipe(TouchOp::DRAG, from, to, options_, out.exception_);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    bool WindowOperator::Split(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(SPLIT, window_.mode_, out, index)) {
            return false;
        }
        BarAction(index, out);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    bool WindowOperator::Maximize(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(MAXIMIZE, window_.mode_, out, index)) {
            return false;
        }
        BarAction(index, out);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    bool WindowOperator::Resume(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(RESUME, window_.mode_, out, index)) {
            return false;
        }
        BarAction(index, out);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    bool WindowOperator::Minimize(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(MINIMIZE, window_.mode_, out, index)) {
            return false;
        }
        BarAction(index, out);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    bool WindowOperator::Close(ApiReplyInfo &out)
    {
        size_t index = 0;
        if (!IsOperational(CLOSE, window_.mode_, out, index)) {
            return false;
        }
        BarAction(index, out);
        return (out.exception_.code_ == ErrCode::NO_ERROR);
    }

    void WindowOperator::BarAction(size_t index, ApiReplyInfo &out)
    {
        CallBar(out);
        if (out.exception_.code_ != ErrCode::NO_ERROR) {
            return;
        }
        auto selector = WidgetSelector();
        auto matcher = WidgetAttrMatcher("index", std::to_string(index), EQ);
        selector.AddMatcher(matcher);
        vector<unique_ptr<Widget>> widgets;
        driver_.FindWidgets(selector, widgets, out.exception_, false);
        if (out.exception_.code_ != ErrCode::NO_ERROR) {
            return;
        }
        auto rect = (*widgets.at(0)).GetBounds();
        Point widgetCenter(rect.GetCenterX(), rect.GetCenterY());
        driver_.PerformClick(TouchOp::CLICK, widgetCenter, options_, out.exception_);
    }
} // namespace OHOS::uitest
