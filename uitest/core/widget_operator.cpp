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

    private:
        const vector<KeyEvent> &events_;
    };

    class TreeSnapshotTaker : public WidgetVisitor {
    public:
        explicit TreeSnapshotTaker(string &receiver, vector<string> &leafNodes) : receiver_(receiver),
            leafNodes_(leafNodes) {};

        ~TreeSnapshotTaker() {}

        void Visit(const Widget &widget) override
        {
            auto type = widget.GetAttr(ATTR_NAMES[UiAttr::TYPE], "") + "/";
            auto value =  widget.GetAttr(ATTR_NAMES[UiAttr::TEXT], "") + "/";
            auto hashcode =  widget.GetAttr(ATTR_NAMES[UiAttr::HASHCODE], "") + ";";
            receiver_ = receiver_ + type + value + hashcode;
            if (value != "/") {
                leafNodes_.push_back(type + value + hashcode);
            }
        }

    private:
        string &receiver_;
        vector<string> &leafNodes_;
    };

    static void TakeScopeUiSnapshot(const UiDriver &driver, const Widget &root, string &snapshot,
        vector<string> &leafNodes)
    {
        TreeSnapshotTaker snapshotTaker(snapshot, leafNodes);
        auto tree = driver.GetWidgetTree();
        if (tree != nullptr) {
            tree->DfsTraverseDescendants(snapshotTaker, root);
        } else {
            LOG_W("WidgetTree is a nullptr currently");
        }
    }

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
        string prevSnapshot = "";
        string targetSnapshot = "";
        while (true) {
            auto scrollWidget = driver_.RetrieveWidget(widget_, error);
            if (scrollWidget == nullptr || error.code_ != NO_ERROR) {
                return;
            }
            string snapshot;
            vector<string> leafNodes;
            TakeScopeUiSnapshot(driver_, *scrollWidget, snapshot, leafNodes);
            if ((prevSnapshot == snapshot) || (snapshot.find(targetSnapshot) != string::npos && targetSnapshot != "")) {
                return;
            }
            prevSnapshot = snapshot;
            if (!leafNodes.empty()) {
                targetSnapshot = (toTop ? leafNodes.front() : leafNodes.back());
            } else {
                targetSnapshot = "";
            }
            auto bounds = scrollWidget->GetBounds();
            if (options_.scrollWidgetDeadZone_ > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += options_.scrollWidgetDeadZone_;
                bounds.bottom_ -= options_.scrollWidgetDeadZone_;
            }
            Point topPoint(bounds.GetCenterX(), bounds.top_);
            Point bottomPoint(bounds.GetCenterX(), bounds.bottom_);
            if (toTop) {
                auto touch = GenericSwipe(TouchOp::SWIPE, topPoint, bottomPoint);
                driver_.PerformTouch(touch, options_, error);
            } else {
                auto touch = GenericSwipe(TouchOp::SWIPE, bottomPoint, topPoint);
                driver_.PerformTouch(touch, options_, error);
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
                if (!driver_.GetUiController(error)->GetCharKeyCode(ch, code, ctrlCode)) {
                    error = ApiCallErr(ERR_INVALID_INPUT, string("Cannot input char ") + ch);
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
        auto touch = OHOS::uitest::GenericClick(TouchOp::CLICK, center);
        driver_.PerformTouch(touch, options_, error);
        driver_.DelayMs(focusTimeMs); // short delay to ensure focus gaining
        auto keyAction = KeysForwarder(events);
        driver_.TriggerKey(keyAction, options_, error);
    }

    unique_ptr<Widget> WidgetOperator::ScrollFindWidget(const WidgetSelector &selector, ApiCallErr &error) const
    {
        bool scrollingUp = true;
        string prevSnapshot = "";
        string targetSnapshot = "";
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
            string snapshot;
            vector<string> leafNodes;
            TakeScopeUiSnapshot(driver_, *scrollWidget, snapshot, leafNodes);
            if ((snapshot == prevSnapshot) || (snapshot.find(targetSnapshot) != string::npos && targetSnapshot != "")) {
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
            if (!leafNodes.empty()) {
                targetSnapshot = (scrollingUp ? leafNodes.front() : leafNodes.back());
            } else {
                targetSnapshot = "";
            }
            // execute scrolling on the scroll_widget without update UI
            auto bounds = scrollWidget->GetBounds();
            if (options_.scrollWidgetDeadZone_ > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += options_.scrollWidgetDeadZone_;
                bounds.bottom_ -= options_.scrollWidgetDeadZone_;
            }
            Point topPoint(bounds.GetCenterX(), bounds.top_);
            Point bottomPoint(bounds.GetCenterX(), bounds.bottom_);
            auto touch = (scrollingUp) ? GenericSwipe(TouchOp::SWIPE, topPoint, bottomPoint) :
                                         GenericSwipe(TouchOp::SWIPE, bottomPoint, topPoint);
            driver_.PerformTouch(touch, options_, error);
        }
    }
} // namespace OHOS::uitest
