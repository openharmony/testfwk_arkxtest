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

bool VirtualScreen::Setup(const VirtualScreenOption &option)
{
    if (option.width_ == 0 || option.height_ == 0) {
        LOG_E("Illegal virtual screen size, width=%{public}u, height=%{public}u", option.width_, option.height_);
        return false;
    }
    if (isRunning_) {
        Stop();
    }
    // 组装原生 VirtualScreenOption，语义与投屏工具 ScreenEncoder::Run 中的创建参数一致
    Rosen::VirtualScreenOption nativeOption;
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
    virtualScreenId_ = ScreenManager::GetInstance().CreateVirtualScreen(nativeOption);
    if (virtualScreenId_ == SCREEN_ID_INVALID) {
        LOG_E("Create virtual screen failed");
        virtualScreenId_ = INVALID_SCREEN_ID;
        return false;
    }
    LOG_I("Create virtual screen success, id=%{public}llu", (unsigned long long)virtualScreenId_);
    // 挂载 surface，将虚拟屏内容输出到调用方提供的 surface（如编码器 surface）
    if (option.surface_ != nullptr) {
        auto ret = ScreenManager::GetInstance().SetVirtualScreenSurface(virtualScreenId_, sptr<Surface>(option.surface_));
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual screen surface failed, ret=%{public}d", static_cast<int32_t>(ret));
            Stop();
            return false;
        }
    }
    // 设置刷新率（帧率 = 120 / refreshInterval）
    auto ret = ScreenManager::GetInstance().SetVirtualScreenRefreshRate(virtualScreenId_, option.refreshInterval_);
    if (ret != DMError::DM_OK) {
        LOG_E("Set virtual screen refresh rate failed, ret=%{public}d", static_cast<int32_t>(ret));
        Stop();
        return false;
    }
    // 隐私豁免：跳过指定应用的隐私窗口
    if (option.skipAppPid_ >= 0 && !option.skipPrivacyWindowsId_.empty()) {
        std::vector<uint64_t> skipPrivacyWindowsId = option.skipPrivacyWindowsId_;
        ret = DisplayManager::GetInstance().SetVirtualScreenSecurityExemption(
            virtualScreenId_, option.skipAppPid_, skipPrivacyWindowsId);
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual screen security exemption failed, ret=%{public}d", static_cast<int32_t>(ret));
            Stop();
            return false;
        }
    }
    // 开启画布旋转，保证镜像画面方向正确
    if (option.enableCanvasRotation_) {
        ret = ScreenManager::GetInstance().SetVirtualMirrorScreenCanvasRotation(virtualScreenId_, true);
        if (ret != DMError::DM_OK) {
            LOG_E("Set virtual mirror screen canvas rotation failed, ret=%{public}d", static_cast<int32_t>(ret));
            Stop();
            return false;
        }
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
        LOG_D("Use default display as main screen, mainScreenId=%{public}llu", (unsigned long long)mainScreenId);
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
    LOG_I("Virtual screen mirror started, virtualScreenId=%{public}llu", (unsigned long long)virtualScreenId_);
    return true;
}

void VirtualScreen::Stop()
{
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