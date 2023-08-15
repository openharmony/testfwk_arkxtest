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

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <iostream>
#include <thread>
#include <utility>
#include <condition_variable>
#include "accessibility_event_info.h"
#include "accessibility_ui_test_ability.h"
#include "ability_manager_client.h"
#include "display_manager.h"
#include "screen_manager.h"
#include "input_manager.h"
#include "png.h"
#include "wm_common.h"
#include "system_ui_controller.h"

using namespace std;
using namespace chrono;

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    using namespace OHOS::MMI;
    using namespace OHOS::Accessibility;
    using namespace OHOS::Rosen;
    using namespace OHOS::Media;

    constexpr auto g_sceneboardId = 1;

    class UiEventMonitor final : public AccessibleAbilityListener {
    public:
        virtual ~UiEventMonitor() override = default;

        void OnAbilityConnected() override;

        void OnAbilityDisconnected() override;

        void OnAccessibilityEvent(const AccessibilityEventInfo &eventInfo) override;

        void SetOnAbilityConnectCallback(function<void()> onConnectCb);

        void SetOnAbilityDisConnectCallback(function<void()> onDisConnectCb);

        bool OnKeyPressEvent(const shared_ptr<MMI::KeyEvent> &keyEvent) override
        {
            return false;
        }

        uint64_t GetLastEventMillis();

        bool WaitEventIdle(uint32_t idleThresholdMs, uint32_t timeoutMs);

        void RegisterUiEventListener(shared_ptr<UiEventListener> listerner);

    private:
        function<void()> onConnectCallback_ = nullptr;
        function<void()> onDisConnectCallback_ = nullptr;
        atomic<uint64_t> lastEventMillis_ = 0;
        vector<shared_ptr<UiEventListener>> listeners_;
    };

    struct EventSpec {
        std::string_view componentTyep;
        int32_t eventType;
        std::string_view event;
    };

    static constexpr EventSpec WATCHED_EVENTS[] = {
        {"Toast", WindowsContentChangeTypes::CONTENT_CHANGE_TYPE_SUBTREE, "toastShow"},
        {"AlertDialog", WindowsContentChangeTypes::CONTENT_CHANGE_TYPE_SUBTREE, "dialogShow"}
    };

    static std::string GetWatchedEvent(const AccessibilityEventInfo &eventInfo)
    {
        for (unsigned long index = 0; index < sizeof(WATCHED_EVENTS) / sizeof(EventSpec); index++) {
            if (WATCHED_EVENTS[index].componentTyep == eventInfo.GetComponentType() &&
                WATCHED_EVENTS[index].eventType == eventInfo.GetWindowContentChangeTypes()) {
                LOG_W("Capture event: %{public}s", WATCHED_EVENTS[index].event.data());
                return string(WATCHED_EVENTS[index].event);
            }
        }
        return "undefine";
    }

    // UiEventMonitor instance.
    static shared_ptr<UiEventMonitor> g_monitorInstance_;

    void UiEventMonitor::SetOnAbilityConnectCallback(function<void()> onConnectCb)
    {
        onConnectCallback_ = std::move(onConnectCb);
    }

    void UiEventMonitor::SetOnAbilityDisConnectCallback(function<void()> onDisConnectCb)
    {
        onDisConnectCallback_ = std::move(onDisConnectCb);
    }

    void UiEventMonitor::OnAbilityConnected()
    {
        if (onConnectCallback_ != nullptr) {
            onConnectCallback_();
        }
    }

    void UiEventMonitor::OnAbilityDisconnected()
    {
        if (onDisConnectCallback_ != nullptr) {
            onDisConnectCallback_();
        }
    }

    // the monitored events
    static constexpr uint32_t EVENT_MASK = EventType::TYPE_VIEW_TEXT_UPDATE_EVENT |
                                           EventType::TYPE_PAGE_STATE_UPDATE |
                                           EventType::TYPE_PAGE_CONTENT_UPDATE |
                                           EventType::TYPE_VIEW_SCROLLED_EVENT |
                                           EventType::TYPE_WINDOW_UPDATE;

    void UiEventMonitor::RegisterUiEventListener(std::shared_ptr<UiEventListener> listerner)
    {
        listeners_.emplace_back(listerner);
    }

    void UiEventMonitor::OnAccessibilityEvent(const AccessibilityEventInfo &eventInfo)
    {
        LOG_W("OnEvent:0x%{public}x", eventInfo.GetEventType());
        auto capturedEvent = GetWatchedEvent(eventInfo);
        if (capturedEvent != "undefine") {
                auto bundleName = eventInfo.GetBundleName();
                auto contentList = eventInfo.GetContentList();
                auto text = !contentList.empty() ? contentList[0] : "";
                auto type = eventInfo.GetComponentType();
                UiEventSourceInfo uiEventSourceInfo = {bundleName, text, type};
            for (auto &listener : listeners_) {
                listener->OnEvent(capturedEvent, uiEventSourceInfo);
            }
        }
        if ((eventInfo.GetEventType() & EVENT_MASK) > 0) {
            lastEventMillis_.store(GetCurrentMillisecond());
        }
    }

    uint64_t UiEventMonitor::GetLastEventMillis()
    {
        if (lastEventMillis_.load() <= 0) {
            lastEventMillis_.store(GetCurrentMillisecond());
        }
        return lastEventMillis_.load();
    }

    bool UiEventMonitor::WaitEventIdle(uint32_t idleThresholdMs, uint32_t timeoutMs)
    {
        const auto currentMs = GetCurrentMillisecond();
        if (lastEventMillis_.load() <= 0) {
            lastEventMillis_.store(currentMs);
        }
        if (currentMs - lastEventMillis_.load() >= idleThresholdMs) {
            return true;
        }
        static constexpr auto sliceMs = 10;
        this_thread::sleep_for(chrono::milliseconds(sliceMs));
        if (timeoutMs <= sliceMs) {
            return false;
        }
        return WaitEventIdle(idleThresholdMs, timeoutMs - sliceMs);
    }

    SysUiController::SysUiController() : UiController() {}

    SysUiController::~SysUiController()
    {
        DisConnectFromSysAbility();
    }

    bool SysUiController::Initialize()
    {
        return this->ConnectToSysAbility();
    }

    static size_t GenerateNodeHash(AccessibilityElementInfo &node)
    {
        static constexpr auto SHIFT_BITS = 32U;
        static constexpr auto hashFunc = hash<string>();
        int64_t intId = node.GetWindowId();
        intId = (intId << SHIFT_BITS) + node.GetAccessibilityId();
        const string strId = node.GetBundleName() + node.GetComponentType() + to_string(intId);
        return hashFunc(strId);
    }

    static Rect GetVisibleRect(Rect windowBounds, Accessibility::Rect nodeBounds)
    {
        auto leftX = nodeBounds.GetLeftTopXScreenPostion();
        auto topY = nodeBounds.GetLeftTopYScreenPostion();
        auto rightX = nodeBounds.GetRightBottomXScreenPostion();
        auto bottomY = nodeBounds.GetRightBottomYScreenPostion();
        Rect newBounds((leftX < windowBounds.left_) ? windowBounds.left_ : leftX,
                       (rightX > windowBounds.right_) ? windowBounds.right_ : rightX,
                       (topY < windowBounds.top_) ? windowBounds.top_ : topY,
                       (bottomY > windowBounds.bottom_) ? windowBounds.bottom_ : bottomY);
        return newBounds;
    }

    static void MarshalAccessibilityNodeAttributes(AccessibilityElementInfo &node, json &to, Rect windowBounds)
    {
        to[ATTR_NAMES[UiAttr::HASHCODE].data()] = to_string(GenerateNodeHash(node));
        to[ATTR_NAMES[UiAttr::TEXT].data()] = node.GetContent();
        to[ATTR_NAMES[UiAttr::ACCESSIBILITY_ID].data()] = to_string(node.GetAccessibilityId());
        to[ATTR_NAMES[UiAttr::ID].data()] = node.GetInspectorKey();
        to[ATTR_NAMES[UiAttr::KEY].data()] = node.GetInspectorKey();
        to[ATTR_NAMES[UiAttr::TYPE].data()] = node.GetComponentType();
        to[ATTR_NAMES[UiAttr::ENABLED].data()] = node.IsEnabled() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::FOCUSED].data()] = node.IsFocused() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::SELECTED].data()] = node.IsSelected() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::CHECKABLE].data()] = node.IsCheckable() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::CHECKED].data()] = node.IsChecked() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::CLICKABLE].data()] = "false";
        to[ATTR_NAMES[UiAttr::LONG_CLICKABLE].data()] = "false";
        to[ATTR_NAMES[UiAttr::SCROLLABLE].data()] = "false";
        to[ATTR_NAMES[UiAttr::HOST_WINDOW_ID].data()] = to_string(node.GetWindowId());
        to[ATTR_NAMES[UiAttr::VISIBLE].data()] = "false";
        const auto bounds = node.GetRectInScreen();
        const auto rect = GetVisibleRect(windowBounds, bounds);
        stringstream stream;
        // "[%d,%d][%d,%d]", rect.left, rect.top, rect.right, rect.bottom
        stream << "[" << rect.left_ << "," << rect.top_ << "]" << "[" << rect.right_ << "," << rect.bottom_ << "]";
        to[ATTR_NAMES[UiAttr::BOUNDS].data()] = stream.str();
        to[ATTR_NAMES[UiAttr::ORIGBOUNDS].data()] = stream.str();
        auto actionList = node.GetActionList();
        for (auto &action : actionList) {
            switch (action.GetActionType()) {
                case ACCESSIBILITY_ACTION_CLICK:
                    to[ATTR_NAMES[UiAttr::CLICKABLE].data()] = "true";
                    break;
                case ACCESSIBILITY_ACTION_LONG_CLICK:
                    to[ATTR_NAMES[UiAttr::LONG_CLICKABLE].data()] = "true";
                    break;
                case ACCESSIBILITY_ACTION_SCROLL_FORWARD:
                case ACCESSIBILITY_ACTION_SCROLL_BACKWARD:
                    to[ATTR_NAMES[UiAttr::SCROLLABLE].data()] = "true";
                    break;
                default:
                    break;
            }
        }
    }

    static void MarshallAccessibilityNodeInfo(AccessibilityElementInfo &from, json &to, int32_t index,
        Rect windowBounds, bool visitChild)
    {
        json attributes;
        MarshalAccessibilityNodeAttributes(from, attributes, windowBounds);
        if (from.GetComponentType() == "rootdecortag" || from.GetInspectorKey() == "ContainerModalTitleRow") {
            attributes[ATTR_NAMES[UiAttr::TYPE].data()] = "DecorBar";
        }
        attributes["index"] = to_string(index);
        to["attributes"] = attributes;
        auto childList = json::array();
        if (!visitChild) {
            to["children"] = childList;
            return;
        }
        const auto childCount = from.GetChildCount();
        AccessibilityElementInfo child;
        auto ability = AccessibilityUITestAbility::GetInstance();
        for (auto idx = 0; idx < childCount; idx++) {
            auto ret = ability->GetChildElementInfo(idx, from, child);
            if (ret == RET_OK) {
                if (!child.IsVisible()) {
                    LOG_I("This node is not visible, node Id: %{public}d", child.GetAccessibilityId());
                    continue;
                }
                auto parcel = json();
                MarshallAccessibilityNodeInfo(child, parcel, idx, windowBounds, visitChild);
                childList.push_back(parcel);
            } else {
                LOG_W("Get Node child at index=%{public}d failed", idx);
            }
        }
        to["children"] = childList;
    }

    static void InflateWindowInfo(AccessibilityWindowInfo& node, Window& info)
    {
        info.focused_ = node.IsFocused();
        info.actived_ = node.IsActive();
        info.decoratorEnabled_ = node.IsDecorEnable();
        info.mode_ = WindowMode::UNKNOWN;
        const auto origMode = static_cast<OHOS::Rosen::WindowMode>(node.GetWindowMode());
        switch (origMode) {
            case OHOS::Rosen::WindowMode::WINDOW_MODE_FULLSCREEN:
                info.mode_ = WindowMode::FULLSCREEN;
                break;
            case OHOS::Rosen::WindowMode::WINDOW_MODE_SPLIT_PRIMARY:
                info.mode_ = WindowMode::SPLIT_PRIMARY;
                break;
            case OHOS::Rosen::WindowMode::WINDOW_MODE_SPLIT_SECONDARY:
                info.mode_ = WindowMode::SPLIT_SECONDARY;
                break;
            case OHOS::Rosen::WindowMode::WINDOW_MODE_FLOATING:
                info.mode_ = WindowMode::FLOATING;
                break;
            case OHOS::Rosen::WindowMode::WINDOW_MODE_PIP:
                info.mode_ = WindowMode::PIP;
                break;
            default:
                info.mode_ = WindowMode::UNKNOWN;
                break;
        }
    }

    static bool GetAamsWindowInfos(vector<AccessibilityWindowInfo> &windows)
    {
        auto ability = AccessibilityUITestAbility::GetInstance();
        if (ability->GetWindows(windows) != RET_OK) {
            LOG_W("GetWindows from AccessibilityUITestAbility failed");
            return false;
        }
        sort(windows.begin(), windows.end(), [](auto &w1, auto &w2) -> bool {
            if (w1.GetWindowId() == g_sceneboardId) {
                return false;
            } else if (w2.GetWindowId() == g_sceneboardId) {
                return true;
            }
            return w1.GetWindowLayer() > w2.GetWindowLayer();
        });
        return true;
    }

    void SysUiController::GetUiHierarchy(vector<pair<Window, nlohmann::json>> &out, bool getWidgetNodes,
        string targetApp)
    {
        static mutex dumpMutex; // disallow concurrent dumpUi
        if (!connected_ && !ConnectToSysAbility()) {
            LOG_W("Connect to AccessibilityUITestAbility failed");
            return;
        }
        dumpMutex.lock();
        vector<AccessibilityWindowInfo> windows;
        if (!GetAamsWindowInfos(windows)) {
            dumpMutex.unlock();
            return;
        }
        auto screenSize = GetDisplaySize();
        AccessibilityElementInfo elementInfo;
        const auto foreAbility = AAFwk::AbilityManagerClient::GetInstance()->GetTopAbility();
        vector<Rect> overlays;
        for (auto &window : windows) {
            if (AccessibilityUITestAbility::GetInstance()->GetRootByWindow(window, elementInfo) == RET_OK) {
                const auto app = elementInfo.GetBundleName();
                LOG_D("Get window at layer %{public}d, appId: %{public}s, windowId: %{public}d",
                    window.GetWindowLayer(), app.c_str(), window.GetWindowId());
                if (targetApp != "" && app != targetApp) {
                    continue;
                }
                // apply window bounds as root node bounds
                auto screenRect = Rect(0, screenSize.px_, 0, screenSize.py_);
                auto boundsInScreen = GetVisibleRect(screenRect, window.GetRectInScreen());
                auto winInfo = Window(window.GetWindowId());
                InflateWindowInfo(window, winInfo);
                winInfo.bounds_ = (window.GetWindowId() == g_sceneboardId) ? screenRect : boundsInScreen;
                winInfo.bundleName_ = app;
                Rect visibleArea = winInfo.bounds_;
                if (!RectAlgorithm::ComputeMaxVisibleRegion(winInfo.bounds_, overlays, visibleArea)) {
                    continue;
                }
                auto root = nlohmann::json();
                root["bundleName"] = app;
                root["abilityName"] = (app == foreAbility.GetBundleName()) ? foreAbility.GetAbilityName() : "";
                root["pagePath"] = (app == foreAbility.GetBundleName()) ? elementInfo.GetPagePath() : "";
                MarshallAccessibilityNodeInfo(elementInfo, root, 0, winInfo.bounds_, getWidgetNodes);
                overlays.push_back(winInfo.bounds_);
                out.push_back(make_pair(move(winInfo), move(root)));
                LOG_D("Get node at layer %{public}d, appId: %{public}s", window.GetWindowLayer(), app.c_str());
            } else {
                LOG_W("GetRootByWindow failed, windowId: %{public}d", window.GetWindowId());
            }
        }
        dumpMutex.unlock();
    }

    void SysUiController::InjectTouchEventSequence(const PointerMatrix &events) const
    {
        for (uint32_t step = 0; step < events.GetSteps(); step++) {
            auto pointerEvent = PointerEvent::Create();
            for (uint32_t finger = 0; finger < events.GetFingers(); finger++) {
                pointerEvent->SetPointerId(finger);
                PointerEvent::PointerItem pinterItem;
                pinterItem.SetPointerId(finger);
                pinterItem.SetDisplayX(events.At(finger, step).point_.px_);
                pinterItem.SetDisplayY(events.At(finger, step).point_.py_);
                switch (events.At(finger, step).stage_) {
                    case ActionStage::DOWN:
                        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
                        break;
                    case ActionStage::MOVE:
                        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
                        break;
                    case ActionStage::UP:
                        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
                        break;
                }
                pinterItem.SetPressed(events.At(finger, step).stage_ != ActionStage::UP);
                pointerEvent->AddPointerItem(pinterItem);
                pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
                DisplayManager &displayMgr = DisplayManager::GetInstance();
                pointerEvent->SetTargetDisplayId(displayMgr.GetDefaultDisplayId());
                InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
                if (events.At(finger, step).holdMs_ > 0) {
                    this_thread::sleep_for(chrono::milliseconds(events.At(finger, step).holdMs_));
                }
            }
        }
    }

    static std::shared_ptr<OHOS::MMI::PointerEvent> CreateMouseActionEvent(MouseOpArgs mouseOpArgs,
        MouseEventType action)
    {
        auto pointerEvent = PointerEvent::Create();
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
        pointerEvent->SetPointerId(0);
        switch (action) {
            case MouseEventType::M_MOVE:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_MOVE);
                break;
            case MouseEventType::AXIS_BEGIN:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_BEGIN);
                break;
            case MouseEventType::AXIS_END:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_END);
                break;
            case MouseEventType::BUTTON_DOWN:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_BUTTON_DOWN);
                break;
            case MouseEventType::BUTTON_UP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_BUTTON_UP);
                break;
            default:
                break;
        }
        PointerEvent::PointerItem item;
        if (action == MouseEventType::AXIS_BEGIN || action == MouseEventType::AXIS_END) {
            constexpr double axialValue = 15;
            auto injectAxialValue = mouseOpArgs.adown_ ? axialValue : -axialValue;
            pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, injectAxialValue);
            item.SetDownTime(0);
        } else if (action == MouseEventType::BUTTON_DOWN || action == MouseEventType::BUTTON_UP) {
            pointerEvent->SetButtonId(mouseOpArgs.button_);
            pointerEvent->SetButtonPressed(mouseOpArgs.button_);
        }
        item.SetDisplayX(mouseOpArgs.point_.px_);
        item.SetDisplayY(mouseOpArgs.point_.py_);
        item.SetPressed(action == MouseEventType::BUTTON_DOWN);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }

    static std::shared_ptr<OHOS::MMI::KeyEvent> CreateSingleKeyEvent(int32_t key, ActionStage action)
    {
        auto keyEvent = OHOS::MMI::KeyEvent::Create();
        keyEvent->SetKeyCode(key);
        switch (action) {
            case ActionStage::DOWN:
                keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
                break;
            case ActionStage::UP:
                keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_UP);
                break;
            default:
                break;
        }
        OHOS::MMI::KeyEvent::KeyItem keyItem;
        keyItem.SetKeyCode(key);
        keyItem.SetPressed(action == ActionStage::DOWN);
        keyEvent->AddKeyItem(keyItem);
        return keyEvent;
    }

    void SysUiController::InjectMouseClick(MouseOpArgs mouseOpArgs) const
    {
        constexpr uint32_t focusTimeMs = 40;
        auto mouseMove = CreateMouseActionEvent(mouseOpArgs, MouseEventType::M_MOVE);
        InputManager::GetInstance()->SimulateInputEvent(mouseMove);
        this_thread::sleep_for(chrono::milliseconds(focusTimeMs));
        if (mouseOpArgs.key1_ != KEYCODE_NONE) {
            auto dwonEvent1 = CreateSingleKeyEvent(mouseOpArgs.key1_, ActionStage::DOWN);
            InputManager::GetInstance()->SimulateInputEvent(dwonEvent1);
            this_thread::sleep_for(chrono::milliseconds(focusTimeMs));
            if (mouseOpArgs.key2_ != KEYCODE_NONE) {
                auto dwonEvent2 = CreateSingleKeyEvent(mouseOpArgs.key2_, ActionStage::DOWN);
                InputManager::GetInstance()->SimulateInputEvent(dwonEvent2);
                this_thread::sleep_for(chrono::milliseconds(focusTimeMs));
            }
        }
        auto mouseDown = CreateMouseActionEvent(mouseOpArgs, MouseEventType::BUTTON_DOWN);
        InputManager::GetInstance()->SimulateInputEvent(mouseDown);
        auto mouseUp = CreateMouseActionEvent(mouseOpArgs, MouseEventType::BUTTON_UP);
        InputManager::GetInstance()->SimulateInputEvent(mouseUp);
        if (mouseOpArgs.key2_ != KEYCODE_NONE) {
            auto upEvent = CreateSingleKeyEvent(mouseOpArgs.key2_, ActionStage::UP);
            InputManager::GetInstance()->SimulateInputEvent(upEvent);
            this_thread::sleep_for(chrono::milliseconds(focusTimeMs));
        }
        if (mouseOpArgs.key1_ != KEYCODE_NONE) {
            auto upEvent = CreateSingleKeyEvent(mouseOpArgs.key1_, ActionStage::UP);
            InputManager::GetInstance()->SimulateInputEvent(upEvent);
            this_thread::sleep_for(chrono::milliseconds(focusTimeMs));
        }
    }

    void SysUiController::InjectMouseScroll(MouseOpArgs mouseOpArgs) const
    {
        if (mouseOpArgs.key1_ != KEYCODE_NONE) {
            auto dwonEvent1 = CreateSingleKeyEvent(mouseOpArgs.key1_, ActionStage::DOWN);
            InputManager::GetInstance()->SimulateInputEvent(dwonEvent1);
            if (mouseOpArgs.key2_ != KEYCODE_NONE) {
                auto dwonEvent2 = CreateSingleKeyEvent(mouseOpArgs.key2_, ActionStage::DOWN);
                InputManager::GetInstance()->SimulateInputEvent(dwonEvent2);
            }
        }
        auto mouseMove = CreateMouseActionEvent(mouseOpArgs, MouseEventType::M_MOVE);
        InputManager::GetInstance()->SimulateInputEvent(mouseMove);
        constexpr uint32_t focusTimeMs = 40;
        for (auto index = 0; index < mouseOpArgs.scrollValue_; index++) {
            auto mouseScroll1 = CreateMouseActionEvent(mouseOpArgs, MouseEventType::AXIS_BEGIN);
            InputManager::GetInstance()->SimulateInputEvent(mouseScroll1);
            auto mouseScroll2 = CreateMouseActionEvent(mouseOpArgs, MouseEventType::AXIS_END);
            InputManager::GetInstance()->SimulateInputEvent(mouseScroll2);
            this_thread::sleep_for(chrono::milliseconds(focusTimeMs));
        }
        if (mouseOpArgs.key2_ != KEYCODE_NONE) {
            auto upEvent = CreateSingleKeyEvent(mouseOpArgs.key2_, ActionStage::UP);
            InputManager::GetInstance()->SimulateInputEvent(upEvent);
        }
        if (mouseOpArgs.key1_ != KEYCODE_NONE) {
            auto upEvent = CreateSingleKeyEvent(mouseOpArgs.key1_, ActionStage::UP);
            InputManager::GetInstance()->SimulateInputEvent(upEvent);
        }
    }

    void SysUiController::InjectMouseMove(MouseOpArgs mouseOpArgs) const
    {
        auto event = CreateMouseActionEvent(mouseOpArgs, MouseEventType::M_MOVE);
        InputManager::GetInstance()->SimulateInputEvent(event);
    }

    void SysUiController::InjectKeyEventSequence(const vector<KeyEvent> &events) const
    {
        vector<int32_t> downKeys;
        for (auto &event : events) {
            if (event.stage_ == ActionStage::UP) {
                auto iter = std::find(downKeys.begin(), downKeys.end(), event.code_);
                if (iter == downKeys.end()) {
                    LOG_W("Cannot release a not-pressed key: %{public}d", event.code_);
                    continue;
                }
                downKeys.erase(iter);
                auto keyEvent = OHOS::MMI::KeyEvent::Create();
                keyEvent->SetKeyCode(event.code_);
                keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_UP);
                OHOS::MMI::KeyEvent::KeyItem keyItem;
                keyItem.SetKeyCode(event.code_);
                keyItem.SetPressed(true);
                keyEvent->AddKeyItem(keyItem);
                InputManager::GetInstance()->SimulateInputEvent(keyEvent);
            } else {
                downKeys.push_back(event.code_);
                auto keyEvent = OHOS::MMI::KeyEvent::Create();
                for (auto downKey : downKeys) {
                    keyEvent->SetKeyCode(downKey);
                    keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
                    OHOS::MMI::KeyEvent::KeyItem keyItem;
                    keyItem.SetKeyCode(downKey);
                    keyItem.SetPressed(true);
                    keyEvent->AddKeyItem(keyItem);
                }
                InputManager::GetInstance()->SimulateInputEvent(keyEvent);
                if (event.holdMs_ > 0) {
                    this_thread::sleep_for(chrono::milliseconds(event.holdMs_));
                }
            }
        }
        // check not released keys
        for (auto downKey : downKeys) {
            LOG_W("Key event sequence injections done with not-released key: %{public}d", downKey);
        }
    }

    void SysUiController::PutTextToClipboard(string_view text) const {}

    bool SysUiController::IsWorkable() const
    {
        return connected_;
    }

    bool SysUiController::GetCharKeyCode(char ch, int32_t &code, int32_t &ctrlCode) const
    {
        ctrlCode = KEYCODE_NONE;
        if (ch >= 'a' && ch <= 'z') {
            code = OHOS::MMI::KeyEvent::KEYCODE_A + static_cast<int32_t>(ch - 'a');
        } else if (ch >= 'A' && ch <= 'Z') {
            ctrlCode = OHOS::MMI::KeyEvent::KEYCODE_SHIFT_LEFT;
            code = OHOS::MMI::KeyEvent::KEYCODE_A + static_cast<int32_t>(ch - 'A');
        } else if (ch >= '0' && ch <= '9') {
            code = OHOS::MMI::KeyEvent::KEYCODE_0 + static_cast<int32_t>(ch - '0');
        } else {
            return false;
        }
        return true;
    }

    bool SysUiController::TakeScreenCap(int32_t fd, std::stringstream &errReceiver, Rect rect) const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        // get PixelMap from DisplayManager API
        shared_ptr<PixelMap> pixelMap;
        if (rect.GetWidth() == 0) {
            pixelMap = displayMgr.GetScreenshot(displayMgr.GetDefaultDisplayId());
        } else {
            Media::Rect region = {.left = rect.left_, .top = rect.top_,
                .width = rect.right_ - rect.left_, .height = rect.bottom_ - rect.top_};
            Media::Size size = {.width = rect.right_ - rect.left_, .height = rect.bottom_ - rect.top_};
            pixelMap = displayMgr.GetScreenshot(displayMgr.GetDefaultDisplayId(), region, size, 0);
        }
        if (pixelMap == nullptr) {
            errReceiver << "Failed to get display pixelMap";
            return false;
        }
        FILE *fp = fdopen(fd, "wb");
        if (fp == nullptr) {
            perror("File opening failed");
            errReceiver << "File opening failed";
            return false;
        }
        png_structp pngStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (pngStruct == nullptr) {
            fclose(fp);
            return false;
        }
        png_infop pngInfo = png_create_info_struct(pngStruct);
        if (pngInfo == nullptr) {
            fclose(fp);
            png_destroy_write_struct(&pngStruct, nullptr);
            return false;
        }
        png_init_io(pngStruct, fp);
        auto width = static_cast<uint32_t>(pixelMap->GetWidth());
        auto height = static_cast<uint32_t>(pixelMap->GetHeight());
        auto data = pixelMap->GetPixels();
        auto stride = static_cast<uint32_t>(pixelMap->GetRowBytes());
        // set png header
        static constexpr int bitmapDepth = 8;
        png_set_IHDR(pngStruct, pngInfo, width, height, bitmapDepth, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_set_packing(pngStruct); // set packing info
        png_write_info(pngStruct, pngInfo); // write to header
        for (uint32_t column = 0; column < height; column++) {
            png_write_row(pngStruct, data + (column * stride));
        }
        // free/close
        png_write_end(pngStruct, pngInfo);
        png_destroy_write_struct(&pngStruct, &pngInfo);
        (void)fclose(fp);
        return true;
    }

    bool SysUiController::ConnectToSysAbility()
    {
        if (connected_) {
            return true;
        }
        mutex mtx;
        unique_lock<mutex> uLock(mtx);
        condition_variable condition;
        auto onConnectCallback = [&condition]() {
            LOG_I("Success connect to AccessibilityUITestAbility");
            condition.notify_all();
        };
        auto onDisConnectCallback = [this]() { this->connected_ = false; };
        if (g_monitorInstance_ == nullptr) {
            g_monitorInstance_ = make_shared<UiEventMonitor>();
        }
        g_monitorInstance_->SetOnAbilityConnectCallback(onConnectCallback);
        g_monitorInstance_->SetOnAbilityDisConnectCallback(onDisConnectCallback);
        auto ability = AccessibilityUITestAbility::GetInstance();
        if (ability->RegisterAbilityListener(g_monitorInstance_) != RET_OK) {
            LOG_E("Failed to register UiEventMonitor");
            return false;
        }
        auto ret = ability->Connect();
        switch (ret) {
            case (RET_ERR_INVALID_PARAM):
                LOG_E("Failed to connect to AccessibilityUITestAbility, RET_ERR_INVALID_PARAM");
                return false;
            case (RET_ERR_NULLPTR):
                LOG_E("Failed to connect to AccessibilityUITestAbility, RET_ERR_NULLPTR");
                return false;
            case (RET_ERR_CONNECTION_EXIST):
                LOG_E("Failed to connect to AccessibilityUITestAbility, RET_ERR_CONNECTION_EXIST");
                return false;
            case (RET_ERR_IPC_FAILED):
                LOG_E("Failed to connect to AccessibilityUITestAbility, RET_ERR_IPC_FAILED");
                return false;
            case (RET_ERR_SAMGR):
                LOG_E("Failed to connect to AccessibilityUITestAbility, RET_ERR_SAMGR");
                return false;
            default:
                break;
        }
        const auto timeout = chrono::milliseconds(1000);
        if (condition.wait_for(uLock, timeout) == cv_status::timeout) {
            LOG_E("Wait connection to AccessibilityUITestAbility timed out");
            return false;
        }
        connected_ = true;
        return true;
    }

    void SysUiController::RegisterUiEventListener(std::shared_ptr<UiEventListener> listener) const
    {
        g_monitorInstance_->RegisterUiEventListener(listener);
    }

    bool SysUiController::WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutMs) const
    {
        return g_monitorInstance_->WaitEventIdle(idleThresholdMs, timeoutMs);
    }

    void SysUiController::DisConnectFromSysAbility()
    {
        if (!connected_ || g_monitorInstance_ == nullptr) {
            return;
        }
        connected_ = false;
        mutex mtx;
        unique_lock<mutex> uLock(mtx);
        condition_variable condition;
        auto onDisConnectCallback = [&condition]() {
            LOG_I("Success disconnect from AccessibilityUITestAbility");
            condition.notify_all();
        };
        g_monitorInstance_->SetOnAbilityDisConnectCallback(onDisConnectCallback);
        auto ability = AccessibilityUITestAbility::GetInstance();
        LOG_I("Start disconnect from AccessibilityUITestAbility");
        if (ability->Disconnect() != RET_OK) {
            LOG_E("Failed to disconnect from AccessibilityUITestAbility");
            return;
        }
        const auto timeout = chrono::milliseconds(200);
        if (condition.wait_for(uLock, timeout) == cv_status::timeout) {
            LOG_E("Wait disconnection from AccessibilityUITestAbility timed out");
            return;
        }
    }

    void SysUiController::SetDisplayRotation(DisplayRotation rotation) const
    {
        auto display = DisplayManager::GetInstance().GetDefaultDisplay();
        if (display == nullptr) {
            LOG_E("DisplayManager init fail");
            return;
        }
        auto screenId = display->GetScreenId();
        ScreenManager &screenMgr = ScreenManager::GetInstance();
        DCHECK(screenMgr);
        auto screen = screenMgr.GetScreenById(screenId);
        if (screen == nullptr) {
            LOG_E("ScreenManager init fail");
            return;
        }
        switch (rotation) {
            case ROTATION_0 :
                screen->SetOrientation(Orientation::VERTICAL);
                break;
            case ROTATION_90 :
                screen->SetOrientation(Orientation::HORIZONTAL);
                break;
            case ROTATION_180 :
                screen->SetOrientation(Orientation::REVERSE_VERTICAL);
                break;
            case ROTATION_270 :
                screen->SetOrientation(Orientation::REVERSE_HORIZONTAL);
                break;
            default :
                break;
        }
    }

    DisplayRotation SysUiController::GetDisplayRotation() const
    {
        auto display = DisplayManager::GetInstance().GetDefaultDisplay();
        if (display == nullptr) {
            LOG_E("DisplayManager init fail");
            return DisplayRotation::ROTATION_0;
        }
        auto rotation = (DisplayRotation)display->GetRotation();
        return rotation;
    }

    void SysUiController::SetDisplayRotationEnabled(bool enabled) const
    {
        ScreenManager &screenMgr = ScreenManager::GetInstance();
        DCHECK(screenMgr);
        screenMgr.SetScreenRotationLocked(enabled);
    }

    Point SysUiController::GetDisplaySize() const
    {
        auto display = DisplayManager::GetInstance().GetDefaultDisplay();
        if (display == nullptr) {
            LOG_E("DisplayManager init fail");
            return {0, 0};
        }
        auto width = display->GetWidth();
        auto height = display->GetHeight();
        Point result(width, height);
        return result;
    }

    Point SysUiController::GetDisplayDensity() const
    {
        auto display = DisplayManager::GetInstance().GetDefaultDisplay();
        if (display == nullptr) {
            LOG_E("DisplayManager init fail");
            return {0, 0};
        }
        auto rate = display->GetVirtualPixelRatio();
        Point displaySize = GetDisplaySize();
        Point result(displaySize.px_ * rate, displaySize.py_ * rate);
        return result;
    }

    bool SysUiController::IsScreenOn() const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        DCHECK(displayMgr);
        auto displayId = displayMgr.GetDefaultDisplayId();
        auto state = displayMgr.GetDisplayState(displayId);
        return (state != DisplayState::OFF);
    }
} // namespace OHOS::uitest