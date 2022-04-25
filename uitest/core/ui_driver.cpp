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

    void UiDriver::UpdateUi(bool updateUiTree, ApiCallErr &error)
    {
        UiController::InstallForDevice(deviceName_);
        uiController_ = UiController::GetController(deviceName_);
        if (uiController_ == nullptr) {
            LOG_E("%{public}s", (string("No available UiController for device: ") + string(deviceName_)).c_str());
            error = ApiCallErr(INTERNAL_ERROR, "No available UiController currently");
            return;
        }
        if (!updateUiTree) {
            return;
        }
        widgetTree_ = make_unique<WidgetTree>("");
        auto domData = json();
        uiController_->GetCurrentUiDom(domData);
        widgetTree_->ConstructFromDom(domData, true);
    }

    /**Inflate widget-image attributes from the given widget-object and the selector.*/
    static void Widget2Image(const Widget &widget, WidgetImage &image, const WidgetSelector &selector)
    {
        map<string, string> attributes;
        widget.DumpAttributes(attributes);
        image.SetAttributes(attributes);
        image.SetSelectionDesc(selector.Describe());
    }

    const Widget *UiDriver::RetrieveWidget(const WidgetImage &img, ApiCallErr &err, bool updateUi)
    {
        if (updateUi) {
            UpdateUi(true, err);
            if (err.code_ != NO_ERROR) {
                return nullptr;
            }
        }
        // retrieve widget by hashcode or by hierarchy
        auto hashcodeMatcher = WidgetAttrMatcher(ATTR_HASHCODE, img.GetHashCode(), EQ);
        auto hierarchyMatcher = WidgetAttrMatcher(ATTR_HIERARCHY, img.GetHierarchy(), EQ);
        auto anyMatcher = Any(hashcodeMatcher, hierarchyMatcher);
        vector<reference_wrapper<const Widget>> recv;
        auto visitor = MatchedWidgetCollector(anyMatcher, recv);
        widgetTree_->DfsTraverse(visitor);
        stringstream msg;
        msg << "Widget: " << img.GetSelectionDesc();
        msg << "dose not exist on current UI! Check if the UI has changed after you got the widget object";
        if (recv.empty()) {
            msg << "(NoCandidates)";
            LOG_W("%{public}s", msg.str().c_str());
            err = ApiCallErr(WIDGET_LOST, msg.str());
            return nullptr;
        }
        DCHECK(recv.size() == 1);
        auto &widget = recv.at(0).get();
        // check equality
        WidgetImage newImage = WidgetImage();
        WidgetSelector selector; // dummy selector
        Widget2Image(widget, newImage, selector);
        if (!img.Compare(newImage)) {
            msg << " (CompareEqualsFailed)";
            LOG_W("%{public}s", msg.str().c_str());
            err = ApiCallErr(WIDGET_LOST, msg.str());
            return nullptr;
        }
        return &widget;
    }

    static void InjectGenericClick(PointerOp type, const Point &point, const UiController &controller,
                                   const UiDriveOptions &options)
    {
        auto action = GenericClick(type);
        vector<TouchEvent> events;
        action.Decompose(events, point, options);
        if (events.empty()) { return; }
        controller.InjectTouchEventSequence(events);
        controller.WaitForUiSteady(options.uiSteadyThresholdMs_, options.waitUiSteadyMaxMs_);
    }

    static void InjectGenericSwipe(PointerOp type, const Point &point0, const Point &point1,
                                   const UiController &controller, const UiDriveOptions &options)
    {
        auto action = GenericSwipe(type);
        vector<TouchEvent> events;
        action.Decompose(events, point0, point1, options);
        if (events.empty()) { return; }
        controller.InjectTouchEventSequence(events);
        controller.WaitForUiSteady(options.uiSteadyThresholdMs_, options.waitUiSteadyMaxMs_);
    }

    /**Convert WidgetOperation to PointerActions and do injection.*/
    static void InjectWidgetOperate(const Rect &bounds, WidgetOp operate, const UiController &controller,
                                    const UiDriveOptions &options)
    {
        const int32_t cx = bounds.GetCenterX();
        const int32_t cy = bounds.GetCenterY();
        switch (operate) {
            case WidgetOp::CLICK:
                InjectGenericClick(PointerOp::CLICK_P, {cx, cy}, controller, options);
                break;
            case WidgetOp::LONG_CLICK:
                InjectGenericClick(PointerOp::LONG_CLICK_P, {cx, cy}, controller, options);
                break;
            case WidgetOp::DOUBLE_CLICK:
                InjectGenericClick(PointerOp::DOUBLE_CLICK_P, {cx, cy}, controller, options);
                break;
            case WidgetOp::SWIPE_L2R:
                InjectGenericSwipe(PointerOp::SWIPE_P, {bounds.left_, cy}, {bounds.right_, cy}, controller, options);
                break;
            case WidgetOp::SWIPE_R2L:
                InjectGenericSwipe(PointerOp::SWIPE_P, {bounds.right_, cy}, {bounds.left_, cy}, controller, options);
                break;
            case WidgetOp::SWIPE_T2B:
                InjectGenericSwipe(PointerOp::SWIPE_P, {cx, bounds.top_}, {cx, bounds.bottom_}, controller, options);
                break;
            case WidgetOp::SWIPE_B2T:
                InjectGenericSwipe(PointerOp::SWIPE_P, {cx, bounds.bottom_}, {cx, bounds.top_}, controller, options);
                break;
        }
    }

    static void InjectKeyAction(const KeyAction &action, const UiController &controller, const UiDriveOptions &options)
    {
        vector<KeyEvent> events;
        action.ComputeEvents(events, options);
        if (events.empty()) { return; }
        controller.InjectKeyEventSequence(events);
        controller.WaitForUiSteady(options.uiSteadyThresholdMs_, options.waitUiSteadyMaxMs_);
    }

    const Widget *UiDriver::FindScrollWidget(const WidgetImage &img) const
    {
        vector<reference_wrapper<const Widget>> recv;
        static constexpr string_view attrType = ATTR_NAMES[UiAttr::TYPE];
        // scrollable widget usually has unique type on a UI frame, some find it by type
        auto typeMatcher = WidgetAttrMatcher(attrType, img.GetAttribute(attrType, ""), EQ);
        auto visitor = MatchedWidgetCollector(typeMatcher, recv);
        widgetTree_->DfsTraverse(visitor);
        if (recv.empty()) {
            return nullptr;
        }
        return &(recv.at(0).get());
    }

    UiDriver::UiDriver(string_view device) : deviceName_(device) {}

    void UiDriver::TriggerKey(const KeyAction &action, ApiCallErr &error)
    {
        UpdateUi(false, error);
        if (error.code_ != NO_ERROR) {
            return;
        }
        InjectKeyAction(action, *uiController_, options_);
    }

    void UiDriver::PerformWidgetOperate(const WidgetImage &image, WidgetOp type, ApiCallErr &error)
    {
        auto widget = RetrieveWidget(image, error);
        if (widget == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        InjectWidgetOperate(widget->GetBounds(), type, *uiController_, options_);
    }

    void UiDriver::InputText(const WidgetImage &image, string_view text, ApiCallErr &error)
    {
        auto widget = RetrieveWidget(image, error);
        if (widget == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto origText = widget->GetAttr(ATTR_NAMES[UiAttr::TEXT], "");
        if (origText.empty() && text.empty()) {
            return;
        }
        static constexpr uint32_t focusTimeMs = 500;
        static constexpr uint32_t typeCharTimeMs = 50;
        vector<KeyEvent> events;
        if (!origText.empty()) {
            for (int index = 0; index < origText.size(); index++) {
                events.emplace_back(KeyEvent {ActionStage::DOWN, 2015, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, 2015, 0});
                events.emplace_back(KeyEvent {ActionStage::DOWN, 2055, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, 2055, 0});
            }
        }
        if (!text.empty()) {
            vector<char> chars(text.begin(), text.end()); // decompose to sing-char input sequence
            vector<pair<int32_t, int32_t>> keyCodes;
            for (auto ch: chars) {
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
                    events.emplace_back(KeyEvent {ActionStage::DOWN, pair.second, 0});
                }
                events.emplace_back(KeyEvent {ActionStage::DOWN, pair.first, typeCharTimeMs});
                events.emplace_back(KeyEvent {ActionStage::UP, pair.first, 0});
                if (pair.second != KEYCODE_NONE) {
                    events.emplace_back(KeyEvent {ActionStage::UP, pair.second, 0});
                }
            }
        }
        InjectWidgetOperate(widget->GetBounds(), WidgetOp::CLICK, *uiController_, options_);
        DelayMs(focusTimeMs); // short delay to ensure focus gaining
        uiController_->InjectKeyEventSequence(events);
        events.clear();
        uiController_->WaitForUiSteady(options_.uiSteadyThresholdMs_, options_.waitUiSteadyMaxMs_);
    }

    static string TakeScopeUiSnapshot(const WidgetTree &tree, const Widget &root)
    {
        stringstream os;
        TreeSnapshotTaker snapshotTaker(os);
        tree.DfsTraverseDescendants(snapshotTaker, root);
        return os.str();
    }

    unique_ptr<WidgetImage> UiDriver::ScrollSearch(const WidgetImage &img, const WidgetSelector &selector,
                                                   ApiCallErr &err, int32_t deadZoneSize)
    {
        vector<TouchEvent> scrollEvents;
        bool scrollingUp = true;
        string prevSnapshot;
        vector<reference_wrapper<const Widget>> receiver;
        while (true) {
            auto scrollWidget = RetrieveWidget(img, err);
            if (scrollWidget == nullptr) {
                scrollWidget = FindScrollWidget(img);
                if (scrollWidget != nullptr) {
                    err = ApiCallErr(NO_ERROR);
                }
            }
            if (scrollWidget == nullptr || err.code_ != NO_ERROR) {
                return nullptr;
            }
            selector.Select(*widgetTree_, receiver);
            if (!receiver.empty()) {
                auto image = make_unique<WidgetImage>();
                Widget2Image(receiver.at(0), *image, selector);
                return image;
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
            const auto type = scrollingUp ? WidgetOp::SWIPE_T2B : WidgetOp::SWIPE_B2T;
            auto bounds = scrollWidget->GetBounds();
            if (deadZoneSize > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += deadZoneSize;
                bounds.bottom_ -= deadZoneSize;
            }
            InjectWidgetOperate(bounds, type, *uiController_, options_);
        }
    }

    void UiDriver::ScrollToEdge(const WidgetImage &img, bool scrollingUp, ApiCallErr &err, int32_t deadZoneSize)
    {
        string prevSnapshot;
        while (true) {
            auto scrollWidget = RetrieveWidget(img, err);
            if (scrollWidget == nullptr) {
                scrollWidget = FindScrollWidget(img);
                if (scrollWidget != nullptr) {
                    err = ApiCallErr(NO_ERROR);
                }
            }
            if (scrollWidget == nullptr || err.code_ != NO_ERROR) {
                return;
            }
            string snapshot = TakeScopeUiSnapshot(*widgetTree_, *scrollWidget);
            if (snapshot == prevSnapshot) {
                return;
            }
            prevSnapshot = snapshot;
            const auto type = scrollingUp ? WidgetOp::SWIPE_T2B : WidgetOp::SWIPE_B2T;
            auto bounds = scrollWidget->GetBounds();
            if (deadZoneSize > 0) {
                // scroll widget from its deadZone maybe unresponsive
                bounds.top_ += deadZoneSize;
                bounds.bottom_ -= deadZoneSize;
            }
            InjectWidgetOperate(bounds, type, *uiController_, options_);
        }
    }

    void UiDriver::DragWidgetToAnother(const WidgetImage &imgFrom, const WidgetImage &imgTo, ApiCallErr &err)
    {
        auto widgetFrom = RetrieveWidget(imgFrom, err);
        if (widgetFrom == nullptr || err.code_ != NO_ERROR) {
            return;
        }
        auto widgetTo = RetrieveWidget(imgTo, err, false);
        if (widgetTo == nullptr || err.code_ != NO_ERROR) {
            return;
        }
        auto boundsFrom = widgetFrom->GetBounds();
        auto boundsTo = widgetTo->GetBounds();
        auto centerFrom = Point {boundsFrom.GetCenterX(), boundsFrom.GetCenterY()};
        auto centerTo = Point {boundsTo.GetCenterX(), boundsTo.GetCenterY()};
        InjectGenericSwipe(PointerOp::DRAG_P, centerFrom, centerTo, *uiController_, options_);
    }

    void UiDriver::FindWidgets(const WidgetSelector &select, vector<unique_ptr<WidgetImage>> &rev, ApiCallErr &err)
    {
        UpdateUi(true, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        vector<reference_wrapper<const Widget>> widgets;
        select.Select(*widgetTree_, widgets);
        // covert widgets to images as return value
        uint32_t index = 0;
        for (auto &ref:widgets) {
            auto image = make_unique<WidgetImage>();
            Widget2Image(ref.get(), *image, select);
            // at sometime, more than one widgets are found, add the node index to the description
            auto origDesc = image->GetSelectionDesc();
            auto newDesc = origDesc + "(index=" + to_string(index) + ")";
            image->SetSelectionDesc(newDesc);
            rev.emplace_back(move(image));
            index++;
        }
    }

    unique_ptr<WidgetImage> UiDriver::WaitForWidget(const WidgetSelector &select, uint32_t maxMs, ApiCallErr &err)
    {
        const uint32_t sliceMs = 20;
        const auto startMs = GetCurrentMillisecond();
        vector<unique_ptr<WidgetImage>> receiver;
        do {
            FindWidgets(select, receiver, err);
            if (err.code_ != NO_ERROR) { // abort on error
                return nullptr;
            }
            if (!receiver.empty()) {
                return move(receiver.at(0));
            }
            DelayMs(sliceMs);
        } while (GetCurrentMillisecond() - startMs < maxMs);
        return nullptr;
    }

    void UiDriver::UpdateWidgetImage(WidgetImage &image, ApiCallErr &error)
    {
        auto widget = RetrieveWidget(image, error);
        if (widget == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto selectionDesc = image.GetSelectionDesc();
        WidgetSelector selector; // dummy selector
        Widget2Image(*widget, image, selector);
        image.SetSelectionDesc(selectionDesc);
    }

    void UiDriver::DelayMs(uint32_t ms)
    {
        if (ms > 0) {
            this_thread::sleep_for(chrono::milliseconds(ms));
        }
    }

    void UiDriver::PerformGenericClick(PointerOp type, const Point &point, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        InjectGenericClick(type, point, *uiController_, options_);
    }

    void UiDriver::PerformGenericSwipe(PointerOp type, const Point &fromPoint, const Point &toPoint, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        InjectGenericSwipe(type, fromPoint, toPoint, *uiController_, options_);
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

    void UiDriver::WriteIntoParcel(json &data) const
    {
        data["device_name"] = deviceName_;
        json options;
        options_.WriteIntoParcel(options);
        data["options"] = options;
    }

    void UiDriver::ReadFromParcel(const json &data)
    {
        deviceName_ = data["device_name"];
        options_.ReadFromParcel(data["options"]);
    }
} // namespace uitest