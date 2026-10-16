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

#ifndef VIRTUAL_SCREEN_H
#define VIRTUAL_SCREEN_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
class Surface;
} // namespace OHOS

namespace OHOS::uitest {
    // 虚拟屏标识，与 OHOS::Rosen::ScreenId 保持一致
    using ScreenId = uint64_t;
    // 无效虚拟屏标识，取值与 Rosen::SCREEN_ID_INVALID（UINT64_MAX）一致；
    // 不用 0 作哨兵——物理默认屏 id 常为 0，DM 分配出 id=0 的虚拟屏会被误判为"未创建"
    constexpr ScreenId INVALID_SCREEN_ID = UINT64_MAX;
    // 虚拟屏最大帧率，刷新间隔与帧率换算基准（与投屏工具 MAX_FRAME_RATE 保持一致）
    constexpr uint32_t MAX_VIRTUAL_SCREEN_FPS = 120;

    // 虚拟屏创建参数。字段语义与 OHOS::Rosen::VirtualScreenOption 一一对应，
    // 并补充投屏场景迁移的刷新率/隐私豁免/画布旋转/旋转锁定配置。
    struct VirtualScreenOption {
        std::string name_ = "uitest";
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        float density_ = 0;                               // 密度，<=0 时自动取默认屏 DPI
        Surface* surface_ = nullptr;                      // 虚拟屏输出 surface（如编码器 surface），调用方保证生命周期
        int32_t flags_ = 0;
        bool isForShot_ = false;
        std::vector<uint64_t> missionIds_ = {};
        uint32_t refreshInterval_ = 1;                    // 帧刷新间隔，帧率 = 120 / refreshInterval
        int32_t skipAppPid_ = -1;                         // 隐私豁免应用 pid，<0 表示不豁免
        std::vector<uint64_t> skipPrivacyWindowsId_ = {}; // 隐私豁免窗口
        bool enableCanvasRotation_ = true;                // 是否开启画布旋转
        // 补充参数：投屏工具（scrcpy_server）start_record 新增的配置项
        uint32_t refreshRate_ = 0;                        // 刷新率 fps（0=不覆盖 refreshInterval_）
        bool screenRotationLocked_ = false;               // 锁定整机屏幕旋转；true 时强制关闭画布旋转（enableCanvasRotation_ 失效）
    };

    // 虚拟屏创建与管理类
    class VirtualScreen final {
    public:
        VirtualScreen() = default;
        ~VirtualScreen();
        VirtualScreen(const VirtualScreen &) = delete;
        VirtualScreen &operator=(const VirtualScreen &) = delete;
        // 创建虚拟屏并完成 surface/刷新率/隐私豁免/画布旋转/旋转锁定配置
        bool Setup(const VirtualScreenOption &option);
        // 将主屏内容镜像到虚拟屏并激活
        bool Run(ScreenId mainScreenId);
        // 停止镜像、恢复屏幕旋转锁定并销毁虚拟屏
        void Stop();
        ScreenId GetVirtualScreenId() const;
        bool IsRunning() const;
    private:
        ScreenId virtualScreenId_ = INVALID_SCREEN_ID;
        bool isRunning_ = false;
        bool rotationLocked_ = false; // 本实例是否锁定过屏幕旋转，Stop 时对称恢复
    };

    // 全局生命周期入口（参考 ScreenCopy::StartScreenCopy/StopScreenCopy）
    bool StartVirtualScreen(const VirtualScreenOption &option, ScreenId mainScreenId);
    void StopVirtualScreen();
    // 获取当前运行中虚拟屏的 id，供调用方绑定 surface 等后续操作；无运行中虚拟屏时返回 INVALID_SCREEN_ID
    ScreenId GetRunningVirtualScreenId();
} // namespace OHOS::uitest

#endif // VIRTUAL_SCREEN_H
