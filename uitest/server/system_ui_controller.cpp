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
#include <sys/mman.h>
#ifdef HIDUMPER_ENABLED
#include <iservice_registry.h>
#include <system_ability_load_callback_stub.h>
#include "idump_broker.h"
#include "system_ability_definition.h"
#endif
#include "pasteboard_client.h"
#include "accessibility_event_info.h"
#include "accessibility_ui_test_ability.h"
#include "ability_manager_client.h"
#include "display_manager.h"
#include "screen_manager.h"
#include "input_manager.h"
#include "png.h"
#include "wm_common.h"
#include "element_node_iterator_impl.h"
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
    using namespace OHOS::HiviewDFX;
    using namespace OHOS;

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

        void WaitScrollCompelete();

        void RegisterUiEventListener(shared_ptr<UiEventListener> listerner);

    private:
        function<void()> onConnectCallback_ = nullptr;
        function<void()> onDisConnectCallback_ = nullptr;
        atomic<uint64_t> lastEventMillis_ = 0;
        atomic<uint64_t> lastScrollBeginEventMillis_ = 0;
        atomic<bool> scrollCompelete_ = true;
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
    static shared_ptr<UiEventMonitor> g_monitorInstance_ = make_shared<UiEventMonitor>();

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
        auto eventType = eventInfo.GetEventType();
        LOG_W("OnEvent:0x%{public}x", eventType);
        auto capturedEvent = GetWatchedEvent(eventInfo);
        if (eventType == Accessibility::EventType::TYPE_VIEW_SCROLLED_START) {
            LOG_I("Capture scroll begin");
            scrollCompelete_.store(false);
            lastScrollBeginEventMillis_.store(GetCurrentMillisecond());
        }
        if (eventType == Accessibility::EventType::TYPE_VIEW_SCROLLED_EVENT) {
            LOG_I("Capture scroll end");
            scrollCompelete_.store(true);
        }
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

    void UiEventMonitor::WaitScrollCompelete()
    {
        if (scrollCompelete_.load()) {
            return;
        }
        const auto currentMs = GetCurrentMillisecond();
        if (lastScrollBeginEventMillis_.load() <= 0) {
            lastScrollBeginEventMillis_.store(currentMs);
        }
        const auto idleThresholdMs = 10000;
        if (currentMs - lastScrollBeginEventMillis_.load() >= idleThresholdMs) {
            LOG_E("wai for scrollEnd event timeout.");
            scrollCompelete_.store(true);
            return;
        }
        static constexpr auto sliceMs = 10;
        this_thread::sleep_for(chrono::milliseconds(sliceMs));
        return WaitScrollCompelete();
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

    static void InflateWindowInfo(AccessibilityWindowInfo& node, Window& info)
    {
        info.focused_ = node.IsFocused();
        info.actived_ = node.IsActive();
        info.decoratorEnabled_ = node.IsDecorEnable();
        // get bundle name by root node
        AccessibilityElementInfo element;
        LOG_D("Start Get Bundle Name by WindowId %{public}d", node.GetWindowId());
        if (AccessibilityUITestAbility::GetInstance()->GetRootByWindow(node, element) != RET_OK) {
            LOG_E("Failed Get Bundle Name by WindowId %{public}d", node.GetWindowId());
        } else {
            std::string app = element.GetBundleName();
            LOG_I("End Get Bundle Name by WindowId %{public}d, app is %{public}s", node.GetWindowId(), app.data());
            info.bundleName_ = app;
            const auto foreAbility = AAFwk::AbilityManagerClient::GetInstance()->GetTopAbility();
            info.abilityName_ = (app == foreAbility.GetBundleName()) ? foreAbility.GetAbilityName() : "";
            info.pagePath_ = (app == foreAbility.GetBundleName()) ? element.GetPagePath() : "";
        }
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
        g_monitorInstance_->WaitScrollCompelete();
        if (ability->GetWindows(windows) != RET_OK) {
            LOG_W("GetWindows from AccessibilityUITestAbility failed");
            return false;
        }
        sort(windows.begin(), windows.end(), [](auto &w1, auto &w2) -> bool {
            return w1.GetWindowLayer() > w2.GetWindowLayer();
        });
        return true;
    }

    void SysUiController::GetUiWindows(std::vector<Window> &out)
    {
        std::lock_guard<std::mutex> dumpLocker(dumpMtx); // disallow concurrent dumpUi
        if (!connected_ && !ConnectToSysAbility()) {
            LOG_W("Connect to AccessibilityUITestAbility failed");
            return;
        }
        vector<AccessibilityWindowInfo> windows;
        LOG_I("Get Window root info");
        if (!GetAamsWindowInfos(windows)) {
            return;
        }
        LOG_D("End Get Window root info");
        auto screenSize = GetDisplaySize();
        auto screenRect = Rect(0, screenSize.px_, 0, screenSize.py_);
        std::vector<Rect> overplays;
        // window wrapper
        for (auto &win : windows) {
            Rect winRectInScreen = GetVisibleRect(screenRect, win.GetRectInScreen());
            Rect visibleArea = winRectInScreen;
            if (!RectAlgorithm::ComputeMaxVisibleRegion(winRectInScreen, overplays, visibleArea)) {
                LOG_I("window is covered, windowId : %{public}d, layer is %{public}d", win.GetWindowId(),
                      win.GetWindowLayer());
                continue;
            }
            LOG_I("window is visible, windowId: %{public}d, active: %{public}d, focus: %{public}d, layer: %{public}d",
                win.GetWindowId(), win.IsActive(), win.IsFocused(), win.GetWindowLayer());
            Window winWrapper{win.GetWindowId()};
            InflateWindowInfo(win, winWrapper);
            winWrapper.bounds_ = winRectInScreen;
            for (const auto &overWin : overplays) {
                Rect intersectionRect{0, 0, 0, 0};
                if (RectAlgorithm::ComputeIntersection(winRectInScreen, overWin, intersectionRect)) {
                    winWrapper.invisibleBoundsVec_.emplace_back(overWin);
                }
            }
            overplays.emplace_back(winRectInScreen);
            out.emplace_back(std::move(winWrapper));
        }
    }

    bool SysUiController::GetWidgetsInWindow(const Window &winInfo, unique_ptr<ElementNodeIterator> &elementIterator)
    {
        std::lock_guard<std::mutex> dumpLocker(dumpMtx); // disallow concurrent dumpUi
        if (!connected_) {
            LOG_W("Connect to AccessibilityUITestAbility failed");
            return false;
        }
        std::vector<AccessibilityElementInfo> elementInfos;
        AccessibilityWindowInfo window;
        LOG_D("Get Window by WindowId %{public}d", winInfo.id_);
        if (AccessibilityUITestAbility::GetInstance()->GetWindow(winInfo.id_, window) != RET_OK) {
            LOG_E("GetWindowInfo failed, windowId: %{public}d", winInfo.id_);
            return false;
        }
        LOG_I("Start Get nodes from window by WindowId %{public}d", winInfo.id_);
        if (AccessibilityUITestAbility::GetInstance()->GetRootByWindowBatch(window, elementInfos) != RET_OK) {
            LOG_E("GetRootByWindowBatch failed, windowId: %{public}d", winInfo.id_);
            return false;
        } else {
            LOG_I("End Get nodes from window by WindowId %{public}d, node size is %{public}zu, appId: %{public}s",
                  winInfo.id_, elementInfos.size(), winInfo.bundleName_.data());
            elementIterator = std::make_unique<ElementNodeIteratorImpl>(elementInfos);
        }
        return true;
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
                    default:
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

    static void SetMousePointerItemAttr(const MouseEvent &event, PointerEvent::PointerItem &item)
    {
        item.SetPointerId(0);
        item.SetToolType(PointerEvent::TOOL_TYPE_MOUSE);
        item.SetDisplayX(event.point_.px_);
        item.SetDisplayY(event.point_.py_);
        item.SetPressed(false);
        item.SetDownTime(0);
    }

    void SysUiController::InjectMouseEvent(const MouseEvent &event) const
    {
        auto pointerEvent = PointerEvent::Create();
        PointerEvent::PointerItem item;
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
        pointerEvent->SetPointerId(0);
        pointerEvent->SetButtonId(event.button_);
        SetMousePointerItemAttr(event, item);
        constexpr double axialValue = 15;
        static bool flag = true;
        auto injectAxialValue = axialValue;
        switch (event.stage_) {
            case ActionStage::DOWN:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_BUTTON_DOWN);
                pointerEvent->SetButtonId(event.button_);
                pointerEvent->SetButtonPressed(event.button_);
                item.SetPressed(true);
                break;
            case ActionStage::MOVE:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_MOVE);
                break;
            case ActionStage::UP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_BUTTON_UP);
                pointerEvent->SetButtonId(event.button_);
                pointerEvent->SetButtonPressed(event.button_);
                break;
            case ActionStage::AXIS_UP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_BEGIN);
                pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, -axialValue);
                flag = false;
                break;
            case ActionStage::AXIS_DOWN:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_BEGIN);
                pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, axialValue);
                flag = true;
                break;
            case ActionStage::AXIS_STOP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_END);
                injectAxialValue = flag ? axialValue : -axialValue;
                pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, injectAxialValue);
                break;
            default:
                break;
        }
        pointerEvent->AddPointerItem(item);
        InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
        this_thread::sleep_for(chrono::milliseconds(event.holdMs_));
    }

    void SysUiController::InjectMouseEventSequence(const vector<MouseEvent> &events) const
    {
        for (auto &event : events) {
            auto keyEvents = event.keyEvents_;
            if (!keyEvents.empty() && keyEvents.front().stage_ == ActionStage::DOWN) {
                InjectKeyEventSequence(keyEvents);
                InjectMouseEvent(event);
            } else {
                InjectMouseEvent(event);
                InjectKeyEventSequence(keyEvents);
            }
        }
    }

    void SysUiController::InjectKeyEventSequence(const vector<KeyEvent> &events) const
    {
        static vector<int32_t> downKeys;
        for (auto &event : events) {
            if (event.code_ == KEYCODE_NONE) {
                continue;
            }
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

    void SysUiController::PutTextToClipboard(string_view text) const
    {
        auto pasteBoardMgr = MiscServices::PasteboardClient::GetInstance();
        pasteBoardMgr->Clear();
        auto pasteData = pasteBoardMgr->CreatePlainTextData(string(text));
        pasteBoardMgr->SetPasteData(*pasteData);
    }

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
        std::shared_ptr<condition_variable> condition = make_shared<condition_variable>();
        auto onConnectCallback = [condition]() {
            LOG_I("Success connect to AccessibilityUITestAbility");
            condition->notify_all();
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
        if (condition->wait_for(uLock, timeout) == cv_status::timeout) {
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
        bool isLocked = false;
        screenMgr.IsScreenRotationLocked(isLocked);
        if (isLocked) {
            screenMgr.SetScreenRotationLocked(false);
        }
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
        screenMgr.SetScreenRotationLocked(!enabled);
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

    class OnSaLoadCallback : public SystemAbilityLoadCallbackStub {
    public:
        explicit OnSaLoadCallback(mutex &mutex): mutex_(mutex) {};
        ~OnSaLoadCallback() {};
        void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject>& remoteObject) override
        {
            if (systemAbilityId == OHOS::DFX_HI_DUMPER_SERVICE_ABILITY_ID) {
                remoteObject_ = remoteObject;
                mutex_.unlock();
            }
        }
        void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
        {
            if (systemAbilityId == OHOS::DFX_HI_DUMPER_SERVICE_ABILITY_ID) {
                mutex_.unlock();
            }
        }

        sptr<IRemoteObject> GetSaObject()
        {
            return remoteObject_;
        }
        
    private:
        mutex &mutex_;
        sptr<IRemoteObject> remoteObject_ = nullptr;
    };

    void SysUiController::GetHidumperInfo(std::string windowId, char **buf, size_t &len)
    {
#ifdef HIDUMPER_ENABLED
        auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (sam == nullptr) {
            LOG_E("Get samgr failed");
            return;
        }
        auto remoteObject = sam->CheckSystemAbility(OHOS::DFX_HI_DUMPER_SERVICE_ABILITY_ID);
        if (remoteObject == nullptr) {
            mutex lock;
            lock.lock();
            sptr<OnSaLoadCallback> loadCallback = new OnSaLoadCallback(lock);
            int32_t result = sam->LoadSystemAbility(OHOS::DFX_HI_DUMPER_SERVICE_ABILITY_ID, loadCallback);
            if (result != ERR_OK) {
                LOG_E("Schedule LoadSystemAbility failed");
                return;
            }
            LOG_E("Schedule LoadSystemAbility succeed");
            lock.lock();
            remoteObject = loadCallback->GetSaObject();
            LOG_E("LoadSystemAbility callbacked, result = %{public}s", remoteObject == nullptr ? "FAIL" : "SUCCESS");
        }
        if (remoteObject == nullptr) {
            LOG_E("remoteObject is null");
            return;
        }
        // run dump command
        sptr<IDumpBroker> client = iface_cast<IDumpBroker>(remoteObject);
        auto fd = memfd_create("dummy_file", 2);
        ftruncate(fd, 0);
        vector<u16string> args;
        args.emplace_back(u"hidumper");
        args.emplace_back(u"-s");
        args.emplace_back(u"WindowManagerService");
        args.emplace_back(u"-a");
        auto winIdInUtf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t > {}.from_bytes(windowId);
        auto arg = u16string(u"-w ").append(winIdInUtf16).append(u" -default -lastpage");
        args.emplace_back(move(arg));
        client->Request(args, fd);
        auto size = lseek(fd, 0, SEEK_END);
        char *tempBuf = new char[size + 1];
        lseek(fd, 0, SEEK_SET);
        read(fd, tempBuf, size);
        *buf = tempBuf;
        len = size;
        close(fd);
#else
        *buf = nullptr;
        len = 0;
#endif
    }
} // namespace OHOS::uitest