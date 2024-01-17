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

    std::unique_ptr<UiController> UiDriver::uiController_;

    void UiDriver::RegisterController(std::unique_ptr<UiController> controller)
    {
        uiController_ = move(controller);
    }

    void UiDriver::RegisterUiEventListener(std::shared_ptr<UiEventListener> listener)
    {
        uiController_->RegisterUiEventListener(listener);
    }

    bool UiDriver::CheckStatus(bool isConnected, ApiCallErr &error)
    {
        DCHECK(uiController_);
        if (isConnected && !uiController_->IsWorkable()) {
            LOG_I("Not connect to AAMS, try to reconnect");
            if (!uiController_->Initialize()) {
                error = ApiCallErr(ERR_INITIALIZE_FAILED, "Can not connect to AAMS");
                return false;
            }
        }
        return true;
    }

    void UiDriver::UpdateUi(bool updateUiTree, ApiCallErr &error, bool getWidgetNodes, string targetWin)
    {
        if (!updateUiTree) {
            return;
        }
        if (!CheckStatus(true, error)) {
            return;
        }
        windows_.clear();
        widgetTree_ = make_unique<WidgetTree>("");
        vector<pair<Window, nlohmann::json>> hierarchies;
        uiController_->GetUiHierarchy(hierarchies, getWidgetNodes, targetWin);
        if (hierarchies.empty()) {
            if (targetWin != "") {
                LOG_E("Get window nodes failed, bundleName: %{public}s", targetWin.c_str());
                return;
            }
            LOG_E("%{public}s", "Get windows failed");
            error = ApiCallErr(ERR_INTERNAL, "Get window nodes failed");
            return;
        }
        vector<unique_ptr<WidgetTree>> trees;
        for (auto &hierarchy : hierarchies) {
            auto tree = make_unique<WidgetTree>("");
            tree->ConstructFromDom(hierarchy.second, true);
            trees.push_back(move(tree));
        }
        vector<int32_t> mergedOrdres;
        WidgetTree::MergeTrees(trees, *widgetTree_, mergedOrdres);
        auto virtualRoot = widgetTree_->GetRootWidget();
        for (size_t index = 0; index < mergedOrdres.size(); index++) {
            auto root = widgetTree_->GetChildWidget(*virtualRoot, index);
            DCHECK(root != nullptr);
            DCHECK(hierarchies.size() > mergedOrdres[index]);
            auto &window = hierarchies[mergedOrdres[index]].first;
            window.visibleBounds_ = root->GetBounds();
            windows_.push_back(move(window));
        }
    }

    void UiDriver::DumpUiHierarchy(nlohmann::json &out, bool listWindows, bool addExternAttr, ApiCallErr &error)
    {
        if (listWindows) {
            if (!CheckStatus(true, error)) {
                return;
            }
            vector<pair<Window, nlohmann::json>> datas;
            uiController_->GetUiHierarchy(datas, true);
            out = nlohmann::json::array();
            for (auto& data : datas) {
                out.push_back(data.second);
            }
        } else {
            UpdateUi(true, error, true);
            if (error.code_ != NO_ERROR || widgetTree_ == nullptr) {
                return;
            }
            widgetTree_->MarshalIntoDom(out);
        }
        if (addExternAttr) {
            map <int32_t, string_view> elementTrees;
            vector <char *> buffers;
            for (auto &win : windows_) {
                char *buffer = nullptr;
                size_t len = 0;
                uiController_->GetHidumperInfo(to_string(win.id_), &buffer, len);
                if (buffer == nullptr) {
                    continue;
                }
                elementTrees.insert(make_pair(win.id_, string_view(buffer, len)));
                buffers.push_back(buffer);
            }
            DumpHandler::AddExtraAttrs(out, elementTrees, 0);
            for (auto &buf : buffers) {
                delete buf;
            }
        }
    }

    static unique_ptr<Widget> CloneFreeWidget(const Widget &from, const WidgetSelector &selector)
    {
        auto clone = from.Clone("NONE", from.GetHierarchy());
        // save the selection desc as dummy attribute
        clone->SetAttr(DUMMY_ATTRNAME_SELECTION, selector.Describe());
        return clone;
    }

    string UiDriver::GetHostApp(const Widget &widget)
    {
        auto winId = widget.GetAttr(ATTR_NAMES[UiAttr::HOST_WINDOW_ID], "0");
        auto id = atoi(winId.c_str());
        for (auto window: windows_) {
            if (id == window.id_) {
                // If not a actived window, get all.
                if (window.actived_ == false) {
                    return "";
                }
                return window.bundleName_;
            }
        }
        return "";
    }

    const Widget *UiDriver::RetrieveWidget(const Widget &widget, ApiCallErr &err, bool updateUi)
    {
        if (updateUi) {
            auto hostApp = this->GetHostApp(widget);
            UpdateUi(true, err, true, hostApp);
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
        if (!CheckStatus(false, error)) {
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
            auto hostApp = select.GetAppLocator();
            UpdateUi(true, err, true, hostApp);
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
        if (!CheckStatus(false, err)) {
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

    void UiDriver::PerformMouseAction(const MouseAction &touch, const UiOpArgs &opt, ApiCallErr &err)
    {
        if (!CheckStatus(false, err)) {
            return;
        }
        vector<MouseEvent> events;
        touch.Decompose(events, opt);
        if (events.empty()) {
            return;
        }
        uiController_->InjectMouseEventSequence(events);
        uiController_->WaitForUiSteady(opt.uiSteadyThresholdMs_, opt.waitUiSteadyMaxMs_);
    }

    void UiDriver::TakeScreenCap(int32_t fd, ApiCallErr &err, Rect rect)
    {
        if (!CheckStatus(false, err)) {
            return;
        }
        stringstream errorRecv;
        if (!uiController_->TakeScreenCap(fd, errorRecv, rect)) {
            string errStr = errorRecv.str();
            LOG_W("ScreenCap failed: %{public}s", errStr.c_str());
            if (errStr.find("File opening failed") == 0) {
                err = ApiCallErr(ERR_INVALID_INPUT, "Invalid save path or permission denied");
            } else {
                err = ApiCallErr(ERR_INTERNAL, errStr);
            }
            LOG_W("ScreenCap failed: %{public}s", errorRecv.str().c_str());
        } else {
            LOG_D("ScreenCap successed");
        }
    }

    unique_ptr<Window> UiDriver::FindWindow(function<bool(const Window &)> matcher, ApiCallErr &err)
    {
        UpdateUi(true, err, false);
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
        UpdateUi(true, err, false);
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

    void UiDriver::SetDisplayRotation(DisplayRotation rotation, ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return;
        }
        uiController_->SetDisplayRotation(rotation);
    }

    DisplayRotation UiDriver::GetDisplayRotation(ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return ROTATION_0;
        }
        return uiController_->GetDisplayRotation();
    }

    void UiDriver::SetDisplayRotationEnabled(bool enabled, ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return;
        }
        uiController_->SetDisplayRotationEnabled(enabled);
    }

    bool UiDriver::WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutSec, ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return false;
        }
        return uiController_->WaitForUiSteady(idleThresholdMs, timeoutSec);
    }

    void UiDriver::WakeUpDisplay(ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return;
        }
        if (uiController_->IsScreenOn()) {
            return;
        } else {
            LOG_I("screen is off, turn it on");
            UiOpArgs uiOpArgs;
            this->TriggerKey(Power(), uiOpArgs, error);
        }
    }

    Point UiDriver::GetDisplaySize(ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return Point(0, 0);
        }
        return uiController_->GetDisplaySize();
    }

    Point UiDriver::GetDisplayDensity(ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return Point(0, 0);
        }
        return uiController_->GetDisplayDensity();
    }

    bool UiDriver::TextToKeyEvents(string_view text, std::vector<KeyEvent> &events, ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return false;
        }
        static constexpr uint32_t typeCharTimeMs = 50;
        if (!text.empty()) {
            vector<char> chars(text.begin(), text.end()); // decompose to sing-char input sequence
            vector<pair<int32_t, int32_t>> keyCodes;
            for (auto ch : chars) {
                int32_t code = KEYCODE_NONE;
                int32_t ctrlCode = KEYCODE_NONE;
                if (!uiController_->GetCharKeyCode(ch, code, ctrlCode)) {
                    return false;
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
        return true;
    }

    void UiDriver::DfsTraverseTree(WidgetVisitor &visitor, const Widget *widget)
    {
        if (widgetTree_ == nullptr) {
            return;
        }
        if (widget == nullptr) {
            widgetTree_->DfsTraverse(visitor);
        } else {
            widgetTree_->DfsTraverseDescendants(visitor, *widget);
        }
    }

    void UiDriver::GetLayoutJson(nlohmann::json &dom)
    {
        widgetTree_->MarshalIntoDom(dom);
    }

    void UiDriver::InputText(string_view text, ApiCallErr &error)
    {
        vector<KeyEvent> events;
        UiOpArgs uiOpArgs;
        if (!text.empty()) {
            if (TextToKeyEvents(text, events, error)) {
                LOG_I("inputText by Keycode");
                auto keyActionForInput = KeysForwarder(events);
                TriggerKey(keyActionForInput, uiOpArgs, error);
            } else {
                uiController_->PutTextToClipboard(text);
                LOG_I("inputText by pasteBoard");
                auto actionForPatse = CombinedKeys(KEYCODE_CTRL, KEYCODE_V, KEYCODE_NONE);
                TriggerKey(actionForPatse, uiOpArgs, error);
            }
        }
    }
} // namespace OHOS::uitest
