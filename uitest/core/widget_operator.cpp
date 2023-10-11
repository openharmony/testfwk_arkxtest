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

#include "widget_operator.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    WidgetOperator::WidgetOperator(UiDriver &driver, const Widget &widget, const UiOpArgs &options)
        : driver_(driver), widget_(widget), options_(options)
    {
    }

    void WidgetOperator::GenericClick(TouchOp op, ApiCallErr &error) const
    {
        DCHECK(op >= TouchOp::CLICK && op <= TouchOp::DOUBLE_CLICK_P);
        auto retrieved = driver_.RetrieveWidget(widget_, error, true);
        if (error.code_ != NO_ERROR) {
            return;
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY());
        auto touch = OHOS::uitest::GenericClick(op, center);
        driver_.PerformTouch(touch, options_, error);
    }

    void WidgetOperator::ScrollToEnd(bool toTop, ApiCallErr &error) const
    {
        while (true) {
            auto scrollWidget = driver_.RetrieveWidget(widget_, error);
            if (scrollWidget == nullptr || error.code_ != NO_ERROR) {
                return;
            }
            vector<string> visibleNodes;
            vector<string> allNodes;
            TreeSnapshotTaker snapshotTaker(visibleNodes, allNodes);
            driver_.DfsTraverseTree(snapshotTaker, scrollWidget);
            if (visibleNodes.empty() && allNodes.empty()) {
                return;
            }
            auto mark1 = toTop ? visibleNodes.front() : visibleNodes.back();
            auto mark2 = toTop ? allNodes.front() : allNodes.back();
            if (mark1 == mark2) {
                return;
            }
            TurnPage(toTop, error);
        }
    }

    void WidgetOperator::DragIntoWidget(const Widget &another, ApiCallErr &error) const
    {
        auto widgetFrom = driver_.RetrieveWidget(widget_, error);
        if (widgetFrom == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto widgetTo = driver_.RetrieveWidget(another, error, false);
        if (widgetTo == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto boundsFrom = widgetFrom->GetBounds();
        auto boundsTo = widgetTo->GetBounds();
        auto centerFrom = Point(boundsFrom.GetCenterX(), boundsFrom.GetCenterY());
        auto centerTo = Point(boundsTo.GetCenterX(), boundsTo.GetCenterY());
        auto touch = GenericSwipe(TouchOp::DRAG, centerFrom, centerTo);
        driver_.PerformTouch(touch, options_, error);
    }

    void WidgetOperator::PinchWidget(float_t scale, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto matcher = WidgetAttrMatcher(ATTR_NAMES[UiAttr::HIERARCHY], "ROOT", ValueMatchPattern::EQ);
        auto selector = WidgetSelector();
        selector.AddMatcher(matcher);
        vector<unique_ptr<Widget>> recv;
        driver_.FindWidgets(selector, recv, error, false);
        if (error.code_ != NO_ERROR) {
            return;
        }
        if (recv.empty()) {
            error = ApiCallErr(ERR_INTERNAL, "Cannot find root widget");
            return;
        }
        auto rootBound = recv.front()->GetBounds();
        auto rectBound = widget_.GetBounds();
        float_t widthScale = (float_t)(rootBound.GetWidth()) / (float_t)(rectBound.GetWidth());
        float_t heightScale = (float_t)(rootBound.GetHeight()) / (float_t)(rectBound.GetHeight());
        float_t originalScale = min(widthScale, heightScale);
        if (scale < 0) {
            error = ApiCallErr(ERR_INVALID_INPUT, "Please input the correct scale");
            return;
        } else if (scale > originalScale) {
            scale = originalScale;
        }
        auto touch = GenericPinch(rectBound, scale);
        driver_.PerformTouch(touch, options_, error);
    }

    void WidgetOperator::InputText(string_view text, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto origText = retrieved->GetAttr(ATTR_NAMES[UiAttr::TEXT], "");
        if (origText.empty() && text.empty()) {
            return;
        }
        static constexpr uint32_t focusTimeMs = 500;
        static constexpr uint32_t typeCharTimeMs = 50;
        vector<KeyEvent> events;
        if (!origText.empty()) {
            for (size_t index = 0; index < origText.size(); index++) {
                events.emplace_back(KeyEvent {ActionStage::DOWN, KEYCODE_DPAD_RIGHT, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, KEYCODE_DPAD_RIGHT, 0});
                events.emplace_back(KeyEvent {ActionStage::DOWN, KEYCODE_DEL, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, KEYCODE_DEL, 0});
            }
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY());
        auto touch = OHOS::uitest::GenericClick(TouchOp::CLICK, center);
        driver_.PerformTouch(touch, options_, error);
        driver_.DelayMs(focusTimeMs); // short delay to ensure focus gaining
        auto keyActionForDelete = KeysForwarder(events);
        driver_.TriggerKey(keyActionForDelete, options_, error);
        driver_.InputText(text, error);
    }

    unique_ptr<Widget> WidgetOperator::ScrollFindWidget(const WidgetSelector &selector, ApiCallErr &error) const
    {
        bool scrollingUp = true;
        vector<unique_ptr<Widget>> receiver;
        while (true) {
            auto scrollWidget = driver_.RetrieveWidget(widget_, error);
            if (scrollWidget == nullptr || error.code_ != NO_ERROR) {
                return nullptr;
            }
            driver_.FindWidgets(selector, receiver, error, false);
            if (!receiver.empty()) {
                auto first = receiver.begin();
                auto clone = (*first)->Clone("NONE", (*first)->GetHierarchy());
                // save the selection desc as dummy attribute
                clone->SetAttr("selectionDesc", selector.Describe());
                return clone;
            }
            vector<string> visibleNodes;
            vector<string> allNodes;
            TreeSnapshotTaker snapshotTaker(visibleNodes, allNodes);
            driver_.DfsTraverseTree(snapshotTaker, scrollWidget);
            if (visibleNodes.empty() && allNodes.empty()) {
                return nullptr;
            }
            auto mark1 = scrollingUp ? visibleNodes.front() : visibleNodes.back();
            auto mark2 = scrollingUp ? allNodes.front() : allNodes.back();
            if (mark1 == mark2) {
                // scrolling down to bottom, search completed with failure
                if (!scrollingUp) {
                    LOG_W("Scroll search widget failed: %{public}s", selector.Describe().data());
                    return nullptr;
                } else {
                    // scrolling down to top, change direction
                    scrollingUp = false;
                }
            }
            TurnPage(scrollingUp, error);
        }
    }

    void WidgetOperator::TurnPage(bool toTop, ApiCallErr &error) const
    {
        auto bounds = widget_.GetBounds();
        Point topPoint(bounds.GetCenterX(), bounds.top_);
        Point bottomPoint(bounds.GetCenterX(), bounds.bottom_);
        if (options_.scrollWidgetDeadZone_ > 0) {
            topPoint.py_ += options_.scrollWidgetDeadZone_;
            bottomPoint.py_ -= options_.scrollWidgetDeadZone_;
            auto screenSize = driver_.GetDisplaySize(error);
            auto gestureZone = screenSize.py_  / 20;
            if (screenSize.py_ - bounds.bottom_ <= gestureZone) {
                bottomPoint.py_ = bottomPoint.py_ - gestureZone;
            }
        }
        auto touch = (toTop) ? GenericSwipe(TouchOp::SWIPE, topPoint, bottomPoint) :
                               GenericSwipe(TouchOp::SWIPE, bottomPoint, topPoint);
        driver_.PerformTouch(touch, options_, error);
    }
} // namespace OHOS::uitest
