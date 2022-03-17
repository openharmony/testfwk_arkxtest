/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
#include "accessibility_ui_test_ability_listener.h"
#include "display_manager.h"
#include "system_ui_controller.h"
#include "input_manager.h"
#include "png.h"

using namespace std;
using namespace chrono;

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    using namespace OHOS::MMI;
    using namespace OHOS::Accessibility;
    using namespace OHOS::Rosen;
    using namespace OHOS::Media;

    constexpr uint32_t MICRO_TO_NANO_FACTOR = 1000;

    class UiEventMonitor final : public IAccessibleUITestAbilityListener {
    public:
        virtual ~UiEventMonitor() = default;

        void OnAbilityConnected() override;

        void OnAbilityDisconnected() override;

        void OnAccessibilityEvent(const AccessibilityEventInfo &eventInfo) override;

        void SetOnAbilityConnectCallback(function<void()> onConnectCb);

        void SetOnAbilityDisConnectCallback(function<void()> onDisConnectCb);

        bool OnKeyPressEvent(const shared_ptr<MMI::KeyEvent> &keyEvent, const int sequence) override
        {
            return false;
        }

        void OnGestureSimulateResult(const int sequence, const bool completedSuccessfully) override {}

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
    static constexpr uint32_t EVENT_MASK = EventType::TYPE_VIEW_TEXT_UPDATE_EVENT
                                           | EventType::TYPE_PAGE_STATE_UPDATE | EventType::TYPE_PAGE_CONTENT_UPDATE
                                           | EventType::TYPE_VIEW_SCROLLED_EVENT | EventType::TYPE_WINDOW_UPDATE;

    void UiEventMonitor::OnAccessibilityEvent(const AccessibilityEventInfo &eventInfo)
    {
        LOG_W("OnEvent:0x%{public}x", eventInfo.GetEventType()); // FIXME
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

    SysUiController::SysUiController(string_view name, string_view device) : UiController(name, device) {}

    SysUiController::~SysUiController()
    {
        DisConnectFromSysAbility();
    }

    static void MarshalAccessibilityNodeAttributes(AccessibilityElementInfo &node, json &to)
    {
        to[ATTR_NAMES[UiAttr::TEXT]] = node.GetContent();
        to[ATTR_NAMES[UiAttr::ID]] = to_string(node.GetAccessibilityId());
        to[ATTR_NAMES[UiAttr::TYPE]] = node.GetComponentType();
        to[ATTR_NAMES[UiAttr::ENABLED]] = node.IsEnabled() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::FOCUSED]] = node.IsFocused() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::SELECTED]] = node.IsSelected() ? "true" : "false";
        to[ATTR_NAMES[UiAttr::CLICKABLE]] = "false";
        to[ATTR_NAMES[UiAttr::LONG_CLICKABLE]] = "false";
        to[ATTR_NAMES[UiAttr::SCROLLABLE]] = "false";
        to["visible"] = node.IsVisible() ? "true" : "false";
        auto actionList = node.GetActionList();
        for (auto &action :actionList) {
            switch (action.GetActionType()) {
                case ACCESSIBILITY_ACTION_CLICK:
                    to[ATTR_NAMES[UiAttr::CLICKABLE]] = "true";
                    break;
                case ACCESSIBILITY_ACTION_LONG_CLICK:
                    to[ATTR_NAMES[UiAttr::LONG_CLICKABLE]] = "true";
                    break;
                case ACCESSIBILITY_ACTION_SCROLL_FORWARD:
                case ACCESSIBILITY_ACTION_SCROLL_BACKWARD:
                    to[ATTR_NAMES[UiAttr::SCROLLABLE]] = "true";
                    break;
                default:
                    break;
            }
        }
    }

    static void MarshallAccessibilityNodeInfo(AccessibilityElementInfo &from, json &to, const Rect &rootBounds)
    {
        static constexpr int32_t visibleSizeMin = 10;
        int32_t xOverlap = 0;
        int32_t yOverlap = 0;
        const auto rect = from.GetRectInScreen();
        const auto nodeBounds = Rect{rect.GetLeftTopXScreenPostion(), rect.GetRightBottomXScreenPostion(),
                                     rect.GetLeftTopYScreenPostion(), rect.GetRightBottomYScreenPostion()};
        rootBounds.ComputeOverlappingDimensions(nodeBounds, xOverlap, yOverlap);
        if (xOverlap <= visibleSizeMin || yOverlap <= visibleSizeMin) {
            // node invisible, skip
            LOG_W("Child not in root rect!!!");
        }
        json attributes;
        MarshalAccessibilityNodeAttributes(from, attributes);
        stringstream stream;
        // "[%d,%d][%d,%d]", rect.left, rect.top, rect.right, rect.bottom
        stream << "[" << nodeBounds.left_ << "," << nodeBounds.top_ << "]"
               << "[" << nodeBounds.right_ << "," << nodeBounds.bottom_ << "]";
        attributes[ATTR_NAMES[UiAttr::BOUNDS]] = stream.str();
        to["attributes"] = attributes;
        auto childList = json::array();
        const auto childCount = from.GetChildCount();
        AccessibilityElementInfo childNode;
        for (auto index = 0; index < childCount; index++) {
            auto success = from.GetChild(index, childNode);
            if (success) {
                auto childParcel = json();
                MarshallAccessibilityNodeInfo(childNode, childParcel, rootBounds);
                childList.push_back(childParcel);
            } else {
                LOG_W("Get Node child at index=%{public}d failed", index);
            }
        }
        to["children"] = childList;
    }

    string GetDomTextOfCurrentWindow1()
    {
        auto ability = AccessibilityUITestAbility::GetInstance();
        auto windows = ability->GetWindows();
        if (windows.empty()) {
            LOG_W("No root window found");
            return "{}";
        } else {
            LOG_I("WindowCount=%{public}d", windows.size());
        }
        for (auto &window:windows) {
            LOG_I("WindowLayer=%{public}d", window.GetWindowLayer());
            auto rect = window.GetRectInScreen();
            LOG_I("WindowPosition=(%{public}d,%{public}d),(%{public}d,%{public}d)",
                  rect.GetLeftTopXScreenPostion(), rect.GetLeftTopYScreenPostion(),
                  rect.GetRightBottomXScreenPostion(), rect.GetRightBottomYScreenPostion());
        }

        auto treeParcel = json();
        auto windowInfo = windows.at(0);
        auto rootNode = AccessibilityElementInfo();
        if (windowInfo.GetRootAccessibilityInfo(rootNode)) {
            auto rootRect = rootNode.GetRectInScreen();
            const auto rootBounds = Rect{rootRect.GetLeftTopXScreenPostion(), rootRect.GetRightBottomXScreenPostion(),
                                         rootRect.GetLeftTopYScreenPostion(), rootRect.GetRightBottomYScreenPostion()};
            MarshallAccessibilityNodeInfo(rootNode, treeParcel, rootBounds);
        }
        return treeParcel.dump();
    }

    string GetDomTextOfCurrentWindow2()
    {
        auto ability = AccessibilityUITestAbility::GetInstance();
        std::optional<AccessibilityElementInfo> elementInfo;
        ability->GetRootElementInfo(elementInfo);
        auto treeParcel = json();
        if (elementInfo.has_value()) {
            const auto rect = elementInfo.value().GetRectInScreen();
            const auto rootBounds = Rect{rect.GetLeftTopXScreenPostion(), rect.GetRightBottomXScreenPostion(),
                                         rect.GetLeftTopYScreenPostion(), rect.GetRightBottomYScreenPostion()};
            MarshallAccessibilityNodeInfo(elementInfo.value(), treeParcel, rootBounds);
        } else {
            LOG_I("Root node not found");
        }
        return treeParcel.dump();
    }

    string SysUiController::GetDomTextOfCurrentWindow() const
    { // TODO:: use cached dom
        auto at0 = GetDomTextOfCurrentWindow1();
        LOG_I("TREE0='%{public}s'", at0.c_str());
        auto at1 = GetDomTextOfCurrentWindow2();
        LOG_I("TREE1='%{public}s'", at1.c_str());
        return at1;
    }

    void SysUiController::WaitForUiSteady(uint32_t idleThresholdMs, uint32_t timeoutMs) const
    {
        this_thread::sleep_for(chrono::milliseconds(idleThresholdMs));
        // TODO:: depend on AccessibilityService
        // FIXME  UiEventMonitor::GetInstance()->WaitEventIdle(idleThresholdMs, timeoutMs);
    }

    void SysUiController::InjectTouchEvent(const TouchEvent &te) const
    {
        auto event = PointerEvent::Create();
        PointerEvent::PointerItem item;
        item.SetPointerId(0);
        item.SetGlobalX(te.point_.px_);
        item.SetGlobalY(te.point_.py_);
        const auto micros = static_cast<int64_t>(GetCurrentMicroseconds());
        switch (te.stage_) {
            case ActionStage::DOWN:
                event->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
                item.SetDownTime(micros);
                break;
            case ActionStage::MOVE:
                event->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
                item.SetDownTime(micros - te.downTimeOffsetMs_ * MICRO_TO_NANO_FACTOR);
                break;
            case ActionStage::UP:
                event->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
                item.SetDownTime(micros - te.downTimeOffsetMs_ * MICRO_TO_NANO_FACTOR);
                break;
        }
        item.SetPressed(te.stage_ != ActionStage::UP);
        event->AddPointerItem(item);
        event->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
        event->SetPointerId(0);
        InputManager::GetInstance()->SimulateInputEvent(event);
        if (te.holdMs_ > 0) {
            this_thread::sleep_for(chrono::milliseconds(te.holdMs_));
        }
    }

    void SysUiController::InjectKeyEvent(const KeyEvent &ke) const
    {
        auto event = OHOS::MMI::KeyEvent::Create();
        event->SetKeyCode(ke.code_);
        const auto micros = static_cast<int64_t>(GetCurrentMicroseconds());
        OHOS::MMI::KeyEvent::KeyItem item;
        OHOS::MMI::KeyEvent::KeyItem itemCtrl;
        item.SetKeyCode(ke.code_);
        if (ke.ctrlKey_ != KEYCODE_NONE) {
            itemCtrl.SetKeyCode(ke.ctrlKey_);
        }
        switch (ke.stage_) {
            case ActionStage::DOWN:
                event->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
                item.SetPressed(true);
                item.SetDownTime(micros);
                if (ke.ctrlKey_ != KEYCODE_NONE) {
                    itemCtrl.SetPressed(true);
                    itemCtrl.SetDownTime(micros - 1 * MICRO_TO_NANO_FACTOR);
                    event->AddPressedKeyItems(itemCtrl);
                }
                event->AddPressedKeyItems(item);
                break;
            case ActionStage::UP:
                event->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_UP);
                item.SetPressed(false);
                item.SetDownTime(micros - ke.downTimeOffsetMs_ * MICRO_TO_NANO_FACTOR);
                event->RemoveReleasedKeyItems(item);
                if (ke.ctrlKey_ != KEYCODE_NONE) {
                    itemCtrl.SetPressed(false);
                    itemCtrl.SetDownTime(micros + 1 * MICRO_TO_NANO_FACTOR);
                    event->RemoveReleasedKeyItems(itemCtrl);
                }
                break;
            default:
                LOG_W("Illegal key action stage: %{public}d", ke.stage_);
                break;
        }
        InputManager::GetInstance()->SimulateInputEvent(event);
        if (ke.holdMs_ > 0) {
            this_thread::sleep_for(chrono::milliseconds(ke.holdMs_));
        }
    }

    void SysUiController::PutTextToClipboard(string_view text) const
    {
    }

    bool SysUiController::IsWorkable() const
    {
        return connected_;
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
            code = OHOS::MMI::KeyEvent::KEYCODE_A + (ch - 'a');
        } else if (ch >= 'A' && ch <= 'Z') {
            ctrlCode = OHOS::MMI::KeyEvent::KEYCODE_SHIFT_LEFT;
            code = OHOS::MMI::KeyEvent::KEYCODE_A + (ch - 'A');
        } else if (ch >= '0' && ch <= '9') {
            code = OHOS::MMI::KeyEvent::KEYCODE_0 + (ch - '0');
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
        fclose(fp);
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
            LOG_I("Success connect to AAMS");
            condition.notify_all();
        };
        if (g_monitorInstance_ == nullptr) {
            g_monitorInstance_ = make_shared<UiEventMonitor>();
        }
        g_monitorInstance_->SetOnAbilityConnectCallback(onConnectCallback);
        auto ability = AccessibilityUITestAbility::GetInstance();
        if (!ability->RegisterListener(g_monitorInstance_)) {
            LOG_E("Failed to register UiEventMonitor");
            return false;
        }
        LOG_I("Start connect to AAMS");
        if (!ability->Connect()) {
            LOG_E("Failed to connect to AAMS");
            return false;
        }
        const auto timeout = chrono::milliseconds(500);
        if (condition.wait_for(uLock, timeout) == cv_status::timeout) {
            LOG_E("Wait connection to AAMS timed out");
            return false;
        }
        LOG_I("Register to AAMS done");
        connected_ = true;
        return true;
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
            LOG_I("Success disconnect from AAMS");
            condition.notify_all();
        };
        g_monitorInstance_->SetOnAbilityDisConnectCallback(onDisConnectCallback);
        auto ability = AccessibilityUITestAbility::GetInstance();
        LOG_I("Start disconnect from AAMS");
        if (! ability->Disconnect()) {
            LOG_E("Failed to disconnect from AAMS");
            return;
        }
        const auto timeout = chrono::milliseconds(200);
        if (condition.wait_for(uLock, timeout) == cv_status::timeout) {
            LOG_E("Wait disconnection from AAMS timed out");
            return;
        }
    }
}