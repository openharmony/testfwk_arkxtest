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
#include <thread>
#include <atomic>
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

    void UiDriver::UpdateUIWindows(ApiCallErr &error, int32_t targetDisplay)
    {
        visitWidgets_.clear();
        targetWidgetsIndex_.clear();
        displayToWindowCacheMap_.clear();
        if (!CheckStatus(true, error)) {
            return;
        }
        std::map<int32_t, vector<Window>> currentDisplayAndWindowCacheMap;
        uiController_->GetUiWindows(currentDisplayAndWindowCacheMap, targetDisplay);
        if (currentDisplayAndWindowCacheMap.empty()) {
            LOG_E("Get Windows Failed");
            error = ApiCallErr(ERR_INTERNAL, "Get window nodes failed");
        }
        for (auto dm : currentDisplayAndWindowCacheMap) {
            auto currentWindowVec = dm.second;
            std::vector<WindowCacheModel> windowCacheVec;
            for (const auto &win : currentWindowVec) {
                WindowCacheModel cacheModel(win);
                windowCacheVec.emplace_back(std::move(cacheModel));
                std::stringstream ss;
                ss << "window rect is ";
                ss << win.bounds_.Describe();
                ss << "overplay window rects are:[";
                for (const auto& overRect : win.invisibleBoundsVec_) {
                    ss << overRect.Describe();
                    ss << ", ";
                }
                ss << "]";
                LOG_D("window: %{public}d in display %{public}d, %{public}s",
                      win.id_, win.displayId_, ss.str().data());
            }
            // actice or focus window move to top
            std::sort(windowCacheVec.begin(), windowCacheVec.end(), WindowCacheCompareGreater());
            displayToWindowCacheMap_.insert(make_pair(dm.first, move(windowCacheVec)));
        }
    }

    void UiDriver::DumpWindowsInfo(const DumpOption &option, Rect& mergeBounds, nlohmann::json& childDom)
    {
        std::vector<WidgetMatchModel> emptyMatcher;
        StrategyBuildParam buildParam;
        buildParam.myselfMatcher = emptyMatcher;
        std::unique_ptr<SelectStrategy> selectStrategy = SelectStrategy::BuildSelectStrategy(buildParam, true);
        auto dm = displayToWindowCacheMap_.find(option.displayId_);
        if (dm == displayToWindowCacheMap_.end()) {
            return;
        }
        auto &windowCacheVec = dm->second;
        for (auto &winCache : windowCacheVec) {
            if (option.bundleName_ != "" && winCache.window_.bundleName_ != option.bundleName_) {
                LOG_D("skip window(%{public}s), it is not target window %{public}s",
                    winCache.window_.bundleName_.data(), option.bundleName_.data());
                continue;
            }
            if (option.windowId_ != "" && winCache.window_.id_ != atoi(option.windowId_.c_str())) {
                LOG_D("skip window(%{public}d), it is not target window %{public}s",
                    winCache.window_.id_, option.windowId_.data());
                continue;
            }
            visitWidgets_.clear();
            targetWidgetsIndex_.clear();
            if (!uiController_->GetWidgetsInWindow(winCache.window_, winCache.widgetIterator_, mode_)) {
                LOG_W("Get Widget from window[%{public}d] failed, skip the window", winCache.window_.id_);
                continue;
            }
            selectStrategy->LocateNode(winCache.window_, *winCache.widgetIterator_, visitWidgets_, targetWidgetsIndex_,
                                       option);
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

    void UiDriver::DumpUiHierarchy(nlohmann::json &out, const DumpOption &option, ApiCallErr &error)
    {
        UpdateUIWindows(error, option.displayId_);
        if (error.code_ != NO_ERROR) {
            return;
        }
        auto dm = displayToWindowCacheMap_.find(option.displayId_);
        if (dm == displayToWindowCacheMap_.end()) {
            LOG_E("Get windows id display %{public}d failed, dump error.", option.displayId_);
            error = ApiCallErr(ERR_INTERNAL, "Get window nodes failed");
            return;
        }
        nlohmann::json childDom = nlohmann::json::array();
        Rect mergeBounds{0, 0, 0, 0};
        DumpWindowsInfo(option, mergeBounds, childDom);
        if (option.listWindows_) {
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

        if (option.addExternAttr_) {
            map<int32_t, string_view> elementTrees;
            vector<char *> buffers;
            auto &windowCacheVec = dm->second;
            for (auto &winCache : windowCacheVec) {
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
        auto displayId = widget.GetDisplayId();
        if (winId.length() < 1) {
            winId = "0";
        }
        auto dm = displayToWindowCacheMap_.find(displayId);
        if (dm == displayToWindowCacheMap_.end()) {
            return "";
        }
        auto &windowCacheVec = dm->second;
        auto id = atoi(winId.c_str());
        for (auto &windowCache : windowCacheVec) {
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
            UpdateUIWindows(err, widget.GetDisplayId());
            if (err.code_ != NO_ERROR) {
                LOG_E("Retrieve Widget with error %{public}s", err.message_.c_str());
                return nullptr;
            }
        } else {
            visitWidgets_.clear();
            targetWidgetsIndex_.clear();
        }
        std::unique_ptr<SelectStrategy> selectStrategy = ConstructSelectStrategyByRetrieve(widget);
        auto &windowCacheVec = displayToWindowCacheMap_.find(widget.GetDisplayId())->second;
        for (auto &curWinCache : windowCacheVec) {
            if (widget.GetAttr(UiAttr::HOST_WINDOW_ID) != std::to_string(curWinCache.window_.id_)) {
                continue;
            }
            selectStrategy->SetAndCalcSelectWindowRect(curWinCache.window_.bounds_,
                                                       curWinCache.window_.invisibleBoundsVec_);
            if (curWinCache.widgetIterator_ == nullptr) {
                if (!uiController_->GetWidgetsInWindow(curWinCache.window_, curWinCache.widgetIterator_, mode_)) {
                    LOG_W("Get Widget from window[%{public}d] failed, skip the window", curWinCache.window_.id_);
                    continue;
                }
            }
            DumpOption option;
            selectStrategy->LocateNode(curWinCache.window_, *curWinCache.widgetIterator_, visitWidgets_,
                                       targetWidgetsIndex_, option);
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

    void UiDriver::TriggerKey(const KeyAction &key, const UiOpArgs &opt, ApiCallErr &error, int32_t displayId)
    {
        if (!CheckStatus(false, error)) {
            return;
        }
        if (!CheckDisplayExist(displayId)) {
            error = ApiCallErr(ERR_INVALID_INPUT, "Invalid display id.");
            return;
        }
        vector<KeyEvent> events;
        key.ComputeEvents(events, opt);
        if (events.empty()) {
            return;
        }
        uiController_->InjectKeyEventSequence(events, displayId);
    }

    void UiDriver::FindWidgets(const WidgetSelector &selector, vector<unique_ptr<Widget>> &rev,
        ApiCallErr &err, bool updateUi, bool skipWaitForUiSteady)
    {
        UiOpArgs opt;
        if (!skipWaitForUiSteady) {
            uiController_->WaitForUiSteady(opt.uiSteadyThresholdMs_, opt.waitUiSteadyMaxMs_);
        }
        if (updateUi) {
            UpdateUIWindows(err, selector.GetDisplayLocator());
            if (err.code_ != NO_ERROR) {
                return;
            }
        } else {
            visitWidgets_.clear();
            targetWidgetsIndex_.clear();
        }
        auto appLocator = selector.GetAppLocator();
        for (auto &dm : displayToWindowCacheMap_) {
            auto &windowCacheVec = dm.second;
            for (auto &curWinCache : windowCacheVec) {
                if (appLocator != "" && curWinCache.window_.bundleName_ != appLocator) {
                    continue;
                }
                if (curWinCache.widgetIterator_ == nullptr &&
                    !uiController_->GetWidgetsInWindow(curWinCache.window_, curWinCache.widgetIterator_, mode_)) {
                    continue;
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
            if (!selector.IsWantMulti() && !targetWidgetsIndex_.empty()) {
                break;
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
        auto displayId = events.At(0, 0).point_.displayId_;
        if (!CheckDisplayExist(displayId)) {
            LOG_E("No display: %{public}d", displayId);
            err = ApiCallErr(ERR_INVALID_INPUT, "Invalid display id.");
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
        auto displayId = events.begin()->point_.displayId_;
        if (!CheckDisplayExist(displayId)) {
            LOG_E("No display: %{public}d", displayId);
            err = ApiCallErr(ERR_INVALID_INPUT, "Invalid display id.");
            return;
        }
        uiController_->InjectMouseEventSequence(events);
    }

    void UiDriver::TakeScreenCap(int32_t fd, ApiCallErr &err, Rect rect, int32_t displayId)
    {
        if (!CheckStatus(false, err)) {
            return;
        }
        stringstream errorRecv;
        if (!uiController_->TakeScreenCap(fd, errorRecv, displayId, rect)) {
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
        for (auto &dm : displayToWindowCacheMap_) {
            auto &windowCacheVec = dm.second;
            for (auto &windowCache : windowCacheVec) {
                if (matcher(windowCache.window_)) {
                    auto clone = make_unique<Window>(0);
                    *clone = windowCache.window_; // copy construct
                    return clone;
                }
            }
        }
        return nullptr;
    }

    const Window *UiDriver::RetrieveWindow(const Window &window, ApiCallErr &err)
    {
        UpdateUIWindows(err, window.displayId_);
        if (err.code_ != NO_ERROR) {
            return nullptr;
        }
        auto dm = displayToWindowCacheMap_.find(window.displayId_);
        auto &windowCacheVec = dm->second;
        for (auto &winCache : windowCacheVec) {
            if (winCache.window_.id_ != window.id_) {
                continue;
            }
            return &winCache.window_;
        }
        stringstream msg;
        msg << "Display " << window.displayId_;
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

    DisplayRotation UiDriver::GetDisplayRotation(ApiCallErr &error, int32_t displayId)
    {
        if (!CheckStatus(false, error)) {
            return ROTATION_0;
        }
        return uiController_->GetDisplayRotation(displayId);
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
            LOG_D("screen is off, turn it on");
            UiOpArgs uiOpArgs;
            this->TriggerKey(Power(), uiOpArgs, error);
        }
    }

    Point UiDriver::GetDisplaySize(ApiCallErr &error, int32_t displayId)
    {
        if (!CheckStatus(false, error)) {
            return Point(0, 0);
        }
        return uiController_->GetDisplaySize(displayId);
    }

    Point UiDriver::GetDisplayDensity(ApiCallErr &error, int32_t displayId)
    {
        if (!CheckStatus(false, error)) {
            return Point(0, 0);
        }
        return uiController_->GetDisplayDensity(displayId);
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

    void UiDriver::InputText(string_view text, ApiCallErr &error, const UiOpArgs &opt, int32_t displayId)
    {
        if (text.empty()) {
            return;
        }
        vector<KeyEvent> events;
        constexpr auto maxKeyEventCounts = 200;
        if (!TextToKeyEvents(text, events, error) || opt.inputByPasteBoard_ || text.length() > maxKeyEventCounts) {
            LOG_D("inputText by pasteBoard");
            uiController_->PutTextToClipboard(text, error);
            if (error.code_ != NO_ERROR) {
                return;
            }
            auto actionForPatse = CombinedKeys(KEYCODE_CTRL, KEYCODE_V, KEYCODE_NONE);
            TriggerKey(actionForPatse, opt, error, displayId);
        } else {
            LOG_D("inputText by Keycode");
            auto keyActionForInput = KeysForwarder(events);
            TriggerKey(keyActionForInput, opt, error, displayId);
        }
    }

    bool UiDriver::IsTouchPadExist()
    {
        return uiController_->IsTouchPadExist();
    }

    void UiDriver::PerformTouchPadAction(const TouchPadAction &touch, const UiOpArgs &opt, ApiCallErr &error)
    {
        if (!CheckStatus(false, error)) {
            return;
        }
        if (!uiController_->IsTouchPadExist()) {
            error = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "This device can not support this action");
            return;
        }
        vector<TouchPadEvent> events;
        touch.Decompose(events, opt, uiController_->GetDisplaySize(0));
        if (events.empty()) {
            return;
        }
        uiController_->InjectTouchPadEventSequence(events);
    }

    void UiDriver::PerformPenTouch(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err)
    {
        if (!CheckStatus(false, err)) {
            return;
        }
        if (opt.touchPressure_ < 0 || opt.touchPressure_ > 1) {
            err = ApiCallErr(ERR_INVALID_INPUT, "Pressure must ranges form 0 to 1");
            return;
        }
        PointerMatrix events;
        touch.Decompose(events, opt);
        PointerMatrix eventsInPen(1, events.GetSteps() + INDEX_TWO);
        eventsInPen.SetTouchPressure(opt.touchPressure_);
        events.ConvertToPenEvents(eventsInPen);
        if (eventsInPen.Empty()) {
            return;
        }
        auto displayId = eventsInPen.At(0, 0).point_.displayId_;
        if (!CheckDisplayExist(displayId)) {
            LOG_E("No display: %{public}d", displayId);
            err = ApiCallErr(ERR_INVALID_INPUT, "Invalid display id.");
            return;
        }
        uiController_->InjectTouchEventSequence(eventsInPen);
    }

    void UiDriver::PerformKnuckleAction(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err)
    {
#ifdef ARKXTEST_KNUCKLE_ACTION_ENABLE
        if (!CheckStatus(false, err)) {
            return;
        }
        PointerMatrix events;
        touch.Decompose(events, opt);
        events.SetToolType(TouchToolType::KNUCKLE);
        auto displayId = events.At(0, 0).point_.displayId_;
        if (!CheckDisplayExist(displayId)) {
            LOG_E("No display: %{public}d", displayId);
            err = ApiCallErr(ERR_INVALID_PARAM, "Invalid display id.");
            return;
        }
        uiController_->InjectTouchEventSequence(events);
        return;
#endif
        err = ApiCallErr(ERR_OPERATION_UNSUPPORTED, "This action is not support.");
        return;
    }

    void UiDriver::SetAamsWorkMode(const AamsWorkMode mode)
    {
        mode_ = mode;
    }

    bool UiDriver::IsWearable()
    {
        return uiController_->IsWearable();
    }

    bool UiDriver::IsAdjustWindowModeEnable()
    {
        return uiController_->IsAdjustWindowModeEnable();
    }

    bool UiDriver::CheckDisplayExist(int32_t displayId)
    {
        return uiController_->CheckDisplayExist(displayId);
    }

    void UiDriver::CloseAamsEvent()
    {
        eventObserverEnable_ = false;
        return uiController_->CloseAamsEvent();
    }

    void UiDriver::OpenAamsEvent()
    {
        eventObserverEnable_ = true;
        return uiController_->OpenAamsEvent();
    }

    bool UiDriver::GetEventObserverEnable()
    {
        return eventObserverEnable_;
    }
    
    void UiDriver::ChangeWindowMode(int32_t windowId, WindowMode mode)
    {
        return uiController_->ChangeWindowMode(windowId, mode);
    }

    // Common helper method to check component presence with timeout
    bool UiDriver::CheckComponentPresenceWithTimeout(const WidgetSelector& selector,
        const TimeoutParams& timeoutParams, ApiCallErr& error)
    {
        bool componentFound = false;
        int32_t currentTime;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutParams.uiOpArgs.pollingInterval_));
            std::vector<std::unique_ptr<Widget>> foundWidgets;
            this->FindWidgets(selector, foundWidgets, error, true, true);
            componentFound = !foundWidgets.empty();
            currentTime = static_cast<int32_t>(GetCurrentMillisecond());
        } while (!componentFound && (currentTime - timeoutParams.startTime) < timeoutParams.totalTimeout);
        
        if (componentFound) {
            int32_t elapsedTime = currentTime - timeoutParams.startTime;
            int32_t remainingTime = timeoutParams.operationTime - elapsedTime;
            if (remainingTime > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(remainingTime));
            }
        }
        return componentFound;
    }

    // Common helper method to calculate operation time based on distance and speed
    int32_t UiDriver::CalculateOperationTime(const Point& from, const Point& to,
        int32_t speed, const UiOpArgs& uiOpArgs)
    {
        double distance = std::sqrt(std::pow(to.px_ - from.px_, TWO) + std::pow(to.py_ - from.py_, TWO));
        if (speed != 0) {
            return static_cast<int32_t>((distance / speed) * uiOpArgs.oneThousand_);
        }
        return 0;
    }

    bool UiDriver::IsComponentPresentWhenLongClick(const WidgetSelector& selector, const TouchAction& action,
        const UiOpArgs& uiOpArgs, ApiCallErr& error)
    {
        try {
            // Calculate operation time based on the action type
            int32_t operationTime = uiOpArgs.longClickHoldMs_;
            int32_t totalTimeout = operationTime + uiOpArgs.delayTime_;
            int32_t startTime = static_cast<int32_t>(GetCurrentMillisecond());
            
            // Start component presence checking in a separate thread
            std::atomic<bool> componentFound(false);
            std::atomic<bool> touchCompleted(false);
            std::thread checkThread([this, &componentFound, &selector, startTime, totalTimeout,
                operationTime, &uiOpArgs, &touchCompleted]() {
                TimeoutParams timeoutParams(startTime, totalTimeout, operationTime, uiOpArgs);
                ApiCallErr threadError(NO_ERROR);
                componentFound = CheckComponentPresenceWithTimeout(selector, timeoutParams, threadError);
                if (threadError.code_ != NO_ERROR) {
                    LOG_W("Component presence check thread encountered error: %{public}s (code: %{public}d)",
                          threadError.message_.c_str(), threadError.code_);
                }
            });
            
            // Start touch operation in a separate thread
            std::thread touchThread([this, &action, &uiOpArgs, &error, &touchCompleted]() {
                this->PerformTouch(action, uiOpArgs, error);
                touchCompleted = true;
            });
            
            // Wait for both threads to finish
            if (touchThread.joinable()) {
                touchThread.join();
            }
            if (checkThread.joinable()) {
                checkThread.join();
            }
            
            return componentFound;
        } catch (const std::exception& e) {
            error = ApiCallErr(ERR_INTERNAL, "LongClick operation failed: " + std::string(e.what()));
            return false;
        }
    }

    bool UiDriver::IsComponentPresentWhenDrag(const WidgetSelector& selector, const TouchAction& action,
        const UiOpArgs& uiOpArgs, ApiCallErr& error)
    {
        try {
            // Calculate operation time based on the action type
            int32_t operationTime = uiOpArgs.longClickHoldMs_;
            int32_t totalTimeout = operationTime + uiOpArgs.delayTime_;
            int32_t startTime = static_cast<int32_t>(GetCurrentMillisecond());
            
            // Start component presence checking in a separate thread
            std::atomic<bool> componentFound(false);
            std::atomic<bool> touchCompleted(false);
            std::thread checkThread([this, &componentFound, &selector, startTime, totalTimeout,
                operationTime, &uiOpArgs, &touchCompleted]() {
                TimeoutParams timeoutParams(startTime, totalTimeout, operationTime, uiOpArgs);
                ApiCallErr threadError(NO_ERROR);
                componentFound = CheckComponentPresenceWithTimeout(selector, timeoutParams, threadError);
                if (threadError.code_ != NO_ERROR) {
                    LOG_W("Component presence check thread encountered error: %{public}s (code: %{public}d)",
                          threadError.message_.c_str(), threadError.code_);
                }
            });
            
            // Start touch operations in a separate thread
            std::thread touchThread([this, &action, &uiOpArgs, &error, &touchCompleted]() {
                this->PerformTouch(action, uiOpArgs, error);
                touchCompleted = true;
            });
            
            // Wait for both threads to finish
            if (touchThread.joinable()) {
                touchThread.join();
            }
            if (checkThread.joinable()) {
                checkThread.join();
            }
            
            return componentFound;
        } catch (const std::exception& e) {
            error = ApiCallErr(ERR_INTERNAL, "Drag operation failed: " + std::string(e.what()));
            return false;
        }
    }

    bool UiDriver::IsComponentPresentWhenSwipe(const WidgetSelector& selector, const TouchAction& action,
        const UiOpArgs& uiOpArgs, ApiCallErr& error)
    {
        try {
            // Calculate operation time based on swipe velocity and distance
            int32_t operationTime = uiOpArgs.swipeVelocityPps_ > 0 ?
                static_cast<int32_t>(1000 / uiOpArgs.swipeVelocityPps_) : uiOpArgs.defaultDuration_;
            int32_t totalTimeout = operationTime + uiOpArgs.delayTime_;
            int32_t startTime = static_cast<int32_t>(GetCurrentMillisecond());
            
            // Start component presence checking in a separate thread
            std::atomic<bool> componentFound(false);
            std::atomic<bool> touchCompleted(false);
            std::thread checkThread([this, &componentFound, &selector, startTime, totalTimeout,
                operationTime, &uiOpArgs, &touchCompleted]() {
                TimeoutParams timeoutParams(startTime, totalTimeout, operationTime, uiOpArgs);
                ApiCallErr threadError(NO_ERROR);
                componentFound = CheckComponentPresenceWithTimeout(selector, timeoutParams, threadError);
                if (threadError.code_ != NO_ERROR) {
                    LOG_W("Component presence check thread encountered error: %{public}s (code: %{public}d)",
                          threadError.message_.c_str(), threadError.code_);
                }
            });
            
            // Start touch operation in a separate thread
            std::thread touchThread([this, &action, &uiOpArgs, &error, &touchCompleted]() {
                this->PerformTouch(action, uiOpArgs, error);
                touchCompleted = true;
            });
            
            // Wait for both threads to finish
            if (touchThread.joinable()) {
                touchThread.join();
            }
            if (checkThread.joinable()) {
                checkThread.join();
            }
            
            return componentFound;
        } catch (const std::exception& e) {
            error = ApiCallErr(ERR_INTERNAL, "Swipe operation failed: " + std::string(e.what()));
            return false;
        }
    }

    bool UiDriver::IsKnuckleSnapshotEnable()
    {
#ifdef ARKXTEST_KNUCKLE_ACTION_ENABLE
        return uiController_->IsKnuckleSnapshotEnable();
#endif
        return false;
    }

    bool UiDriver::IsKnuckleRecordEnable()
    {
#ifdef ARKXTEST_KNUCKLE_ACTION_ENABLE
        return uiController_->IsKnuckleRecordEnable();
#endif
        return false;
    }
} // namespace OHOS::uitest
