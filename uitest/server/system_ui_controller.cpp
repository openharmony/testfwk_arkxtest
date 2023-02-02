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

    private:
        function<void()> onConnectCallback_ = nullptr;
        function<void()> onDisConnectCallback_ = nullptr;
        atomic<uint64_t> lastEventMillis_ = 0;
    };

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

    void UiEventMonitor::OnAccessibilityEvent(const AccessibilityEventInfo &eventInfo)
    {
        LOG_W("OnEvent:0x%{public}x", eventInfo.GetEventType());
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

    SysUiController::SysUiController(string_view name) : UiController(name) {}

    SysUiController::~SysUiController()
    {
        DisConnectFromSysAbility();
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

    static void MarshalAccessibilityNodeAttributes(AccessibilityElementInfo &node, json &to)
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
        const auto bounds = node.GetRectInScreen();
        const auto rect = Rect(bounds.GetLeftTopXScreenPostion(), bounds.GetRightBottomXScreenPostion(),
                               bounds.GetLeftTopYScreenPostion(), bounds.GetRightBottomYScreenPostion());
        stringstream stream;
        // "[%d,%d][%d,%d]", rect.left, rect.top, rect.right, rect.bottom
        stream << "[" << rect.left_ << "," << rect.top_ << "]" << "[" << rect.right_ << "," << rect.bottom_ << "]";
        to[ATTR_NAMES[UiAttr::BOUNDS].data()] = stream.str();
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

    static void MarshallAccessibilityNodeInfo(AccessibilityElementInfo &from, json &to, int32_t index, bool isDecorBar)
    {
        json attributes;
        MarshalAccessibilityNodeAttributes(from, attributes);
        if (isDecorBar || from.GetInspectorKey() == "ContainerModalTitleRow") {
            attributes[ATTR_NAMES[UiAttr::TYPE].data()] = "DecorBar";
        }
        attributes["index"] = to_string(index);
        to["attributes"] = attributes;
        auto childList = json::array();
        const auto childCount = from.GetChildCount();
        AccessibilityElementInfo child;
        auto ability = AccessibilityUITestAbility::GetInstance();
        for (auto idx = 0; idx < childCount; idx++) {
            auto ret = ability->GetChildElementInfo(idx, from, child);
            if (ret == RET_OK) {
                isDecorBar = false;
                if (child.GetComponentType() == "rootdecortag") {
                    AccessibilityElementInfo child2;
                    (ability->GetChildElementInfo(0, child, child2) == RET_OK && child2.IsVisible()) ||
                    (ability->GetChildElementInfo(1, child, child2) == RET_OK && child2.IsVisible());
                    child = child2;
                    isDecorBar = true;
                }
                if (!child.IsVisible()) {
                    LOG_I("invisible node drop, id: %{public}d", child.GetAccessibilityId());
                    continue;
                }
                auto parcel = json();
                MarshallAccessibilityNodeInfo(child, parcel, idx, isDecorBar);
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
        auto rect = node.GetRectInScreen();
        info.bounds_ = Rect(rect.GetLeftTopXScreenPostion(), rect.GetRightBottomXScreenPostion(),
                            rect.GetLeftTopYScreenPostion(), rect.GetRightBottomYScreenPostion());
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

    void SysUiController::GetUiHierarchy(vector<pair<Window, nlohmann::json>> &out)
    {
        if (!connected_) {
            LOG_I("Not connect to AccessibilityUITestAbility, try to connect it");
            if (!this->ConnectToSysAbility()) {
                LOG_W("Connect to AccessibilityUITestAbility failed");
                return;
            }
        }
        auto ability = AccessibilityUITestAbility::GetInstance();
        vector<AccessibilityWindowInfo> windows;
        if (ability->GetWindows(windows) != RET_OK) {
            LOG_W("GetWindows from AccessibilityUITestAbility failed");
            return;
        }
        sort(windows.begin(), windows.end(), [](auto &w1, auto &w2) -> bool {
            return w1.GetWindowLayer() > w2.GetWindowLayer();
        });
        AccessibilityElementInfo elementInfo;
        for (auto &window : windows) {
            if (ability->GetRootByWindow(window, elementInfo) == RET_OK) {
                const auto app = elementInfo.GetBundleName();
                LOG_D("Get window at layer %{public}d, appId: %{public}s", window.GetWindowLayer(), app.c_str());
                auto winInfo = Window(window.GetWindowId());
                InflateWindowInfo(window, winInfo);
                winInfo.bundleName_ = app;
                // apply window bounds as root node bounds
                auto windowBounds = window.GetRectInScreen();
                elementInfo.SetRectInScreen(windowBounds);
                auto root = nlohmann::json();
                MarshallAccessibilityNodeInfo(elementInfo, root, 0, false);
                out.push_back(make_pair(move(winInfo), move(root)));
            } else {
                LOG_W("GetRootByWindow failed");
            }
        }
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
        return true;
    }

    bool SysUiController::GetCharKeyCode(char ch, int32_t &code, int32_t &ctrlCode) const
    {
        static const map<char, int32_t> keyMap = {
            {'*',    OHOS::MMI::KeyEvent::KEYCODE_STAR},
            {'#',    OHOS::MMI::KeyEvent::KEYCODE_POUND},
            {',',    OHOS::MMI::KeyEvent::KEYCODE_COMMA},
            {'.',    OHOS::MMI::KeyEvent::KEYCODE_PERIOD},
            {'`',    OHOS::MMI::KeyEvent::KEYCODE_GRAVE},
            {'-',    OHOS::MMI::KeyEvent::KEYCODE_MINUS},
            {'=',    OHOS::MMI::KeyEvent::KEYCODE_EQUALS},
            {'[',    OHOS::MMI::KeyEvent::KEYCODE_LEFT_BRACKET},
            {']',    OHOS::MMI::KeyEvent::KEYCODE_RIGHT_BRACKET},
            {'\\',   OHOS::MMI::KeyEvent::KEYCODE_BACKSLASH},
            {';',    OHOS::MMI::KeyEvent::KEYCODE_SEMICOLON},
            {'\'',   OHOS::MMI::KeyEvent::KEYCODE_APOSTROPHE},
            {'/',    OHOS::MMI::KeyEvent::KEYCODE_SLASH},
            {'@',    OHOS::MMI::KeyEvent::KEYCODE_AT},
            {'+',    OHOS::MMI::KeyEvent::KEYCODE_PLUS},
            {'    ', OHOS::MMI::KeyEvent::KEYCODE_TAB},
            {' ',    OHOS::MMI::KeyEvent::KEYCODE_SPACE},
            {0x7F,   OHOS::MMI::KeyEvent::KEYCODE_DEL}};
        ctrlCode = KEYCODE_NONE;
        if (ch >= 'a' && ch <= 'z') {
            code = OHOS::MMI::KeyEvent::KEYCODE_A + static_cast<int32_t>(ch - 'a');
        } else if (ch >= 'A' && ch <= 'Z') {
            ctrlCode = OHOS::MMI::KeyEvent::KEYCODE_SHIFT_LEFT;
            code = OHOS::MMI::KeyEvent::KEYCODE_A + static_cast<int32_t>(ch - 'A');
        } else if (ch >= '0' && ch <= '9') {
            code = OHOS::MMI::KeyEvent::KEYCODE_0 + static_cast<int32_t>(ch - '0');
        } else {
            auto find = keyMap.find(ch);
            if (find != keyMap.end()) {
                code = find->second;
            } else {
                LOG_W("No keyCode found for char '%{public}c'", ch);
                return false;
            }
        }
        return true;
    }

    bool SysUiController::TakeScreenCap(string_view savePath, stringstream &errReceiver) const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        // get PixelMap from DisplayManager API
        shared_ptr<PixelMap> pixelMap = displayMgr.GetScreenshot(displayMgr.GetDefaultDisplayId());
        if (pixelMap == nullptr) {
            errReceiver << "Failed to get display pixelMap";
            return false;
        }
        FILE *fp = fopen(savePath.data(), "wb");
        if (fp == nullptr) {
            perror("File opening failed");
            errReceiver << "File opening failed: " << savePath;
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

    // UiEventMonitor instance.
    static shared_ptr<UiEventMonitor> g_monitorInstance_;

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
        auto screenId = display->GetScreenId();
        ScreenManager &screenMgr = ScreenManager::GetInstance();
        auto screen = screenMgr.GetScreenById(screenId);
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
        auto rotation = (DisplayRotation)display->GetRotation();
        return rotation;
    }

    void SysUiController::SetDisplayRotationEnabled(bool enabled) const
    {
        ScreenManager &screenMgr = ScreenManager::GetInstance();
        screenMgr.SetScreenRotationLocked(enabled);
    }

    Point SysUiController::GetDisplaySize() const
    {
        LOG_I("SysUiController::GetDisplaySize");
        auto display = DisplayManager::GetInstance().GetDefaultDisplay();
        auto width = display->GetWidth();
        auto height = display->GetHeight();
        Point result(width, height);
        return result;
    }

    Point SysUiController::GetDisplayDensity() const
    {
        auto display = DisplayManager::GetInstance().GetDefaultDisplay();
        auto rate = display->GetVirtualPixelRatio();
        Point displaySize = GetDisplaySize();
        Point result(displaySize.px_ * rate, displaySize.py_ * rate);
        return result;
    }

    bool SysUiController::IsScreenOn() const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        auto displayId = displayMgr.GetDefaultDisplayId();
        auto state = displayMgr.GetDisplayState(displayId);
        return (state != DisplayState::OFF);
    }
} // namespace OHOS::uitest