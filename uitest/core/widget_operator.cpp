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

    class KeysForwarder : public KeyAction {
    public:
        explicit KeysForwarder(const vector<KeyEvent> &evetns) : events_(evetns) {};

        void ComputeEvents(vector<KeyEvent> &recv, const UiOpArgs &opt) const override
        {
            recv = events_;
        }

        std::string Describe() const override
        {
            return "KeysForwarder";
        }

    private:
        const vector<KeyEvent> &events_;
    };

    class TreeSnapshotTaker : public WidgetVisitor {
    public:
        explicit TreeSnapshotTaker(stringstream &receiver) : receiver_(receiver) {};

        ~TreeSnapshotTaker() {}

        void Visit(const Widget &widget) override
        {
            receiver_ << widget.GetAttr(ATTR_NAMES[UiAttr::TYPE], "") << "/";
            receiver_ << widget.GetAttr(ATTR_NAMES[UiAttr::TEXT], "") << ";";
        }

    private:
        stringstream &receiver_;
    };

    static string TakeScopeUiSnapshot(UiDriver& driver, const Widget &root)
    {
        stringstream os;
        TreeSnapshotTaker snapshotTaker(os);
        driver.GetWidgetTree()->DfsTraverseDescendants(snapshotTaker, root);
        return os.str();
    }

    WidgetOperator::WidgetOperator(UiDriver &driver, const Widget &widget, const UiOpArgs &options)
        : driver_(driver), widget_(widget), options_(options)
    {
    }

    void WidgetOperator::GenericClick(TouchOp op, ApiCallErr &error) const
    {
        DCHECK(op >= TouchOp::CLICK && op <= TouchOp::DOUBLE_CLICK_P);
        auto retrieved = driver_.RetrieveWidget(widget_, error, true);
        if (error.code_ != ErrCode::NO_ERROR) {
            return;
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY());
        driver_.PerformClick(op, center, options_, error);
    }

    void WidgetOperator::ScrollToEnd(bool toTop, ApiCallErr &error) const
    {
        string prevSnapshot;
        while (true) {
            auto scrollWidget = driver_.RetrieveWidget(widget_, error);
            if (scrollWidget == nullptr || error.code_ != NO_ERROR) {
                return;
            }
            string snapshot = TakeScopeUiSnapshot(driver_, *scrollWidget);
            if (snapshot == prevSnapshot) {
                return;
            }
            prevSnapshot = snapshot;
            auto bounds = scrollWidget->GetBounds();
            if (options_.scrollWidgetDeadZone_ > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += options_.scrollWidgetDeadZone_;
                bounds.bottom_ -= options_.scrollWidgetDeadZone_;
            }
            Point topPoint(bounds.GetCenterX(), bounds.top_), bottomPoint(bounds.GetCenterX(), bounds.bottom_);
            if (toTop) {
                driver_.PerformSwipe(TouchOp::SWIPE, topPoint, bottomPoint, options_, error);
            } else {
                driver_.PerformSwipe(TouchOp::SWIPE, bottomPoint, topPoint, options_, error);
            }
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
        driver_.PerformSwipe(TouchOp::DRAG, centerFrom, centerTo, options_, error);
    }

    void WidgetOperator::pinchWidget(float_t scale, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto matcher = WidgetAttrMatcher(ATTR_NAMES[UiAttr::HIERARCHY], "ROOT", ValueMatchPattern::EQ);
        auto selector = WidgetSelector();
        selector.AddMatcher(matcher);
        vector<unique_ptr<Widget>> recv;
        driver_.FindWidgets(selector, recv, error);
        if (error.code_ != ErrCode::NO_ERROR) {
            return;
        }
        if (recv.empty()) {
            error = ApiCallErr(INTERNAL_ERROR, "Cannot find root widget");
                return;
            }
        auto rootBound = recv.front()->GetBounds();
        auto rectBound = widget_.GetBounds();
        auto widthScale = (float_t)(rootBound.GetWidth() / rectBound.GetWidth());
        auto heightScale = (float_t)(rootBound.GetHeight() / rectBound.GetHeight());
        auto originalScale = min(widthScale, heightScale);
        if (scale < 0) {
            error = ApiCallErr(USAGE_ERROR, "Please input the correct scale");
            return;
        } else if (scale > originalScale) {
            scale = originalScale;
        }
        driver_.PerformPinch(TouchOp::PINCH, rectBound, scale, options_, error);
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
                events.emplace_back(KeyEvent {ActionStage::DOWN, 2015, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, 2015, 0});
                events.emplace_back(KeyEvent {ActionStage::DOWN, 2055, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, 2055, 0});
            }
        }
        if (!text.empty()) {
            vector<char> chars(text.begin(), text.end()); // decompose to sing-char input sequence
            vector<pair<int32_t, int32_t>> keyCodes;
            for (auto ch : chars) {
                int32_t code = KEYCODE_NONE;
                int32_t ctrlCode = KEYCODE_NONE;
                if (!driver_.GetUiController()->GetCharKeyCode(ch, code, ctrlCode)) {
                    error = ApiCallErr(USAGE_ERROR, string("Cannot input char ") + ch);
                    return;
                }
                keyCodes.emplace_back(make_pair(code, ctrlCode));
            }
            for (auto &pair : keyCodes) {
                if (pair.second != KEYCODE_NONE) {
                    events.emplace_back(KeyEvent {ActionStage::DOWN, pair.second, 0});
                }
                events.emplace_back(KeyEvent {ActionStage::DOWN, pair.first, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, pair.first, 0});
                if (pair.second != KEYCODE_NONE) {
                    events.emplace_back(KeyEvent {ActionStage::UP, pair.second, 0});
                }
            }
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY());
        driver_.PerformClick(TouchOp::CLICK, center, options_, error);
        driver_.DelayMs(focusTimeMs); // short delay to ensure focus gaining
        auto keyAction = KeysForwarder(events);
        driver_.TriggerKey(keyAction, options_, error);
    }

    unique_ptr<Widget> WidgetOperator::ScrollFindWidget(const WidgetSelector &selector, ApiCallErr &error) const
    {
        PointerMatrix scrollEvents;
        bool scrollingUp = true;
        string prevSnapshot;
        vector<reference_wrapper<const Widget>> receiver;
        while (true) {
            auto scrollWidget = driver_.RetrieveWidget(widget_, error);
            if (scrollWidget == nullptr || error.code_ != NO_ERROR) {
                return nullptr;
            }
            selector.Select(*(driver_.GetWidgetTree()), receiver);
            if (!receiver.empty()) {
                auto& first = receiver.front().get();
                auto clone = first.Clone("NONE", first.GetHierarchy());
                // save the selection desc as dummy attribute
                clone->SetAttr("selectionDesc", selector.Describe());
                return clone;
            }
            string snapshot = TakeScopeUiSnapshot(driver_, *scrollWidget);
            if (snapshot == prevSnapshot) {
                // scrolling down to bottom, search completed with failure
                if (!scrollingUp) {
                    auto msg = string("Scroll search widget failed: ") + selector.Describe();
                    LOG_W("%{public}s", msg.c_str());
                    return nullptr;
                } else {
                    // scrolling down to top, change direction
                    scrollingUp = false;
                }
            }
            prevSnapshot = snapshot;
            // execute scrolling on the scroll_widget without update UI
            auto bounds = scrollWidget->GetBounds();
            if (options_.scrollWidgetDeadZone_ > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += options_.scrollWidgetDeadZone_;
                bounds.bottom_ -= options_.scrollWidgetDeadZone_;
            }
            Point topPoint(bounds.GetCenterX(), bounds.top_), bottomPoint(bounds.GetCenterX(), bounds.bottom_);
            if (scrollingUp) {
                driver_.PerformSwipe(TouchOp::SWIPE, topPoint, bottomPoint, options_, error);
            } else {
                driver_.PerformSwipe(TouchOp::SWIPE, bottomPoint, topPoint, options_, error);
            }
        }
    }
} // namespace OHOS::uitest
