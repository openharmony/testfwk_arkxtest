/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <display_manager.h>
#include <memory>
#include <mutex>
#include <screen_manager.h>
#include "common_utilities_hpp.h"
#include "virtual_screen.h"

namespace OHOS::uitest {
using namespace std;
using namespace OHOS;
using namespace OHOS::Rosen;

static unique_ptr<VirtualScreen> g_virtualScreen = nullptr;
// g_virtualScreen 启停互斥：扩展侧 startCapture 在释放 g_captureLock 后才调用 StartVirtualScreen，
// 而 stopCapture 持 g_captureLock 调用 StopVirtualScreen，并发的 start/stop 会竞争该全局指针；
// 本锁在模块内部串行化全局单例的创建/销毁/查询（锁序恒为 g_captureLock → 本锁，无反向嵌套）
static mutex g_virtualScreenLock;

VirtualScreen::~VirtualScreen()
{
    Stop();
}

// 组装原生 VirtualScreenOption，语义与投屏工具 ScreenEncoder::Run 中的创建参数一致
static void BuildNativeOption(const VirtualScreenOption &option, Rosen::VirtualScreenOption &nativeOption)
{
    nativeOption.name_ = option.name_;
    nativeOption.width_ = option.width_;
    nativeOption.height_ = option.height_;
    nativeOption.density_ = option.density_;
    nativeOption.surface_ = sptr<Surface>(option.surface_);
    nativeOption.flags_ = option.flags_;
    nativeOption.isForShot_ = option.isForShot_;
    nativeOption.missionIds_ = option.missionIds_;
    if (nativeOption.density_ <= 0) {
        auto defaultDisplay = DisplayManager::GetInstance().GetDefaultDisplay();
        if (defaultDisplay != nullptr) {
            nativeOption.density_ = defaultDisplay->GetDpi();
            LOG_D("Use default display dpi, density=%{public}f", nativeOption.density_);
        }
    }
}

// 挂载 surface/设置刷新率/隐私豁免/开启画布旋转，任一步失败即返回失败（由调用方回滚销毁）
static bool ConfigVirtualScreen(ScreenId screenId, const VirtualScreenOption &option)
{
    // 挂载 surface，将虚拟屏内容输出到调用方提供的 surface（如编码器 surface）
    if (option.surface_ != nullptr) {
        auto ret = ScreenManager::GetInstance().SetVirtualScreenSurface(screenId,
            sptr<Surface>(option.surface_));
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual screen surface failed, ret=%{public}d", static_cast<int32_t>(ret));
            return false;
        }
    }
    // 设置刷新率（帧率 = 120 / refreshInterval）
    auto ret = ScreenManager::GetInstance().SetVirtualScreenRefreshRate(screenId, option.refreshInterval_);
    if (ret != DMError::DM_OK) {
        LOG_E("Set virtual screen refresh rate failed, ret=%{public}d", static_cast<int32_t>(ret));
        return false;
    }
    // 隐私豁免：跳过指定应用的隐私窗口
    if (option.skipAppPid_ >= 0 && !option.skipPrivacyWindowsId_.empty()) {
        std::vector<uint64_t> skipPrivacyWindowsId = option.skipPrivacyWindowsId_;
        ret = DisplayManager::GetInstance().SetVirtualScreenSecurityExemption(
            screenId, option.skipAppPid_, skipPrivacyWindowsId);
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual screen security exemption failed, ret=%{public}d", static_cast<int32_t>(ret));
            return false;
        }
    }
    // 开启画布旋转，保证镜像画面方向正确
    if (option.enableCanvasRotation_) {
        ret = ScreenManager::GetInstance().SetVirtualMirrorScreenCanvasRotation(screenId, true);
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual mirror screen canvas rotation failed, ret=%{public}d", static_cast<int32_t>(ret));
            return false;
        }
    }
    return true;
}

bool VirtualScreen::Setup(const VirtualScreenOption &option)
{
    if (option.width_ == 0 || option.height_ == 0) {
        LOG_E("Illegal virtual screen size, width=%{public}u, height=%{public}u", option.width_, option.height_);
        return false;
    }
    if (isRunning_) {
        Stop();
    }
    Rosen::VirtualScreenOption nativeOption;
    BuildNativeOption(option, nativeOption);
    virtualScreenId_ = ScreenManager::GetInstance().CreateVirtualScreen(nativeOption);
    if (virtualScreenId_ == SCREEN_ID_INVALID) {
        LOG_E("Create virtual screen failed");
        virtualScreenId_ = INVALID_SCREEN_ID;
        return false;
    }
    LOG_I("Create virtual screen success, id=%{public}llu", static_cast<unsigned long long>(virtualScreenId_));
    if (!ConfigVirtualScreen(virtualScreenId_, option)) {
        Stop();
        return false;
    }
    // 锁定整机屏幕旋转（全局状态，非虚拟屏自身配置），Stop 时对称恢复
    if (option.screenRotationLocked_) {
        ScreenManager::GetInstance().SetScreenRotationLocked(true);
        rotationLocked_ = true;
    }
    return true;
}

