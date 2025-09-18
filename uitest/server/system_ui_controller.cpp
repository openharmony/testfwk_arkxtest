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
#include "dump_broker_proxy.h"
#include "system_ability_definition.h"
#endif
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
#include "test_server_client.h"
#include "test_server_error_code.h"
#include "parameters.h"
#include "image_packer.h"

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

    const std::map<char, int32_t> SingleKeySymbalMap = {
        {' ', OHOS::MMI::KeyEvent::KEYCODE_SPACE},
        {'`', OHOS::MMI::KeyEvent::KEYCODE_GRAVE},
        {'[', OHOS::MMI::KeyEvent::KEYCODE_LEFT_BRACKET},
        {']', OHOS::MMI::KeyEvent::KEYCODE_RIGHT_BRACKET},
        {'\\', OHOS::MMI::KeyEvent::KEYCODE_BACKSLASH},
        {',', OHOS::MMI::KeyEvent::KEYCODE_COMMA},
        {';', OHOS::MMI::KeyEvent::KEYCODE_SEMICOLON},
        {'\'', OHOS::MMI::KeyEvent::KEYCODE_APOSTROPHE},
        {'/', OHOS::MMI::KeyEvent::KEYCODE_SLASH},
        {'*', OHOS::MMI::KeyEvent::KEYCODE_NUMPAD_MULTIPLY},
        {'-', OHOS::MMI::KeyEvent::KEYCODE_MINUS},
        {'.', OHOS::MMI::KeyEvent::KEYCODE_PERIOD},
        {'=', OHOS::MMI::KeyEvent::KEYCODE_EQUALS}
    };

    const std::map<char, int32_t> MultiKeySymbalMap = {
        {'~', OHOS::MMI::KeyEvent::KEYCODE_GRAVE},
        {'!', OHOS::MMI::KeyEvent::KEYCODE_1},
        {'@', OHOS::MMI::KeyEvent::KEYCODE_2},
        {'#', OHOS::MMI::KeyEvent::KEYCODE_3},
        {'$', OHOS::MMI::KeyEvent::KEYCODE_4},
        {'%', OHOS::MMI::KeyEvent::KEYCODE_5},
        {'^', OHOS::MMI::KeyEvent::KEYCODE_6},
        {'&', OHOS::MMI::KeyEvent::KEYCODE_7},
        {'(', OHOS::MMI::KeyEvent::KEYCODE_9},
        {')', OHOS::MMI::KeyEvent::KEYCODE_0},
        {'+', OHOS::MMI::KeyEvent::KEYCODE_EQUALS},
        {'_', OHOS::MMI::KeyEvent::KEYCODE_MINUS},
        {':', OHOS::MMI::KeyEvent::KEYCODE_SEMICOLON},
        {'"', OHOS::MMI::KeyEvent::KEYCODE_APOSTROPHE},
        {'<', OHOS::MMI::KeyEvent::KEYCODE_COMMA},
        {'>', OHOS::MMI::KeyEvent::KEYCODE_PERIOD},
        {'?', OHOS::MMI::KeyEvent::KEYCODE_SLASH},
        {'{', OHOS::MMI::KeyEvent::KEYCODE_LEFT_BRACKET},
        {'}', OHOS::MMI::KeyEvent::KEYCODE_RIGHT_BRACKET},
        {'|', OHOS::MMI::KeyEvent::KEYCODE_BACKSLASH}
    };

    static constexpr EventSpec WATCHED_EVENTS[] = {
        {"Toast", WindowsContentChangeTypes::CONTENT_CHANGE_TYPE_SUBTREE, "toastShow"},
        {"AlertDialog", WindowsContentChangeTypes::CONTENT_CHANGE_TYPE_SUBTREE, "dialogShow"}
    };

    static std::string GetWatchedEvent(const AccessibilityEventInfo &eventInfo)
    {
        auto eventCounts = sizeof(WATCHED_EVENTS) / sizeof(EventSpec);
        for (unsigned long index = 0; index < eventCounts; index++) {
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
    static const std::vector<uint32_t> EVENT_MASK = {
        EventType::TYPE_VIEW_TEXT_UPDATE_EVENT,
        EventType::TYPE_PAGE_STATE_UPDATE,
        EventType::TYPE_PAGE_CONTENT_UPDATE,
        EventType::TYPE_VIEW_SCROLLED_EVENT,
        EventType::TYPE_WINDOW_UPDATE,
        EventType::TYPE_PAGE_OPEN
    };

    void UiEventMonitor::RegisterUiEventListener(std::shared_ptr<UiEventListener> listerner)
    {
        listeners_.emplace_back(listerner);
    }

    void UiEventMonitor::OnAccessibilityEvent(const AccessibilityEventInfo &eventInfo)
    {
        auto eventType = eventInfo.GetEventType();
        LOG_D("OnEvent:0x%{public}x", eventType);
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
        if (std::find(EVENT_MASK.begin(), EVENT_MASK.end(), eventInfo.GetEventType()) != EVENT_MASK.end()) {
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
        auto currentMs = GetCurrentMillisecond();
        if (lastScrollBeginEventMillis_.load() <= 0) {
            lastScrollBeginEventMillis_.store(currentMs);
        }
        const auto idleThresholdMs = 10000;
        static constexpr auto sliceMs = 10;
        while (currentMs - lastScrollBeginEventMillis_.load() < idleThresholdMs) {
            if (scrollCompelete_.load()) {
                return;
            }
            this_thread::sleep_for(chrono::milliseconds(sliceMs));
            currentMs = GetCurrentMillisecond();
        }
        LOG_E("wait for scrollEnd event timeout.");
        scrollCompelete_.store(true);
        return;
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

    bool SysUiController::Initialize(ApiCallErr &error)
    {
        return this->ConnectToSysAbility(error);
    }

    static Rect GetFoldArea()
    {
        auto foldCreaseRegion = DisplayManager::GetInstance().GetCurrentFoldCreaseRegion();
        auto areas = foldCreaseRegion->GetCreaseRects();
        if (areas.size() == 1) {
            auto foldArea = *areas.begin();
            LOG_D("foldArea, left: %{public}d, top: %{public}d, width: %{public}d, height: %{public}d",
                  foldArea.posX_, foldArea.posY_, foldArea.width_, foldArea.height_);
            return Rect(foldArea.posX_, foldArea.posX_ + foldArea.width_,
                        foldArea.posY_, foldArea.posY_ + foldArea.height_);
        } else {
            LOG_E("Invalid display info.");
            return Rect(0, 0, 0, 0);
        }
    }

    static Rect GetVisibleRect(Rect screenBounds, AccessibilityWindowInfo &node)
    {
        auto nodeBounds = node.GetRectInScreen();
        auto leftX = nodeBounds.GetLeftTopXScreenPostion();
        auto topY = nodeBounds.GetLeftTopYScreenPostion();
        auto rightX = nodeBounds.GetRightBottomXScreenPostion();
        auto bottomY = nodeBounds.GetRightBottomYScreenPostion();
        float scaleX = node.GetScaleX();
        float scaleY = node.GetScaleY();
        LOG_D("Get window %{public}d, leftX: %{public}d, topY: %{public}d, rightX: %{public}d, bottomY: %{public}d,"
              "scaleX: %{public}f, scaleY: %{public}f",
              node.GetWindowId(), leftX, topY, rightX, bottomY, scaleX, scaleY);
        scaleX = (fabs(scaleX) < 1e-6) ? 1.0 : scaleX;
        scaleY = (fabs(scaleY) < 1e-6) ? 1.0 : scaleY;
        rightX = leftX + (rightX - leftX) * scaleX;
        bottomY = topY + (bottomY - topY) * scaleY;
        if (node.GetDisplayId() == VIRTUAL_DISPLAY_ID) {
            auto foldArea = GetFoldArea();
            topY += foldArea.bottom_;
            bottomY += foldArea.bottom_;
        }
        Rect newBounds((leftX < screenBounds.left_) ? screenBounds.left_ : leftX,
                       (rightX > screenBounds.right_) ? screenBounds.right_ : rightX,
                       (topY < screenBounds.top_) ? screenBounds.top_ : topY,
                       (bottomY > screenBounds.bottom_) ? screenBounds.bottom_ : bottomY);
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
        auto touchAreas = node.GetTouchHotAreas();
        for (auto area : touchAreas) {
            Rect rect { info.bounds_.left_ + area.GetLeftTopXScreenPostion(),
                        info.bounds_.left_ + area.GetRightBottomXScreenPostion(),
                        info.bounds_.top_  + area.GetLeftTopYScreenPostion(),
                        info.bounds_.top_  + area.GetRightBottomYScreenPostion() };
            info.touchHotAreas_.push_back(rect);
        }
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

    static bool GetAamsWindowInfos(vector<AccessibilityWindowInfo> &windows, int32_t displayId)
    {
        LOG_D("Get Window root info in display %{public}d", displayId);
        auto ability = AccessibilityUITestAbility::GetInstance();
        g_monitorInstance_->WaitScrollCompelete();
        auto ret = ability->GetWindows(displayId, windows);
        if (ret != RET_OK) {
            LOG_W("GetWindows in display %{public}d from AccessibilityUITestAbility failed, ret: %{public}d",
                displayId, ret);
            return false;
        }
        auto foldStatus = DisplayManager::GetInstance().GetFoldStatus();
        if (foldStatus == Rosen::FoldStatus::HALF_FOLD && displayId == 0) {
            vector<AccessibilityWindowInfo> windowsInVirtual;
            auto ret1 = ability->GetWindows(VIRTUAL_DISPLAY_ID, windowsInVirtual);
            LOG_D("GetWindows in display 999 from AccessibilityUITestAbility, ret: %{public}d", ret1);
            for (auto &win : windowsInVirtual) {
                windows.emplace_back(win);
            }
        }
        if (windows.empty()) {
            LOG_E("Get Windows in display %{public}d failed", displayId);
            return false;
        }
        sort(windows.begin(), windows.end(), [](auto &w1, auto &w2) -> bool {
            return w1.GetWindowLayer() > w2.GetWindowLayer();
        });
        LOG_D("End Get Window root info");
        return true;
    }

    static void UpdateWindowAttrs(Window &win, std::vector<Rect> &overplays)
    {
        for (const auto &overWin : overplays) {
            Rect intersectionRect{0, 0, 0, 0};
            if (RectAlgorithm::ComputeIntersection(win.bounds_, overWin, intersectionRect)) {
                win.invisibleBoundsVec_.emplace_back(overWin);
            }
        }
        RectAlgorithm::ComputeMaxVisibleRegion(win.bounds_, overplays, win.visibleBounds_);
        if (win.touchHotAreas_.empty()) {
            overplays.emplace_back(win.bounds_);
        } else {
            std::string touchAreaInfo;
            for (auto rect : win.touchHotAreas_) {
                touchAreaInfo += rect.Describe() + " ";
                overplays.emplace_back(rect);
            }
            LOG_I("window %{public}d touchArea: %{public}s", win.id_, touchAreaInfo.c_str());
        }
        if (win.displayId_ == VIRTUAL_DISPLAY_ID) {
            win.offset_ = Point(0, GetFoldArea().bottom_);
        }
    }

    void SysUiController::GetUiWindows(std::map<int32_t, vector<Window>> &out, int32_t targetDisplay)
    {
        std::lock_guard<std::mutex> dumpLocker(dumpMtx); // disallow concurrent dumpUi
        ApiCallErr error = ApiCallErr(NO_ERROR);
        if (!connected_ && !ConnectToSysAbility(error)) {
            LOG_E("%{public}s", error.message_.c_str());
            return;
        }
        DisplayManager &dpm = DisplayManager::GetInstance();
        auto displayIds = dpm.GetAllDisplayIds();
        for (auto displayId : displayIds) {
            if ((targetDisplay != -1 && targetDisplay != static_cast<int32_t>(displayId)) ||
                displayId == VIRTUAL_DISPLAY_ID) {
                continue;
            }
            vector<AccessibilityWindowInfo> windows;
            if (!GetAamsWindowInfos(windows, displayId)) {
                continue;
            }
            auto screenSize = GetDisplaySize(displayId);
            auto screenRect = Rect(0, screenSize.px_, 0, screenSize.py_);
            std::vector<Window> winInfos;
            std::vector<Rect> overplays;
            // window wrapper
            for (auto &win : windows) {
                Rect winRectInScreen = GetVisibleRect(screenRect, win);
                Rect visibleArea = winRectInScreen;
                if (!RectAlgorithm::ComputeMaxVisibleRegion(winRectInScreen, overplays, visibleArea)) {
                    LOG_I("window is covered, windowId : %{public}d, layer is %{public}d", win.GetWindowId(),
                          win.GetWindowLayer());
                    continue;
                }
                LOG_I("window is visible, windowId: %{public}d, active: %{public}d, focus: %{public}d,"
                    "layer: %{public}d, displayId: %{public}" PRIu64 "",
                    win.GetWindowId(), win.IsActive(), win.IsFocused(), win.GetWindowLayer(), win.GetDisplayId());
                Window winWrapper{win.GetWindowId()};
                winWrapper.bounds_ = winRectInScreen;
                InflateWindowInfo(win, winWrapper);
                winWrapper.displayId_ = win.GetDisplayId();
                UpdateWindowAttrs(winWrapper, overplays);
                winWrapper.displayId_ = displayId;
                winInfos.emplace_back(move(winWrapper));
            }
            out.insert(make_pair(displayId, move(winInfos)));
        }
    }

    bool SysUiController::GetWidgetsInWindow(const Window &winInfo, unique_ptr<ElementNodeIterator> &elementIterator,
        AamsWorkMode mode)
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
        LOG_D("Start Get nodes from window by WindowId %{public}d", winInfo.id_);
        auto ret = RET_ERR_FAILED;
        auto ability = AccessibilityUITestAbility::GetInstance();
        if (mode == AamsWorkMode::FASTGETNODE) {
            LOG_D("GetRootByWindowBatch in reduced mode");
            ret = ability->GetRootByWindowBatch(window, elementInfos, false, true);
        } else {
            ret = ability->GetRootByWindowBatch(window, elementInfos);
        }
        if (ret != RET_OK) {
            LOG_E("GetRootByWindowBatch failed, windowId: %{public}d", winInfo.id_);
            return false;
        } else {
            LOG_I("End Get nodes from window by WindowId %{public}d, node size is %{public}zu, appId: %{public}s",
                  winInfo.id_, elementInfos.size(), winInfo.bundleName_.data());
            elementIterator = std::make_unique<ElementNodeIteratorImpl>(elementInfos);
        }
        return true;
    }

    int32_t SysUiController::GetValidDisplayId(int32_t id) const
    {
        if (id == UNASSIGNED) {
            id = DisplayManager::GetInstance().GetDefaultDisplayId();
        }
        return id;
    }

    bool SysUiController::CheckDisplayExist(int32_t displayId) const
    {
        if (displayId == UNASSIGNED) {
            return true;
        }
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        return displayMgr.GetDisplayById(displayId) != nullptr;
    }

    static void SetItemByType(PointerEvent::PointerItem &pinterItem, const PointerMatrix &events,
        uint32_t finger, bool pressed, const Point &point)
    {
        switch (events.GetToolType()) {
            case TouchToolType::FINGER:
                pinterItem.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
                break;
            case TouchToolType::PEN:
                pinterItem.SetToolType(PointerEvent::TOOL_TYPE_PEN);
                pinterItem.SetPressure(events.GetTouchPressure());
                break;
            case TouchToolType::KNUCKLE:
                pinterItem.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
                break;
            default:
                return;
        }
        pinterItem.SetPointerId(finger);
        pinterItem.SetOriginPointerId(finger);
        pinterItem.SetDisplayX(point.px_);
        pinterItem.SetDisplayY(point.py_);
        pinterItem.SetRawDisplayX(point.px_);
        pinterItem.SetRawDisplayY(point.py_);
        pinterItem.SetPressed(pressed);
        LOG_I("Add touchItem, finger:%{public}d, pressed:%{public}d, location:%{public}d, %{public}d",
            finger, pressed, point.px_, point.py_);
    }

    static void AddPointerItems(PointerEvent &event, const vector<pair<bool, Point>> &fingerStatus,
        const PointerMatrix &events, uint32_t currentFinger, uint32_t currentStep)
    {
        if (events.IsSyncInject()) {
            for (auto finger = 0; finger < events.GetFingers(); finger++) {
                PointerEvent::PointerItem pinterItem;
                SetItemByType(pinterItem, events, finger, fingerStatus[finger].first,
                    events.At(finger, currentStep).point_);
                event.UpdatePointerItem(finger, pinterItem);
            }
        } else {
            PointerEvent::PointerItem pinterItem;
            SetItemByType(pinterItem, events, currentFinger, fingerStatus[currentFinger].first,
                fingerStatus[currentFinger].second);
            event.UpdatePointerItem(currentFinger, pinterItem);
            // update pinterItem of other fingers which in pressed state.
            for (uint32_t index = 0; index < fingerStatus.size(); index++) {
                if (index == currentFinger) {
                    continue;
                }
                if (fingerStatus[index].first) {
                    PointerEvent::PointerItem pinterItemForOther;
                    SetItemByType(pinterItemForOther, events, index, true, fingerStatus[index].second);
                    event.UpdatePointerItem(index, pinterItemForOther);
                }
            }
        }
    }

    void SysUiController::InjectTouchEventSequence(const PointerMatrix &events) const
    {
        // fingerStatus stores the press status and coordinates of each finger.
        vector<pair<bool, Point>> fingerStatus(events.GetFingers(), make_pair(false, Point(0,0)));
        for (uint32_t step = 0; step < events.GetSteps(); step++) {
            for (uint32_t finger = 0; finger < events.GetFingers(); finger++) {
                auto pointerEvent = PointerEvent::Create();
                if (pointerEvent == nullptr) {
                    LOG_E("Creat PointerEvent failed.");
                    return;
                }
                bool isPressed = (events.At(finger, step).stage_ == ActionStage::DOWN) ||
                                 (events.At(finger, step).stage_ == ActionStage::MOVE);
                fingerStatus[finger] = make_pair(isPressed, events.At(finger, step).point_);
                pointerEvent->SetPointerId(finger);
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
                    case ActionStage::PROXIMITY_IN:
                        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PROXIMITY_IN);
                        break;
                    case ActionStage::PROXIMITY_OUT:
                        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PROXIMITY_OUT);
                        break;
                    default:
                        return;
                }
                AddPointerItems(*pointerEvent, fingerStatus, events, finger, step);
                pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
                auto displayId = GetValidDisplayId(events.At(finger, step).point_.displayId_);
                pointerEvent->SetTargetDisplayId(displayId);
                InputManager::GetInstance()->SimulateInputEvent(pointerEvent, false);
                LOG_D("Inject touchEvent to display : %{public}d", displayId);
                if (events.At(finger, step).holdMs_ > 0) {
                    this_thread::sleep_for(chrono::milliseconds(events.At(finger, step).holdMs_));
                }
            }
        }
    }

    static void SetMousePointerItemAttr(const MouseEvent &event, PointerEvent::PointerItem &item)
    {
        item.SetPointerId(0);
        item.SetOriginPointerId(0);
        item.SetToolType(PointerEvent::TOOL_TYPE_MOUSE);
        item.SetDisplayX(event.point_.px_);
        item.SetDisplayY(event.point_.py_);
        item.SetRawDisplayX(event.point_.px_);
        item.SetRawDisplayY(event.point_.py_);
        item.SetPressed(false);
        item.SetDownTime(0);
        LOG_I("Inject mouseEvent, pressed:%{public}d, location:%{public}d, %{public}d",
            event.stage_ == ActionStage::DOWN, event.point_.px_, event.point_.py_);
    }

    static void SetMousePointerEventAttr(shared_ptr<PointerEvent> pointerEvent, const MouseEvent &event)
    {
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
        pointerEvent->SetPointerId(0);
        if (event.button_ != MouseButton::BUTTON_NONE) {
            pointerEvent->SetButtonId(event.button_);
            if ((event.stage_ == ActionStage::DOWN || event.stage_ == ActionStage::MOVE)) {
                pointerEvent->SetButtonPressed(event.button_);
            } else if (event.stage_ == ActionStage::UP) {
                pointerEvent->DeleteReleaseButton(event.button_);
            }
        }
    }

    void SysUiController::InjectMouseEvent(const MouseEvent &event) const
    {
        auto pointerEvent = PointerEvent::Create();
        if (pointerEvent == nullptr) {
            return;
        }
        PointerEvent::PointerItem item;
        SetMousePointerEventAttr(pointerEvent, event);
        constexpr double axialValue = 15;
        static double injectAxialValue = 0;
        switch (event.stage_) {
            case ActionStage::DOWN:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_BUTTON_DOWN);
                item.SetPressed(true);
                break;
            case ActionStage::MOVE:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_MOVE);
                break;
            case ActionStage::UP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_BUTTON_UP);
                break;
            case ActionStage::AXIS_UP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_BEGIN);
                pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, -axialValue);
                injectAxialValue = injectAxialValue - axialValue;
                break;
            case ActionStage::AXIS_DOWN:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_BEGIN);
                pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, axialValue);
                injectAxialValue = injectAxialValue + axialValue;
                break;
            case ActionStage::AXIS_STOP:
                pointerEvent->SetPointerAction(OHOS::MMI::PointerEvent::POINTER_ACTION_AXIS_END);
                pointerEvent->SetAxisValue(OHOS::MMI::PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, injectAxialValue);
                injectAxialValue = 0;
                break;
            default:
                return;
        }
        SetMousePointerItemAttr(event, item);
        pointerEvent->AddPointerItem(item);
        auto displayId = GetValidDisplayId(event.point_.displayId_);
        pointerEvent->SetTargetDisplayId(displayId);
        if (!downKeys_.empty()) {
            pointerEvent->SetPressedKeys(downKeys_);
        }
        InputManager::GetInstance()->SimulateInputEvent(pointerEvent, false);
        LOG_I("Inject mouseEvent to display : %{public}d", displayId);
        this_thread::sleep_for(chrono::milliseconds(event.holdMs_));
    }

    void SysUiController::InjectMouseEventSequence(const vector<MouseEvent> &events) const
    {
        for (auto &event : events) {
            auto keyEvents = event.keyEvents_;
            if (!keyEvents.empty() && keyEvents.front().stage_ == ActionStage::DOWN) {
                InjectKeyEventSequence(keyEvents, event.point_.displayId_);
                InjectMouseEvent(event);
            } else {
                InjectMouseEvent(event);
                InjectKeyEventSequence(keyEvents, event.point_.displayId_);
            }
        }
    }

    void SysUiController::InjectKeyEventSequence(const vector<KeyEvent> &events, int32_t displayId) const
    {
        displayId = GetValidDisplayId(displayId);
        for (auto &event : events) {
            if (event.code_ == KEYCODE_NONE) {
                continue;
            }
            auto keyEvent = OHOS::MMI::KeyEvent::Create();
            if (keyEvent == nullptr) {
                LOG_E("Creat KeyEvent failed.");
                return;
            }
            if (event.stage_ == ActionStage::UP) {
                auto iter = std::find(downKeys_.begin(), downKeys_.end(), event.code_);
                if (iter == downKeys_.end()) {
                    LOG_W("Cannot release a not-pressed key: %{public}d", event.code_);
                    continue;
                }
                downKeys_.erase(iter);
                keyEvent->SetKeyCode(event.code_);
                keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_UP);
                OHOS::MMI::KeyEvent::KeyItem keyItem;
                keyItem.SetKeyCode(event.code_);
                keyItem.SetPressed(false);
                keyEvent->AddKeyItem(keyItem);
                keyEvent->SetTargetDisplayId(displayId);
                InputManager::GetInstance()->SimulateInputEvent(keyEvent);
                LOG_I("Inject keyEvent up, keycode:%{public}d", event.code_);
            } else {
                downKeys_.push_back(event.code_);
                for (auto downKey : downKeys_) {
                    keyEvent->SetKeyCode(downKey);
                    keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
                    OHOS::MMI::KeyEvent::KeyItem keyItem;
                    keyItem.SetKeyCode(downKey);
                    keyItem.SetPressed(true);
                    keyEvent->AddKeyItem(keyItem);
                }
                keyEvent->SetTargetDisplayId(displayId);
                InputManager::GetInstance()->SimulateInputEvent(keyEvent);
                LOG_I("Inject keyEvent down, keycode:%{public}d", event.code_);
                if (event.holdMs_ > 0) {
                    this_thread::sleep_for(chrono::milliseconds(event.holdMs_));
                }
            }
        }
        // check not released keys
        for (auto downKey : downKeys_) {
            LOG_W("Key event sequence injections done with not-released key: %{public}d", downKey);
        }
    }

    bool SysUiController::IsTouchPadExist() const
    {
        std::vector<int32_t> inputDeviceIdList;
        auto getDeviceIdsCallback = [&inputDeviceIdList](std::vector<int32_t>& deviceIds) {
            inputDeviceIdList = deviceIds;
        };
        int32_t ret1 = InputManager::GetInstance()->GetDeviceIds(getDeviceIdsCallback);
        if (ret1 != RET_OK) {
            LOG_E("Get device ids failed");
            return false;
        }
        const int32_t touchPadTag = 1 << 3;
        for (auto inputDeviceId : inputDeviceIdList) {
            std::shared_ptr<MMI::InputDevice> inputDevice;
            auto getDeviceCallback = [&inputDevice](std::shared_ptr<MMI::InputDevice> device) {
                inputDevice = device;
            };
            int32_t ret2 = MMI::InputManager::GetInstance()->GetDevice(inputDeviceId, getDeviceCallback);
            if (ret2 != RET_OK || inputDevice == nullptr) {
                LOG_E("Get device failed");
                continue;
            }
            if (inputDevice->GetType() & touchPadTag) {
                return true;
            }
        }
        return false;
    }

    void SysUiController::InjectTouchPadEventSequence(const vector<TouchPadEvent>& events) const
    {
        vector<pair<bool, Point>> fingerStatus(1, make_pair(false, Point(0, 0)));
        for (auto &event : events) {
            auto pointerEvent = PointerEvent::Create();
            if (pointerEvent == nullptr) {
                LOG_E("Creat PointerEvent failed.");
                return;
            }
            pointerEvent->SetPointerId(0);
            switch (event.stage) {
                case ActionStage::DOWN:
                    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_SWIPE_BEGIN);
                    break;
                case ActionStage::MOVE:
                    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_SWIPE_UPDATE);
                    break;
                case ActionStage::UP:
                    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_SWIPE_END);
                    break;
                default:
                    break;
            }
            fingerStatus[0] = make_pair(false, event.point);
            PointerMatrix pointer;
            AddPointerItems(*pointerEvent, fingerStatus, pointer, 0, 0);
            pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
            pointerEvent->SetFingerCount(event.fingerCount);
            auto displayId = GetValidDisplayId(event.point.displayId_);
            pointerEvent->SetTargetDisplayId(displayId);
            InputManager::GetInstance()->SimulateInputEvent(pointerEvent, false);
            LOG_D("Inject touchEvent to display %{public}d", displayId);
            if (event.holdMs > 0) {
                this_thread::sleep_for(chrono::milliseconds(event.holdMs));
            }
        }
    }

    void SysUiController::PutTextToClipboard(string_view text, ApiCallErr &error) const
    {
        auto ret = OHOS::testserver::TestServerClient::GetInstance().SetPasteData(string(text));
        if (ret != OHOS::testserver::TEST_SERVER_OK) {
            LOG_E("Set pasteBoard data failed.");
            error = ApiCallErr(ERR_INTERNAL, "Set pasteBoard data failed.");
            if (ret != OHOS::testserver::TEST_SERVER_SET_PASTE_DATA_FAILED) {
                error.code_ = ERR_NO_SYSTEM_CAPABILITY;
            }
            return;
        }
        static constexpr auto sliceMs = 500;
        this_thread::sleep_for(chrono::milliseconds(sliceMs));
    }

    void SysUiController::ChangeWindowMode(int32_t windowId, WindowMode mode) const
    {
        switch (mode) {
            case WindowMode::FULLSCREEN:
                OHOS::testserver::TestServerClient::GetInstance().ChangeWindowMode(windowId,
                    static_cast<uint32_t>(OHOS::Rosen::WindowMode::WINDOW_MODE_FULLSCREEN));
                break;
            case WindowMode::SPLIT_PRIMARY:
                OHOS::testserver::TestServerClient::GetInstance().ChangeWindowMode(windowId,
                    static_cast<uint32_t>(OHOS::Rosen::WindowMode::WINDOW_MODE_SPLIT_PRIMARY));
                break;
            case WindowMode::FLOATING:
                OHOS::testserver::TestServerClient::GetInstance().ChangeWindowMode(windowId,
                    static_cast<uint32_t>(OHOS::Rosen::WindowMode::WINDOW_MODE_FLOATING));
                break;
            case WindowMode::MINIMIZED:
                OHOS::testserver::TestServerClient::GetInstance().MinimizeWindow(windowId);
                break;
            case WindowMode::CLOSED:
                OHOS::testserver::TestServerClient::GetInstance().TerminateWindow(windowId);
                break;
            default:
                break;
        }
        static constexpr auto sliceMs = 500;
        this_thread::sleep_for(chrono::milliseconds(sliceMs));
    }

    bool SysUiController::IsWorkable() const
    {
        return connected_;
    }

    bool SysUiController::IsWearable() const
    {
        bool isWearable = false;
#ifdef ARKXTEST_WATCH_FEATURE_ENABLE
        isWearable = true;
#endif
        return isWearable;
    }

    bool SysUiController::IsAdjustWindowModeEnable() const
    {
        bool IsAdjustWindowModeEnable = false;
#ifdef ARKXTEST_ADJUST_WINDOWMODE_ENABLE
        IsAdjustWindowModeEnable = true;
#endif
        return IsAdjustWindowModeEnable;
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
        } else if (SingleKeySymbalMap.find(ch) != SingleKeySymbalMap.end()) {
            code = SingleKeySymbalMap.find(ch)->second;
        } else if (MultiKeySymbalMap.find(ch) != MultiKeySymbalMap.end()) {
            ctrlCode = OHOS::MMI::KeyEvent::KEYCODE_SHIFT_LEFT;
            code = MultiKeySymbalMap.find(ch)->second;
        } else {
            return false;
        }
        return true;
    }

    bool SysUiController::TakeScreenCap(int32_t fd, std::stringstream &errReceiver, int32_t displayId, Rect rect) const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        displayId = GetValidDisplayId(displayId);
        // get PixelMap from DisplayManager API
        shared_ptr<PixelMap> pixelMap;
        if (rect.GetWidth() == 0) {
            pixelMap = displayMgr.GetScreenshot(displayId);
        } else {
            Media::Rect region = {.left = rect.left_, .top = rect.top_,
                .width = rect.right_ - rect.left_, .height = rect.bottom_ - rect.top_};
            Media::Size size = {.width = rect.right_ - rect.left_, .height = rect.bottom_ - rect.top_};
            pixelMap = displayMgr.GetScreenshot(displayId, region, size, 0);
        }
        if (pixelMap == nullptr) {
            errReceiver << "Failed to get display pixelMap";
            return false;
        }
        int64_t packedSize = 0L;
        auto pixelSize = static_cast<uint32_t>(pixelMap->GetByteCount());
        LOG_D("PixelSize: %{public}d", pixelSize);
        auto buffer = new (std::nothrow) uint8_t[pixelSize];
        Media::ImagePacker imagePacker;
        Media::PackOption packOption;
        packOption.format = "image/png";
        imagePacker.StartPacking(buffer, pixelSize, packOption);
        imagePacker.AddImage(*pixelMap);
        uint32_t packResult = imagePacker.FinalizePacking(packedSize);
        LOG_D("Packed pixelMap, packResult: %{public}d", packResult);
        LOG_D("Packed pixelMap, packedSize: %{public}" PRId64, packedSize);
        if (packResult != NO_ERROR || buffer == nullptr) {
            delete[] buffer;
            return false;
        }
        int ret = write(fd, buffer, packedSize);
        if (ret == -1) {
            int err = errno;
            LOG_E("write failed: %{public}d", err);
            LOG_E("write failed reason: %{public}s", strerror(err));
            return false;
        }
        delete[] buffer;
        return true;
    }

    bool SysUiController::ConnectToSysAbility(ApiCallErr &error)
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
            error = ApiCallErr(ERR_INITIALIZE_FAILED, "Can not connect to AAMS, REGISTER_LISTENER_FAILED");
            return false;
        }
        auto ret = ability->Connect();
        LOG_I("Connect to AAMS, result: %{public}d", ret);
        if (ret != RET_OK) {
            error = ApiCallErr(ERR_INITIALIZE_FAILED, "Can not connect to AAMS");
            if (ret == RET_ERR_CONNECTION_EXIST) {
                error.message_ += ", RET_ERR_CONNECTION_EXIST";
            } else {
                error.message_ += ", RET_ERR_AAMS";
            }
            return false;
        }
        const auto timeout = chrono::milliseconds(7000);
        if (condition->wait_for(uLock, timeout) == cv_status::timeout) {
            LOG_E("Wait connection to AccessibilityUITestAbility timed out");
            error = ApiCallErr(ERR_INITIALIZE_FAILED, "Can not connect to AAMS, RET_TIMEOUT");
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

    DisplayRotation SysUiController::GetDisplayRotation(int32_t displayId) const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        displayId = GetValidDisplayId(displayId);
        auto display = displayMgr.GetDisplayById(displayId);
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
        screenMgr.SetScreenRotationLocked(!enabled);
    }

    Point SysUiController::GetDisplaySize(int32_t displayId) const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        displayId = GetValidDisplayId(displayId);
        auto display = displayMgr.GetDisplayById(displayId);
        if (display == nullptr) {
            LOG_E("DisplayManager init fail");
            return {0, 0, displayId};
        }
        auto width = display->GetWidth();
        auto height = display->GetHeight();
        LOG_D("GetDisplaysize in display %{public}d, width: %{public}d, height: %{public}d", displayId, width, height);
        auto foldStatus = displayMgr.GetFoldStatus();
        if (foldStatus == Rosen::FoldStatus::HALF_FOLD && displayId == 0) {
            auto virtualDisplay = displayMgr.GetDisplayById(VIRTUAL_DISPLAY_ID);
            if (virtualDisplay != nullptr) {
                auto virtualwidth = virtualDisplay->GetWidth();
                auto virtualheight = virtualDisplay->GetHeight();
                auto foldArea = GetFoldArea();
                auto foldAreaWidth = foldArea.right_ - foldArea.left_;
                auto foldAreaHeight = foldArea.bottom_ - foldArea.top_;
                LOG_D("GetDisplaysize in virtual display, width: %{public}d, height: %{public}d",
                    virtualwidth, virtualheight);
                LOG_D("GetDisplaysize in foldArea, width: %{public}d, height: %{public}d", foldAreaWidth,
                    foldAreaHeight);
                height = height + virtualheight + foldAreaHeight;
            }
        }
        Point result(width, height, displayId);
        return result;
    }

    Point SysUiController::GetDisplayDensity(int32_t displayId) const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        displayId = GetValidDisplayId(displayId);
        auto display = displayMgr.GetDisplayById(displayId);
        if (display == nullptr) {
            LOG_E("DisplayManager init fail");
            return {0, 0, displayId};
        }
        auto rate = display->GetVirtualPixelRatio();
        Point displaySize = GetDisplaySize(displayId);
        Point result(displaySize.px_ * rate, displaySize.py_ * rate, displayId);
        return result;
    }

    bool SysUiController::IsScreenOn() const
    {
        DisplayManager &displayMgr = DisplayManager::GetInstance();
        auto displayId = displayMgr.GetDefaultDisplayId();
        auto state = displayMgr.GetDisplayState(displayId);
        return (state != DisplayState::OFF);
    }

    void SysUiController::CloseAamsEvent() const
    {
        AccessibilityUITestAbility::GetInstance()->ConfigureEvents({ Accessibility::EventType::TYPE_VIEW_INVALID });
    }

    void SysUiController::OpenAamsEvent() const
    {
        AccessibilityUITestAbility::GetInstance()->ConfigureEvents(EVENT_MASK);
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

    static void CreateHidumperCmd(const std::string &windowId, vector<u16string> &result)
    {
        result.emplace_back(u"hidumper");
        result.emplace_back(u"-s");
        result.emplace_back(u"WindowManagerService");
        result.emplace_back(u"-a");
        auto winIdInUtf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(windowId);
        auto arg = u16string(u"-w ").append(winIdInUtf16).append(u" -default -lastpage");
        result.emplace_back(move(arg));
    }

    void SysUiController::GetHidumperInfo(std::string windowId, char **buf, size_t &len)
    {
#ifdef HIDUMPER_ENABLED
        auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        // wati SA start
        constexpr auto delayMs = 2000;
        this_thread::sleep_for(chrono::milliseconds(delayMs));
        if (sam == nullptr) {
            LOG_E("Get samgr failed");
            return;
        }
        auto remoteObject = sam->CheckSystemAbility(OHOS::DFX_HI_DUMPER_SERVICE_ABILITY_ID);
        if (remoteObject == nullptr) {
            mutex lock;
            lock.lock();
            sptr<OnSaLoadCallback> loadCallback = new OnSaLoadCallback(lock);
            if (sam->LoadSystemAbility(OHOS::DFX_HI_DUMPER_SERVICE_ABILITY_ID, loadCallback) != ERR_OK) {
                LOG_E("Schedule LoadSystemAbility failed");
                lock.unlock();
                return;
            }
            LOG_E("Schedule LoadSystemAbility succeed");
            lock.unlock();
            remoteObject = loadCallback->GetSaObject();
            LOG_E("LoadSystemAbility callbacked, result = %{public}s", remoteObject == nullptr ? "FAIL" : "SUCCESS");
        }
        if (remoteObject == nullptr) {
            LOG_E("remoteObject is null");
            return;
        }
        // run dump command
        sptr<IDumpBroker> client = iface_cast<IDumpBroker>(remoteObject);
        if (client == nullptr) {
            LOG_E("IDumpBroker converts failed");
            return;
        }
        auto fd = memfd_create("dummy_file", 2);
        ftruncate(fd, 0);
        vector<u16string> args;
        CreateHidumperCmd(windowId, args);
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

    bool SysUiController::IsKnuckleSnapshotEnable() const
    {
        string uri = "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_100?Proxy=true";
        string knuckleSnapshotKey = "fingersense_smartshot_enabled";
        auto value = OHOS::testserver::TestServerClient::GetInstance().GetValueFromDataShare(uri, knuckleSnapshotKey);
        LOG_D("key = %{public}s, value = %{public}s", knuckleSnapshotKey.c_str(), value.c_str());
        return atoi(value.c_str()) != 0;
    }

    bool SysUiController::IsKnuckleRecordEnable() const
    {
        string uri = "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_100?Proxy=true";
        string knuckleRecordKey = "fingersense_screen_recording_enabled";
        auto value = OHOS::testserver::TestServerClient::GetInstance().GetValueFromDataShare(uri, knuckleRecordKey);
        LOG_D("key = %{public}s, value = %{public}s", knuckleRecordKey.c_str(), value.c_str());
        return atoi(value.c_str()) != 0;
    }
} // namespace OHOS::uitest