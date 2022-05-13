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

#include <future>
#include "ui_driver.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static constexpr string_view DUMMY_ATTRNAME_SELECTION = "selectionDesc";

    class TreeSnapshotTaker : public WidgetVisitor {
    public:
        explicit TreeSnapshotTaker(stringstream &receiver) : receiver_(receiver){};

        ~TreeSnapshotTaker() {}

        void Visit(const Widget &widget) override
        {
            receiver_ << widget.GetAttr(ATTR_NAMES[UiAttr::TYPE], "") << "/";
            receiver_ << widget.GetAttr(ATTR_NAMES[UiAttr::TEXT], "") << ";";
        }

    private:
        stringstream &receiver_;
    };

    void UiDriver::UpdateUi(bool updateUiTree, ApiCallErr &error)
    {
        UiController::InstallFromProvider();
        uiController_ = UiController::GetController();
        if (uiController_ == nullptr) {
            LOG_E("%{public}s", "No available UiController currently");
            error = ApiCallErr(INTERNAL_ERROR, "No available UiController currently");
            return;
        }
        if (!updateUiTree) {
            return;
        }
        widgetTree_ = make_unique<WidgetTree>("");
        auto domData = json();
        uiController_->GetCurrentUiDom(domData);
        if (domData.type() == nlohmann::detail::value_t::null || domData.empty()) {
            LOG_E("%{public}s", "Get window nodes failed");
            error = ApiCallErr(INTERNAL_ERROR, "Get window nodes failed");
            return;
        }
        widgetTree_->ConstructFromDom(domData, true);
    }

    static unique_ptr<Widget> CloneFreeWidget(const Widget &from, const WidgetSelector &selector)
    {
        auto clone = make_unique<Widget>(from.GetHierarchy());
        map<string, string> attributes;
        from.DumpAttributes(attributes);
        for (auto &[name, value] : attributes) {
            clone->SetAttr(name, value);
        }
        const auto bounds = from.GetBounds();
        clone->SetBounds(bounds.left_, bounds.right_, bounds.top_, bounds.bottom_);
        // belongs to no WidgetTree
        clone->SetHostTreeId("NONE");
        // save the selection desc as dummy attribute
        clone->SetAttr(DUMMY_ATTRNAME_SELECTION, selector.Describe());
        return clone;
    }

    const Widget *UiDriver::RetrieveWidget(const Widget &widget, ApiCallErr &err, bool updateUi)
    {
        if (updateUi) {
            UpdateUi(true, err);
            if (err.code_ != NO_ERROR) {
                return nullptr;
            }
        }
        // retrieve widget by hashcode or by hierarchy
        constexpr auto attrHashCode = ATTR_NAMES[UiAttr::HASHCODE];
        constexpr auto attrHierarchy = ATTR_NAMES[UiAttr::HIERARCHY];
        auto hashcodeMatcher = WidgetAttrMatcher(attrHashCode, widget.GetAttr(attrHashCode, "NA"), EQ);
        auto hierarchyMatcher = WidgetAttrMatcher(attrHierarchy, widget.GetHierarchy(), EQ);
        auto anyMatcher = Any(hashcodeMatcher, hierarchyMatcher);
        vector<reference_wrapper<const Widget>> recv;
        auto visitor = MatchedWidgetCollector(anyMatcher, recv);
        widgetTree_->DfsTraverse(visitor);
        stringstream msg;
        msg << "Widget: " << widget.GetAttr(DUMMY_ATTRNAME_SELECTION, "");
        msg << "dose not exist on current UI! Check if the UI has changed after you got the widget object";
        if (recv.empty()) {
            msg << "(NoCandidates)";
            LOG_W("%{public}s", msg.str().c_str());
            err = ApiCallErr(WIDGET_LOST, msg.str());
            return nullptr;
        }
        DCHECK(recv.size() == 1);
        auto &retrieved = recv.at(0).get();
        // confirm type
        constexpr auto attrType = ATTR_NAMES[UiAttr::TYPE];
        if (widget.GetAttr(attrType, "A").compare(retrieved.GetAttr(attrType, "B")) != 0) {
            msg << " (CompareEqualsFailed)";
            LOG_W("%{public}s", msg.str().c_str());
            err = ApiCallErr(WIDGET_LOST, msg.str());
            return nullptr;
        }
        return &retrieved;
    }

    static void InjectGenericClick(PointerOp type, const Point &point, const UiController &controller,
                                   const UiOpArgs &options)
    {
        auto action = GenericClick(type);
        vector<TouchEvent> events;
        action.Decompose(events, point, options);
        if (events.empty()) {
            return;
        }
        controller.InjectTouchEventSequence(events);
        controller.WaitForUiSteady(options.uiSteadyThresholdMs_, options.waitUiSteadyMaxMs_);
    }

    static void InjectGenericSwipe(PointerOp type, const Point &point0, const Point &point1,
                                   const UiController &controller, const UiOpArgs &options)
    {
        auto action = GenericSwipe(type);
        vector<TouchEvent> events;
        action.Decompose(events, point0, point1, options);
        if (events.empty()) {
            return;
        }
        controller.InjectTouchEventSequence(events);
        controller.WaitForUiSteady(options.uiSteadyThresholdMs_, options.waitUiSteadyMaxMs_);
    }

    static void InjectKeyAction(const KeyAction &action, const UiController &controller, const UiOpArgs &options)
    {
        vector<KeyEvent> events;
        action.ComputeEvents(events, options);
        if (events.empty()) {
            return;
        }
        controller.InjectKeyEventSequence(events);
        controller.WaitForUiSteady(options.uiSteadyThresholdMs_, options.waitUiSteadyMaxMs_);
    }

    void UiDriver::TriggerKey(const KeyAction &key, const UiOpArgs &opt, ApiCallErr &error)
    {
        UpdateUi(false, error);
        if (error.code_ != NO_ERROR) {
            return;
        }
        InjectKeyAction(key, *uiController_, opt);
    }

    void UiDriver::OperateWidget(const Widget &widget, WidgetOp op, const UiOpArgs &opt, ApiCallErr &error)
    {
        auto retrieved = RetrieveWidget(widget, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        const auto bounds = retrieved->GetBounds();
        const int32_t cx = bounds.GetCenterX();
        const int32_t cy = bounds.GetCenterY();
        switch (op) {
            case WidgetOp::CLICK:
                InjectGenericClick(PointerOp::CLICK_P, {cx, cy}, *uiController_, opt);
                break;
            case WidgetOp::LONG_CLICK:
                InjectGenericClick(PointerOp::LONG_CLICK_P, {cx, cy}, *uiController_, opt);
                break;
            case WidgetOp::DOUBLE_CLICK:
                InjectGenericClick(PointerOp::DOUBLE_CLICK_P, {cx, cy}, *uiController_, opt);
                break;
            case WidgetOp::SCROLL_TO_TOP:
            case WidgetOp::SCROLL_TO_BOTTOM:
                ScrollToEnd(widget, op == WidgetOp::SCROLL_TO_TOP, opt, error);
                break;
        }
    }

    void UiDriver::InputText(const Widget &widget, string_view text, const UiOpArgs &opt, ApiCallErr &error)
    {
        auto retrieved = RetrieveWidget(widget, error);
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
                events.emplace_back(KeyEvent{ActionStage::DOWN, 2015, typeCharTimeMs});
                events.emplace_back(KeyEvent{ActionStage::UP, 2015, 0});
                events.emplace_back(KeyEvent{ActionStage::DOWN, 2055, typeCharTimeMs});
                events.emplace_back(KeyEvent{ActionStage::UP, 2055, 0});
            }
        }
        if (!text.empty()) {
            vector<char> chars(text.begin(), text.end()); // decompose to sing-char input sequence
            vector<pair<int32_t, int32_t>> keyCodes;
            for (auto ch : chars) {
                int32_t code = KEYCODE_NONE;
                int32_t ctrlCode = KEYCODE_NONE;
                if (!uiController_->GetCharKeyCode(ch, code, ctrlCode)) {
                    error = ApiCallErr(USAGE_ERROR, string("Cannot input char ") + ch);
                    return;
                }
                keyCodes.emplace_back(make_pair(code, ctrlCode));
            }
            for (auto &pair : keyCodes) {
                if (pair.second != KEYCODE_NONE) {
                    events.emplace_back(KeyEvent{ActionStage::DOWN, pair.second, 0});
                }
                events.emplace_back(KeyEvent{ActionStage::DOWN, pair.first, typeCharTimeMs});
                events.emplace_back(KeyEvent{ActionStage::UP, pair.first, 0});
                if (pair.second != KEYCODE_NONE) {
                    events.emplace_back(KeyEvent{ActionStage::UP, pair.second, 0});
                }
            }
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY());
        InjectGenericClick(PointerOp::CLICK_P, center, *uiController_, opt);
        DelayMs(focusTimeMs); // short delay to ensure focus gaining
        uiController_->InjectKeyEventSequence(events);
        events.clear();
        uiController_->WaitForUiSteady(opt.uiSteadyThresholdMs_, opt.waitUiSteadyMaxMs_);
    }

    static string TakeScopeUiSnapshot(const WidgetTree &tree, const Widget &root)
    {
        stringstream os;
        TreeSnapshotTaker snapshotTaker(os);
        tree.DfsTraverseDescendants(snapshotTaker, root);
        return os.str();
    }

    unique_ptr<Widget> UiDriver::ScrollSearch(const Widget &widget, const WidgetSelector &selector,
                                              const UiOpArgs &opt, ApiCallErr &err)
    {
        vector<TouchEvent> scrollEvents;
        bool scrollingUp = true;
        string prevSnapshot;
        vector<reference_wrapper<const Widget>> receiver;
        while (true) {
            auto scrollWidget = RetrieveWidget(widget, err);
            if (scrollWidget == nullptr || err.code_ != NO_ERROR) {
                return nullptr;
            }
            selector.Select(*widgetTree_, receiver);
            if (!receiver.empty()) {
                return CloneFreeWidget(receiver.at(0), selector);
            }
            string snapshot = TakeScopeUiSnapshot(*widgetTree_, *scrollWidget);
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
            if (opt.scrollWidgetDeadZone_ > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += opt.scrollWidgetDeadZone_;
                bounds.bottom_ -= opt.scrollWidgetDeadZone_;
            }
            Point topPoint(bounds.GetCenterX(), bounds.top_), bottomPoint(bounds.GetCenterX(), bounds.bottom_);
            if (scrollingUp) {
                InjectGenericSwipe(PointerOp::SWIPE_P, topPoint, bottomPoint, *uiController_, opt);
            } else {
                InjectGenericSwipe(PointerOp::SWIPE_P, bottomPoint, topPoint, *uiController_, opt);
            }
        }
    }

    void UiDriver::ScrollToEnd(const Widget &widget, bool scrollUp, const UiOpArgs &opt, ApiCallErr &err)
    {
        string prevSnapshot;
        while (true) {
            auto scrollWidget = RetrieveWidget(widget, err);
            if (scrollWidget == nullptr || err.code_ != NO_ERROR) {
                return;
            }
            string snapshot = TakeScopeUiSnapshot(*widgetTree_, *scrollWidget);
            if (snapshot == prevSnapshot) {
                return;
            }
            prevSnapshot = snapshot;
            auto bounds = scrollWidget->GetBounds();
            if (opt.scrollWidgetDeadZone_ > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += opt.scrollWidgetDeadZone_;
                bounds.bottom_ -= opt.scrollWidgetDeadZone_;
            }
            Point topPoint(bounds.GetCenterX(), bounds.top_), bottomPoint(bounds.GetCenterX(), bounds.bottom_);
            if (scrollUp) {
                InjectGenericSwipe(PointerOp::SWIPE_P, topPoint, bottomPoint, *uiController_, opt);
            } else {
                InjectGenericSwipe(PointerOp::SWIPE_P, bottomPoint, topPoint, *uiController_, opt);
            }
        }
    }

    void UiDriver::DragIntoWidget(const Widget &wa, const Widget &wb, const UiOpArgs &opt, ApiCallErr &err)
    {
        auto widgetFrom = RetrieveWidget(wa, err);
        if (widgetFrom == nullptr || err.code_ != NO_ERROR) {
            return;
        }
        auto widgetTo = RetrieveWidget(wb, err, false);
        if (widgetTo == nullptr || err.code_ != NO_ERROR) {
            return;
        }
        auto boundsFrom = widgetFrom->GetBounds();
        auto boundsTo = widgetTo->GetBounds();
        auto centerFrom = Point{boundsFrom.GetCenterX(), boundsFrom.GetCenterY()};
        auto centerTo = Point{boundsTo.GetCenterX(), boundsTo.GetCenterY()};
        InjectGenericSwipe(PointerOp::DRAG_P, centerFrom, centerTo, *uiController_, opt);
    }

    void UiDriver::FindWidgets(const WidgetSelector &select, vector<unique_ptr<Widget>> &rev, ApiCallErr &err)
    {
        UpdateUi(true, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        vector<reference_wrapper<const Widget>> widgets;
        select.Select(*widgetTree_, widgets);
        // covert widgets to images as return value
        uint32_t index = 0;
        for (auto &ref : widgets) {
            auto image = CloneFreeWidget(ref.get(), select);
            // at sometime, more than one widgets are found, add the node index to the description
            auto selectionDesc = select.Describe() + "(index=" + to_string(index) + ")";
            image->SetAttr(DUMMY_ATTRNAME_SELECTION, selectionDesc);
            rev.emplace_back(move(image));
            index++;
        }
    }

    unique_ptr<Widget> UiDriver::WaitForWidget(const WidgetSelector &select, const UiOpArgs &opt, ApiCallErr &err)
    {
        const uint32_t sliceMs = 20;
        const auto startMs = GetCurrentMillisecond();
        vector<unique_ptr<Widget>> receiver;
        do {
            FindWidgets(select, receiver, err);
            if (err.code_ != NO_ERROR) { // abort on error
                return nullptr;
            }
            if (!receiver.empty()) {
                return move(receiver.at(0));
            }
            DelayMs(sliceMs);
        } while (GetCurrentMillisecond() - startMs < opt.waitWidgetMaxMs_);
        return nullptr;
    }

    unique_ptr<Widget> UiDriver::GetWidgetSnapshot(Widget &widget, ApiCallErr &error)
    {
        auto retrieved = RetrieveWidget(widget, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return nullptr;
        }
        auto selector = WidgetSelector(); // dummy
        auto ptr =  CloneFreeWidget(*retrieved, selector);
        ptr->SetAttr(DUMMY_ATTRNAME_SELECTION, widget.GetAttr(DUMMY_ATTRNAME_SELECTION, ""));
        return ptr;
    }

    void UiDriver::DelayMs(uint32_t ms)
    {
        if (ms > 0) {
            this_thread::sleep_for(chrono::milliseconds(ms));
        }
    }

    void UiDriver::PerformClick(PointerOp op, const Point &point, const UiOpArgs &opt, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        InjectGenericClick(op, point, *uiController_, opt);
    }

    void UiDriver::PerformSwipe(PointerOp op, const Point &from, const Point &to, const UiOpArgs &opt, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        InjectGenericSwipe(op, from, to, *uiController_, opt);
    }

    void UiDriver::TakeScreenCap(string_view savePath, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        stringstream errorRecv;
        if (!uiController_->TakeScreenCap(savePath, errorRecv)) {
            LOG_W("ScreenCap failed: %{public}s", errorRecv.str().c_str());
        } else {
            LOG_D("ScreenCap saved to %{public}s", savePath.data());
        }
    }
} // namespace OHOS::uitest