bool VirtualScreen::Run(ScreenId mainScreenId)
{
    if (virtualScreenId_ == INVALID_SCREEN_ID) {
        LOG_E("Virtual screen not created, call Setup first");
        return false;
    }
    if (isRunning_) {
        LOG_W("Virtual screen is already running");
        return true;
    }
    if (mainScreenId == SCREEN_ID_INVALID) {
        mainScreenId = static_cast<ScreenId>(DisplayManager::GetInstance().GetDefaultDisplayId());
        LOG_D("Use default display as main screen, mainScreenId=%{public}llu",
            static_cast<unsigned long long>(mainScreenId));
    }
    if (mainScreenId == SCREEN_ID_INVALID) {
        LOG_E("Get default display id failed");
        return false;
    }
    // 镜像组 id 取主屏 screen id
    ScreenId screenGroupId = mainScreenId;
    auto display = DisplayManager::GetInstance().GetDisplayByScreen(mainScreenId);
    if (display != nullptr) {
        screenGroupId = display->GetScreenId();
    }
    // 建立镜像：把主屏内容镜像到虚拟屏
    std::vector<ScreenId> mirrorIds = { virtualScreenId_ };
    auto ret = ScreenManager::GetInstance().MakeMirror(mainScreenId, mirrorIds, screenGroupId);
    if (ret != DMError::DM_OK) {
        LOG_E("Make mirror failed, ret=%{public}d", static_cast<int32_t>(ret));
        return false;
    }
    isRunning_ = true;
    LOG_I("Virtual screen mirror started, virtualScreenId=%{public}llu",
        static_cast<unsigned long long>(virtualScreenId_));
    return true;
}

void VirtualScreen::Stop()
{
    // 对称恢复屏幕旋转锁定（对应 Setup 中的 SetScreenRotationLocked(true)），先于其余清理执行
    if (rotationLocked_) {
        ScreenManager::GetInstance().SetScreenRotationLocked(false);
        rotationLocked_ = false;
    }
    if (virtualScreenId_ == INVALID_SCREEN_ID) {
        return;
    }
    if (isRunning_) {
        std::vector<ScreenId> mirrorIds = { virtualScreenId_ };
        auto ret = ScreenManager::GetInstance().StopMirror(mirrorIds);
        if (ret != DMError::DM_OK) {
            LOG_E("Stop mirror failed, ret=%{public}d", static_cast<int32_t>(ret));
        }
        isRunning_ = false;
    }
    auto ret = ScreenManager::GetInstance().DestroyVirtualScreen(virtualScreenId_);
    if (ret != DMError::DM_OK) {
        LOG_E("Destroy virtual screen failed, ret=%{public}d", static_cast<int32_t>(ret));
    }
    virtualScreenId_ = INVALID_SCREEN_ID;
    LOG_I("Virtual screen destroyed");
}

ScreenId VirtualScreen::GetVirtualScreenId() const
{
    return virtualScreenId_;
}

bool VirtualScreen::IsRunning() const
{
    return isRunning_;
}

// 销毁当前全局虚拟屏实例，调用方须已持有 g_virtualScreenLock
static void DestroyVirtualScreenLocked()
{
    if (g_virtualScreen != nullptr) {
        g_virtualScreen->Stop();
        g_virtualScreen = nullptr;
    }
}

bool StartVirtualScreen(const VirtualScreenOption &option, ScreenId mainScreenId)
{
    lock_guard<mutex> guard(g_virtualScreenLock);
    DestroyVirtualScreenLocked();
    g_virtualScreen = make_unique<VirtualScreen>();
    if (g_virtualScreen == nullptr || !g_virtualScreen->Setup(option)) {
        LOG_E("Setup virtual screen failed");
        DestroyVirtualScreenLocked();
        return false;
    }
    if (!g_virtualScreen->Run(mainScreenId)) {
        LOG_E("Run virtual screen failed");
        DestroyVirtualScreenLocked();
        return false;
    }
    return true;
}

void StopVirtualScreen()
{
    lock_guard<mutex> guard(g_virtualScreenLock);
    DestroyVirtualScreenLocked();
}

ScreenId GetRunningVirtualScreenId()
{
    lock_guard<mutex> guard(g_virtualScreenLock);
    return g_virtualScreen != nullptr ? g_virtualScreen->GetVirtualScreenId() : INVALID_SCREEN_ID;
}
} // namespace OHOS::uitest
