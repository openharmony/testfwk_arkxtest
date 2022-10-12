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

    void UiDriver::UpdateUi(bool updateUiTree, ApiCallErr &error)
    {
        UiController::InstallFromProvider();
        uiController_ = UiController::GetController();
        if (uiController_ == nullptr) {
            LOG_E("%{public}s", "No available UiController currently");
            error = ApiCallErr(ERR_INTERNAL, "No available UiController currently");
            return;
        }
        if (!updateUiTree) {
            return;
        }
        windows_.clear();
        widgetTree_ = make_unique<WidgetTree>("");
        vector<pair<Window, nlohmann::json>> hierarchies;
        uiController_->GetUiHierarchy(hierarchies);
        if (hierarchies.empty()) {
            LOG_E("%{public}s", "Get windows failed");
            error = ApiCallErr(ERR_INTERNAL, "Get window nodes failed");
            return;
        }
        vector<unique_ptr<WidgetTree>> trees;
        for (auto &hierarchy : hierarchies) {
            auto tree = make_unique<WidgetTree>("");
            tree->ConstructFromDom(hierarchy.second, true);
            auto &window = hierarchy.first;
            windows_.push_back(move(window));
            trees.push_back(move(tree));
        }
        WidgetTree::MergeTrees(trees, *widgetTree_);
    }

    /** Get current UI tree.*/
    const WidgetTree *UiDriver::GetWidgetTree() const
    {
        return widgetTree_ == nullptr? nullptr : widgetTree_.get();
    }

    /** Get current UI controller.*/
    const UiController *UiDriver::GetUiController(ApiCallErr &error)
    {
        this->UpdateUi(false, error);
        return uiController_;
    }

    void UiDriver::DumpUiHierarchy(nlohmann::json &out, ApiCallErr &error)
    {
        UpdateUi(true, error);
        if (error.code_ != NO_ERROR || widgetTree_ == nullptr) {
            return;
        }
        widgetTree_->MarshalIntoDom(out);
    }

    static unique_ptr<Widget> CloneFreeWidget(const Widget &from, const WidgetSelector &selector)
    {
        auto clone = from.Clone("NONE", from.GetHierarchy());
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
            err = ApiCallErr(ERR_COMPONENT_LOST, msg.str());
            LOG_W("%{public}s", err.message_.c_str());
            return nullptr;
        }
        DCHECK(recv.size() == 1);
        auto &retrieved = recv.at(0).get();
        // confirm type
        constexpr auto attrType = ATTR_NAMES[UiAttr::TYPE];
        if (widget.GetAttr(attrType, "A").compare(retrieved.GetAttr(attrType, "B")) != 0) {
            msg << " (CompareEqualsFailed)";
            err = ApiCallErr(ERR_COMPONENT_LOST, msg.str());
            LOG_W("%{public}s", err.message_.c_str());
            return nullptr;
        }
        return &retrieved;
    }

    void UiDriver::TriggerKey(const KeyAction &key, const UiOpArgs &opt, ApiCallErr &error)
    {
        UpdateUi(false, error);
        if (error.code_ != NO_ERROR) {
            return;
        }
        vector<KeyEvent> events;
        key.ComputeEvents(events, opt);
        if (events.empty()) {
            return;
        }
        uiController_->InjectKeyEventSequence(events);
        uiController_->WaitForUiSteady(opt.uiSteadyThresholdMs_, opt.waitUiSteadyMaxMs_);
    }

    void UiDriver::FindWidgets(const WidgetSelector &select, vector<unique_ptr<Widget>> &rev,
        ApiCallErr &err, bool updateUi)
    {
        if (updateUi) {
            UpdateUi(true, err);
            if (err.code_ != NO_ERROR) {
                return;
            }
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

    void UiDriver::DelayMs(uint32_t ms)
    {
        if (ms > 0) {
            this_thread::sleep_for(chrono::milliseconds(ms));
        }
    }

    void UiDriver::PerformTouch(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        PointerMatrix events;
        touch.Decompose(events, opt);
        if (events.Empty()) {
            return;
        }
        uiController_->InjectTouchEventSequence(events);
        uiController_->WaitForUiSteady(opt.uiSteadyThresholdMs_, opt.waitUiSteadyMaxMs_);
    }

    void UiDriver::TakeScreenCap(string_view savePath, ApiCallErr &err)
    {
        UpdateUi(false, err);
        if (err.code_ != NO_ERROR) {
            return;
        }
        stringstream errorRecv;
        if (!uiController_->TakeScreenCap(savePath, errorRecv)) {
            string errStr = errorRecv.str();
            LOG_W("ScreenCap failed: %{public}s", errStr.c_str());
            if (errStr.find("File opening failed") == 0) {
                err = ApiCallErr(ERR_INVALID_INPUT, "Invalid save path or permission denied");
            } else {
                err = ApiCallErr(ERR_INTERNAL, errStr);
            }
            LOG_W("ScreenCap failed: %{public}s", errorRecv.str().c_str());
        } else {
            LOG_D("ScreenCap saved to %{public}s", savePath.data());
        }
    }

    unique_ptr<Window> UiDriver::FindWindow(function<bool(const Window &)> matcher, ApiCallErr &err)
    {
        UpdateUi(true, err);
        if (err.code_ != NO_ERROR) {
            return nullptr;
        }
        for (const auto &window : windows_) {
            if (matcher(window)) {
                auto clone = make_unique<Window>(0);
                *clone = window; // copy construct
                return clone;
            }
        }
        return nullptr;
    }

    const Window *UiDriver::RetrieveWindow(const Window &window, ApiCallErr &err)
    {
        UpdateUi(true, err);
        if (err.code_ != NO_ERROR) {
            return nullptr;
        }
        for (const auto &win : windows_) {
            if (win.id_ == window.id_) {
                return &win;
            }
        }
        stringstream msg;
        msg << "Window " << window.id_;
        msg << "dose not exist on current UI! Check if the UI has changed after you got the window object";
        err = ApiCallErr(ERR_COMPONENT_LOST, msg.str());
        LOG_W("%{public}s", err.message_.c_str());
        return nullptr;
    }
} // namespace OHOS::uitest
