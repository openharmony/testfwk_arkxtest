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
    // 本地无效虚拟屏标识（区别于 Rosen::SCREEN_ID_INVALID）
    constexpr ScreenId INVALID_SCREEN_ID = 0;

    // 虚拟屏创建参数。字段语义与 OHOS::Rosen::VirtualScreenOption 一一对应，
    // 并补充投屏场景迁移的刷新率/隐私豁免/画布旋转配置。
    struct VirtualScreenOption {
        std::string name = "uitest";
        uint32_t width = 0;
        uint32_t height = 0;
        float density = 0;                               // 密度，<=0 时自动取默认屏 DPI
        Surface* surface = nullptr;                      // 虚拟屏输出 surface（如编码器 surface），调用方保证生命周期
        int32_t flags = 0;
        bool isForShot = false;
        std::vector<uint64_t> missionIds = {};
        uint32_t refreshInterval = 1;                    // 帧刷新间隔，帧率 = 120 / refreshInterval
        int32_t skipAppPid = -1;                         // 隐私豁免应用 pid，<0 表示不豁免
        std::vector<uint64_t> skipPrivacyWindowsId = {}; // 隐私豁免窗口
        bool enableCanvasRotation = true;                // 是否开启画布旋转
    };

    // 虚拟屏创建与管理类
    class VirtualScreen {
    public:
        VirtualScreen() = default;
        virtual ~VirtualScreen();
        // 创建虚拟屏并完成 surface/刷新率/隐私豁免/画布旋转配置
        bool Setup(const VirtualScreenOption &option);
        // 将主屏内容镜像到虚拟屏并激活
        bool Run(ScreenId mainScreenId);
        // 停止镜像并销毁虚拟屏
        void Stop();
        ScreenId GetVirtualScreenId() const;
        bool IsRunning() const;
    private:
        ScreenId virtualScreenId = INVALID_SCREEN_ID;
        bool isRunning = false;
    };

    // 全局生命周期入口（参考 ScreenCopy::StartScreenCopy/StopScreenCopy）
    bool StartVirtualScreen(const VirtualScreenOption &option, ScreenId mainScreenId);
    void StopVirtualScreen();
    // 获取当前运行中虚拟屏的 id，供调用方绑定 surface 等后续操作；无运行中虚拟屏时返回 INVALID_SCREEN_ID
    ScreenId GetRunningVirtualScreenId();
} // namespace OHOS::uitest

#endif // VIRTUAL_SCREEN_H
