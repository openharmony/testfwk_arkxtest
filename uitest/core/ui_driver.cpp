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
#include "ui_model.h"
#include "ui_driver.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    class WindowCacheCompareGreater {
    public:
        bool operator()(const WindowCacheModel &w1, const WindowCacheModel &w2)
        {
            if (w1.window_.actived_) {
                return true;
            }
            if (w2.window_.actived_) {
                return false;
            }
            if (w1.window_.focused_) {
                return true;
            }
            if (w2.window_.focused_) {
                return false;
            }
            return w1.window_.windowLayer_ > w2.window_.windowLayer_;
        }
    };

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
            LOG_W("Not connect to AAMS, try to reconnect");
            if (!uiController_->Initialize(error)) {
                LOG_E("%{public}s", error.message_.c_str());
                return false;
            }
        }
        return true;
    }

    void UiDriver::UpdateUIWindows(ApiCallErr &error)
    {
        visitWidgets_.clear();
        targetWidgetsIndex_.clear();
        windowCacheVec_.clear();
        if (!CheckStatus(true, error)) {
            return;
        }
        std::vector<Window> currentWindowVec;
        uiController_->GetUiWindows(currentWindowVec);
        if (currentWindowVec.empty()) {
            LOG_E("Get Windows Failed");
            error = ApiCallErr(ERR_INTERNAL, "Get window nodes failed");
        }

        for (const auto &win : currentWindowVec) {
            WindowCacheModel cacheModel(win);
            windowCacheVec_.emplace_back(std::move(cacheModel));
            std::stringstream ss;
            ss << "window rect is ";
            ss << win.bounds_.Describe();
            ss << "overplay window rects are:[";
            for (const auto& overRect : win.invisibleBoundsVec_) {
                ss << overRect.Describe();
                ss << ", ";
            }
            ss << "]";
            LOG_D("window id is %{public}d, rect info is %{public}s", win.id_, ss.str().data());
        }
        // actice or focus window move to top
        std::sort(windowCacheVec_.begin(), windowCacheVec_.end(), WindowCacheCompareGreater());
    }

    void UiDriver::DumpWindowsInfo(bool listWindows, Rect& mergeBounds, nlohmann::json& childDom)
    {
        std::vector<WidgetMatchModel> emptyMatcher;
        StrategyBuildParam buildParam;
        buildParam.myselfMatcher = emptyMatcher;
        std::unique_ptr<SelectStrategy> selectStrategy = SelectStrategy::BuildSelectStrategy(buildParam, true);
        for (auto &winCache : windowCacheVec_) {
            visitWidgets_.clear();
            targetWidgetsIndex_.clear();
            if (!uiController_->GetWidgetsInWindow(winCache.window_, winCache.widgetIterator_)) {
                LOG_W("Get Widget from window[%{public}d] failed, skip the window", winCache.window_.id_);
                continue;
            }
            selectStrategy->LocateNode(winCache.window_, *winCache.widgetIterator_, visitWidgets_, targetWidgetsIndex_,
                                       !listWindows);
            nlohmann::json child = nlohmann::json();
            if (visitWidgets_.empty()) {
                LOG_E("Window %{public}s has no node, skip it", winCache.window_.bundleName_.data());
                continue;
            } else {
                DumpHandler::DumpWindowInfoToJson(visitWidgets_, child);
            }
            child["attributes"]["abilityName"] = winCache.window_.abilityName_;
            child["attributes"]["bundleName"] = winCache.window_.bundleName_;
            child["attributes"]["pagePath"] = winCache.window_.pagePath_;
            childDom.emplace_back(child);
            mergeBounds.left_ = std::min(mergeBounds.left_, winCache.window_.bounds_.left_);
            mergeBounds.top_ = std::min(mergeBounds.top_, winCache.window_.bounds_.top_);
            mergeBounds.right_ = std::max(mergeBounds.right_, winCache.window_.bounds_.right_);
            mergeBounds.bottom_ = std::max(mergeBounds.bottom_, winCache.window_.bounds_.bottom_);
        }
    }

    void UiDriver::DumpUiHierarchy(nlohmann::json &out, bool listWindows, bool addExternAttr, ApiCallErr &error)
    {
        UpdateUIWindows(error);
        if (error.code_ != NO_ERROR) {
            return;
        }
        nlohmann::json childDom = nlohmann::json::array();
        Rect mergeBounds{0, 0, 0, 0};
        DumpWindowsInfo(listWindows, mergeBounds, childDom);
        if (listWindows) {
            out = childDom;
        } else {
            nlohmann::json attrData = nlohmann::json();

            for (int i = 0; i < UiAttr::HIERARCHY; ++i) {
                attrData[ATTR_NAMES[i].data()] = "";
            }
            std::stringstream ss;
            ss << "[" << mergeBounds.left_ << "," << mergeBounds.top_ << "]"
               << "[" << mergeBounds.right_ << "," << mergeBounds.bottom_ << "]";
            attrData[ATTR_NAMES[UiAttr::BOUNDS].data()] = ss.str();
            out["attributes"] = attrData;
            out["children"] = childDom;
        }

        if (addExternAttr) {
            map<int32_t, string_view> elementTrees;
            vector<char *> buffers;
            for (auto &winCache : windowCacheVec_) {
                char *buffer = nullptr;
                size_t len = 0;
                uiController_->GetHidumperInfo(to_string(winCache.window_.id_), &buffer, len);
                if (buffer == nullptr) {
                    continue;
                }
                elementTrees.insert(make_pair(winCache.window_.id_, string_view(buffer, len)));
                buffers.push_back(buffer);
            }
            DumpHandler::AddExtraAttrs(out, elementTrees, 0);
            for (auto &buf : buffers) {
                delete buf;
            }
        }
    }

    static unique_ptr<Widget> CloneFreeWidget(const Widget &from, const string &selectDesc)
    {
        auto clone = from.Clone(from.GetHierarchy());
        clone->SetAttr(UiAttr::DUMMY_ATTRNAME_SELECTION, selectDesc + from.GetAttr(UiAttr::HASHCODE));
        // save the selection desc as dummy attribute
        return clone;
    }

    static std::unique_ptr<SelectStrategy> ConstructSelectStrategyByRetrieve(const Widget &widget)
    {
        WidgetMatchModel attrMatch{UiAttr::HASHCODE, widget.GetAttr(UiAttr::HASHCODE), EQ};
        StrategyBuildParam buildParam;
        buildParam.myselfMatcher.emplace_back(attrMatch);
        return SelectStrategy::BuildSelectStrategy(buildParam, false);
    }

    string UiDriver::GetHostApp(const Widget &widget)
    {
        auto winId = widget.GetAttr(UiAttr::HOST_WINDOW_ID);
        if (winId.length() < 1) {
            winId = "0";
        }
        auto id = atoi(winId.c_str());
        for (auto &windowCache : windowCacheVec_) {
            if (id == windowCache.window_.id_) {
                // If not a actived window, get all.
                if (windowCache.window_.actived_ == false) {
                    return "";
                }
                return windowCache.window_.bundleName_;
            }
        }
        return "";
    }

    const Widget *UiDriver::RetrieveWidget(const Widget &widget, ApiCallErr &err, bool updateUi)
    {
        if (updateUi) {
            UpdateUIWindows(err);
            if (err.code_ != NO_ERROR) {
                LOG_I("Retrieve Widget with error %{public}s", err.message_.c_str());
                return nullptr;
            }
        } else {
            visitWidgets_.clear();
            targetWidgetsIndex_.clear();
        }

        std::unique_ptr<SelectStrategy> selectStrategy = ConstructSelectStrategyByRetrieve(widget);
        for (auto &curWinCache : windowCacheVec_) {
            if (widget.GetAttr(UiAttr::HOST_WINDOW_ID) != std::to_string(curWinCache.window_.id_)) {
                continue;
            }
            selectStrategy->SetAndCalcSelectWindowRect(curWinCache.window_.bounds_,
                                                       curWinCache.window_.invisibleBoundsVec_);
            if (curWinCache.widgetIterator_ == nullptr) {
                if (!uiController_->GetWidgetsInWindow(curWinCache.window_, curWinCache.widgetIterator_)) {
                    LOG_W("Get Widget from window[%{public}d] failed, skip the window", curWinCache.window_.id_);
                    continue;
                }
            }
            selectStrategy->LocateNode(curWinCache.window_, *curWinCache.widgetIterator_, visitWidgets_,
                                       targetWidgetsIndex_);
            if (!targetWidgetsIndex_.empty()) {
                break;
            }
        }
        stringstream msg;
        msg << "Widget: " << widget.GetAttr(UiAttr::DUMMY_ATTRNAME_SELECTION);
        msg << "dose not exist on current UI! Check if the UI has changed after you got the widget object";
        if (targetWidgetsIndex_.empty()) {
            msg << "(NoCandidates)";
            err = ApiCallErr(ERR_COMPONENT_LOST, msg.str());
            LOG_W("%{public}s", err.message_.c_str());
            return nullptr;
        }
        DCHECK(targetWidgetsIndex_.size() == 1);
        // confirm type
        if (widget.GetAttr(UiAttr::TYPE) != visitWidgets_[targetWidgetsIndex_[0]].GetAttr(UiAttr::TYPE)) {
            msg << " (CompareEqualsFailed)";
            err = ApiCallErr(ERR_COMPONENT_LOST, msg.str());
            LOG_W("%{public}s", err.message_.c_str());
            return nullptr;
        }
        return &visitWidgets_[targetWidgetsIndex_[0]];
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
    }

    void UiDriver::FindWidgets(const WidgetSelector &selector, vector<unique_ptr<Widget>> &rev,
        ApiCallErr &err, bool updateUi)
    {
        UiOpArgs opt;
        uiController_->WaitForUiSteady(opt.uiSteadyThresholdMs_, opt.waitUiSteadyMaxMs_);
        if (updateUi) {
            UpdateUIWindows(err);
            if (err.code_ != NO_ERROR) {
                return;
            }
        } else {
            visitWidgets_.clear();
            targetWidgetsIndex_.clear();
        }
        for (auto &curWinCache : windowCacheVec_) {
            LOG_D("Start find in Window, window id is %{public}d", curWinCache.window_.id_);
            if (curWinCache.widgetIterator_ == nullptr) {
                std::unique_ptr<ElementNodeIterator> widgetIterator = nullptr;
                if (!uiController_->GetWidgetsInWindow(curWinCache.window_, curWinCache.widgetIterator_)) {
                    LOG_W("Get Widget from window[%{public}d] failed, skip the window", curWinCache.window_.id_);
                    continue;
                }
            }
            selector.Select(curWinCache.window_, *curWinCache.widgetIterator_, visitWidgets_, targetWidgetsIndex_);
            if (!selector.IsWantMulti() && !targetWidgetsIndex_.empty()) {
                break;
            }
            if (!selector.IsWantMulti()) {
                visitWidgets_.clear();
                targetWidgetsIndex_.clear();
            }
        }
        if (targetWidgetsIndex_.empty()) {
            LOG_W("self node not found by %{public}s", selector.Describe().data());
            return;
        }
        // covert widgets to images as return value
        uint32_t index = 0;
        for (auto targetIndex : targetWidgetsIndex_) {
            auto image = CloneFreeWidget(visitWidgets_[targetIndex], selector.Describe());
            // at sometime, more than one widgets are found, add the node index to the description
            rev.emplace_back(move(image));
            index++;
        }
    }

    unique_ptr<Widget> UiDriver::WaitForWidget(const WidgetSelector &selector, const UiOpArgs &opt, ApiCallErr &err)
    {
        const uint32_t sliceMs = 20;
        const auto startMs = GetCurrentMillisecond();
        vector<unique_ptr<Widget>> receiver;
        do {
            FindWidgets(selector, receiver, err);
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
        UpdateUIWindows(err);
        if (err.code_ != NO_ERROR) {
            return nullptr;
        }
        for (auto &windowCache : windowCacheVec_) {
            if (matcher(windowCache.window_)) {
                auto clone = make_unique<Window>(0);
                *clone = windowCache.window_; // copy construct
                return clone;
            }
        }
        return nullptr;
    }

    const Window *UiDriver::RetrieveWindow(const Window &window, ApiCallErr &err)
    {
        UpdateUIWindows(err);
        if (err.code_ != NO_ERROR) {
            return nullptr;
        }
        for (auto &winCache : windowCacheVec_) {
            if (winCache.window_.id_ != window.id_) {
                continue;
            }
            return &winCache.window_;
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
                if (pair.second != KEYCODE_NONE) {
                    events.emplace_back(KeyEvent {ActionStage::UP, pair.second, 0});
                }
                events.emplace_back(KeyEvent {ActionStage::UP, pair.first, 0});
            }
        }
        return true;
    }

    void UiDriver::InputText(string_view text, Point point, ApiCallErr &error)
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

    void UiDriver::GetMergeWindowBounds(Rect &mergeRect)
    {
        for (const auto &winCache : windowCacheVec_) {
            mergeRect.left_ = std::min(winCache.window_.bounds_.left_, mergeRect.left_);
            mergeRect.top_ = std::min(winCache.window_.bounds_.top_, mergeRect.top_);
            mergeRect.right_ = std::max(winCache.window_.bounds_.right_, mergeRect.right_);
            mergeRect.bottom_ = std::max(winCache.window_.bounds_.bottom_, mergeRect.bottom_);
        }
    }
} // namespace OHOS::uitest
