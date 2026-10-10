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
#include <screen_manager.h>
#include "common_utilities_hpp.h"
#include "virtual_screen.h"

namespace OHOS::uitest {
using namespace std;
using namespace OHOS;
using namespace OHOS::Rosen;

static unique_ptr<VirtualScreen> g_virtualScreen = nullptr;

VirtualScreen::~VirtualScreen()
{
    Stop();
}

// 组装原生 VirtualScreenOption，语义与投屏工具 ScreenEncoder::Run 中的创建参数一致
static void BuildNativeOption(const VirtualScreenOption &option, Rosen::VirtualScreenOption &nativeOption)
{
    nativeOption.name_ = option.name;
    nativeOption.width_ = option.width;
    nativeOption.height_ = option.height;
    nativeOption.density_ = option.density;
    nativeOption.surface_ = sptr<Surface>(option.surface);
    nativeOption.flags_ = option.flags;
    nativeOption.isForShot_ = option.isForShot;
    nativeOption.missionIds_ = option.missionIds;
    if (nativeOption.density_ <= 0) {
        auto defaultDisplay = DisplayManager::GetInstance().GetDefaultDisplay();
        if (defaultDisplay != nullptr) {
            nativeOption.density_ = defaultDisplay->GetDpi();
            LOG_D("Use default display dpi, density=%{public}f", nativeOption.density_);
        }
    }
}

// 挂载 surface/设置刷新率/隐私豁免/开启画布旋转/锁定屏幕旋转，任一步失败即返回失败（由调用方回滚销毁）
static bool ConfigVirtualScreen(ScreenId screenId, const VirtualScreenOption &option)
{
    // 挂载 surface，将虚拟屏内容输出到调用方提供的 surface（如编码器 surface）
    if (option.surface != nullptr) {
        auto ret = ScreenManager::GetInstance().SetVirtualScreenSurface(screenId,
            sptr<Surface>(option.surface));
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual screen surface failed, ret=%{public}d", static_cast<int32_t>(ret));
            return false;
        }
    }
    // 设置刷新率（帧率 = 120 / refreshInterval）
    auto ret = ScreenManager::GetInstance().SetVirtualScreenRefreshRate(screenId, option.refreshInterval);
    if (ret != DMError::DM_OK) {
        LOG_E("Set virtual screen refresh rate failed, ret=%{public}d", static_cast<int32_t>(ret));
        return false;
    }
    // 隐私豁免：跳过指定应用的隐私窗口
    if (option.skipAppPid >= 0 && !option.skipPrivacyWindowsId.empty()) {
        std::vector<uint64_t> skipPrivacyWindowsId = option.skipPrivacyWindowsId;
        ret = DisplayManager::GetInstance().SetVirtualScreenSecurityExemption(
            screenId, option.skipAppPid, skipPrivacyWindowsId);
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual screen security exemption failed, ret=%{public}d", static_cast<int32_t>(ret));
            return false;
        }
    }
    // 开启画布旋转，保证镜像画面方向正确
    if (option.enableCanvasRotation) {
        ret = ScreenManager::GetInstance().SetVirtualMirrorScreenCanvasRotation(screenId, true);
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual mirror screen canvas rotation failed, ret=%{public}d", static_cast<int32_t>(ret));
            return false;
        }
    }
    // 锁定屏幕旋转，与画布旋转相互独立，按需单独启用
    if (option.screenRotationLocked) {
        ScreenManager::GetInstance().SetScreenRotationLocked(true);
    }
    return true;
}

bool VirtualScreen::Setup(const VirtualScreenOption &option)
{
    if (option.width == 0 || option.height == 0) {
        LOG_E("Illegal virtual screen size, width=%{public}u, height=%{public}u", option.width, option.height);
        return false;
    }
    if (isRunning) {
        Stop();
    }
    Rosen::VirtualScreenOption nativeOption;
    BuildNativeOption(option, nativeOption);
    virtualScreenId = ScreenManager::GetInstance().CreateVirtualScreen(nativeOption);
    if (virtualScreenId == SCREEN_ID_INVALID) {
        LOG_E("Create virtual screen failed");
        virtualScreenId = INVALID_SCREEN_ID;
        return false;
    }
    LOG_I("Create virtual screen success, id=%{public}llu", static_cast<unsigned long long>(virtualScreenId));
    if (!ConfigVirtualScreen(virtualScreenId, option)) {
        Stop();
        return false;
    }
    return true;
}

bool VirtualScreen::Run(ScreenId mainScreenId)
{
    if (virtualScreenId == INVALID_SCREEN_ID) {
        LOG_E("Virtual screen not created, call Setup first");
        return false;
    }
    if (isRunning) {
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
    std::vector<ScreenId> mirrorIds = { virtualScreenId };
    auto ret = ScreenManager::GetInstance().MakeMirror(mainScreenId, mirrorIds, screenGroupId);
    if (ret != DMError::DM_OK) {
        LOG_E("Make mirror failed, ret=%{public}d", static_cast<int32_t>(ret));
        return false;
    }
    isRunning = true;
    LOG_I("Virtual screen mirror started, virtualScreenId=%{public}llu",
        static_cast<unsigned long long>(virtualScreenId));
    return true;
}

void VirtualScreen::Stop()
{
    if (virtualScreenId == INVALID_SCREEN_ID) {
        return;
    }
    if (isRunning) {
        std::vector<ScreenId> mirrorIds = { virtualScreenId };
        auto ret = ScreenManager::GetInstance().StopMirror(mirrorIds);
        if (ret != DMError::DM_OK) {
            LOG_E("Stop mirror failed, ret=%{public}d", static_cast<int32_t>(ret));
        }
        isRunning = false;
    }
    auto ret = ScreenManager::GetInstance().DestroyVirtualScreen(virtualScreenId);
    if (ret != DMError::DM_OK) {
        LOG_E("Destroy virtual screen failed, ret=%{public}d", static_cast<int32_t>(ret));
    }
    virtualScreenId = INVALID_SCREEN_ID;
    LOG_I("Virtual screen destroyed");
}

ScreenId VirtualScreen::GetVirtualScreenId() const
{
    return virtualScreenId;
}

bool VirtualScreen::IsRunning() const
{
    return isRunning;
}

bool StartVirtualScreen(const VirtualScreenOption &option, ScreenId mainScreenId)
{
    StopVirtualScreen();
    g_virtualScreen = make_unique<VirtualScreen>();
    if (g_virtualScreen == nullptr || !g_virtualScreen->Setup(option)) {
        LOG_E("Setup virtual screen failed");
        StopVirtualScreen();
        return false;
    }
    if (!g_virtualScreen->Run(mainScreenId)) {
        LOG_E("Run virtual screen failed");
        StopVirtualScreen();
        return false;
    }
    return true;
}

void StopVirtualScreen()
{
    if (g_virtualScreen != nullptr) {
        g_virtualScreen->Stop();
        g_virtualScreen = nullptr;
    }
}

ScreenId GetRunningVirtualScreenId()
{
    return g_virtualScreen != nullptr ? g_virtualScreen->GetVirtualScreenId() : INVALID_SCREEN_ID;
}
} // namespace OHOS::uitest