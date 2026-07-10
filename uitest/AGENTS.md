# UiTest - AI 知识库

## 概述

uitest 是 OpenHarmony 的 UI 自动化测试框架，采用**客户端-服务端架构**，多语言支持（ArkTS-Static、ArkTS-Dynamic、Cangjie）。

构建、部署、单元测试命令统一见根 `../AGENTS.md` § 构建和验证（含 uitest 单组件构建目标、产物路径、设备推送、单元测试目标）。

## 知识路由

改动前按任务/路径/词汇先读对应章节；编辑前须声明 ① 任务属哪一行 ② 已读本文件对应章节 ③ 触发了哪些项目约束。

| 触发条件 | 主要文件（高频落点） | 先读本文件 |
| --- | --- | --- |
| 新增/改 uitest API（Driver/On/Component/UiWindow） | `core/frontend_api_handler.cpp`（108KB，API 分发核心）、`core/frontend_api_defines.h` | § 新增 API + § 项目约束（ask-before: .d.ts 兼容） |
| 改组件匹配算法 | `core/widget_selector.cpp` + `core/select_strategy.cpp` | § 组件流程第 1 条 |
| 改输入注入 | `input/ui_input.cpp` | § Display ID 传播（输入注入路由） |
| 改多用户/多显示器逻辑 | `server/system_ui_controller.cpp` | § 多屏多用户 + § Display ID 传播 |
| 改 NAPI/ANI/CJ 任一绑定 | `napi/uitest_napi.cpp` / `ets/ani/` / `cj/uitest_ffi.cpp` | § 新增 API 第 3 步 + § 项目约束（禁止: 三绑定同步） |
| 加 C++ 单元测试 | `test/<classname>_test.cpp` + `BUILD.gn` | — |
| 任务提到 `displayId` / 多用户 / 多显示器 / 跨屏 | — | § 多屏多用户 + § Display ID 传播 |
| 任务提到 `PenKey` / `triggerPenKey` / 触控笔 | — | § 组件流程第 2 条 + § 项目约束（禁止: triggerPenKey gate） |

## 代码风格

### C++ 代码风格

- **头文件保护**：`#ifndef FILE_NAME_H` / `#define` / `#endif`——禁用 `#pragma once`。每个文件须有 Apache 2.0 许可证头（`Copyright (c) YYYY Huawei Device Co., Ltd.`）。
- **命名空间**：`namespace OHOS::uitest { ... } // namespace OHOS::uitest`。函数体**4 空格缩进**。文件顶部常见 `using namespace std;` 和 `using namespace nlohmann;`。
- **命名**：类/函数/结构体 PascalCase（`UiDriver`、`FindWidgets`、`ApiCallErr`）；成员变量下划线后缀（`uiController_`、`code_`）；`constexpr` 常量 UPPER_CASE（`INDEX_ZERO`、`NAPI_MAX_BUF_LEN`）。**禁用 `m_` 前缀。** 普通结构体数据字段常无后缀（`bundleName`、`text`）。
- **头文件包含**：系统 → 第三方（`nlohmann/json.hpp`、`napi/native_api.h`）→ 本项目头文件（`ui_driver.h`、`common_utilities_hpp.h`）。条件包含用 `#ifdef ARKXTEST_*_ENABLE` 保护。
- **智能指针**：惯用 `unique_ptr`/`shared_ptr` + `make_unique`/`move`。IPC 引用计数对象使用 OHOS `sptr`/`wptr`。
- **现代写法**：虚函数重写加 `override`；`~Foo() = default`。禁止用异常做流程控制。

### 错误处理与日志

- **禁止用异常做流程控制。** uitest 使用出参 `ApiCallErr& err`（最后一个有意义参数，`frontend_error_defines.h:67`，持有 `ErrCode code_` + `message_`）。错误码定义在 `frontend_error_defines.h:20`（`ERR_INVALID_INPUT=401`、`ERR_OPERATION_UNSUPPORTED=17000005` 等）。
- **`DCHECK(cond)` 是致命的**：记录 `LOG_E` + `_Exit(0)`（`common_utilities_hpp.h:164`）。仅用于不变量断言，不可用于可恢复错误。
- **日志**：`LOG_D/I/W/E(fmt, args...)` 宏（自动从文件/函数名生成 tag）。格式串使用 HiLog 隐私标签：`%{public}s`、`%{public}d`。每个 BUILD.gn 目标须定义 `LOG_TAG`（`UiTestKit_Base`、`UiTestKit_Server` 等）。

### ArkTS 风格（绑定与 API）

- **`.ets`（ArkTS-Static）** 文件在 `ets/ani/ets/` 下，使用 `'use static'` 风格的完整类型注解，通过 `export native function` 声明原生函数，用 `loadLibraryWithPermissionCheck` 加载库。导入 OHOS kit（`@ohos.hilog`、`@ohos.base`）。
- **NAPI `.cpp`**（`napi/uitest_napi.cpp`）：通过 `napi_module_register` 注册模块；用辅助函数（`JsStrToCppStr`）做 ArkTS↔C++ 转换。可选特性用 `#ifdef ARKXTEST_API_METRICS_ENABLE` 保护。
- **字符串**：ArkTS 中用单引号；日志用模板字符串。

## 架构

### 客户端-服务端模型

```
测试应用（ArkTS）
    ↓ NAPI/ANI 绑定
uitest_client（libuitest.z.so/libuitest_ani.so）
    ↓ IPC（CommonEvent）
uitest_server（uitest 守护进程）
    ↓ Accessibility/Window Manager
被测系统
```

### 组件流程

1. **组件查找**：`Driver.findComponent(ON.text('Hello'))` → IPC → Server → `WidgetSelector` → UI Model
   - 通过 `On` 相对定位：`isBefore`/`isAfter`/`within`（按 `On` matcher），`beforeComponent`/`afterComponent`/`withinComponent`（按具体 `Component` 对象，API 26 起）
   - `beforeComponent`/`afterComponent` 方向**反直觉**：`beforeComponent` 调用 `AddRearLocator`（目标在树序中位于给定组件*之后*），`afterComponent` 调用 `AddFrontLocator`（目标位于*之前*）。`withinComponent` 调用 `AddParentLocator`。
   - 传入这些方法的 `Component` 会通过 `RetrieveWidget` 重新校验；若在当前 UI 中已不存在，返回 `ERR_INVALID_PARAM`。
2. **组件操作**：`Component.click()` → IPC → Server → `WidgetOperator` → Input System
   - 坐标类操作：`clickAt`/`doubleClickAt`/`longClickAt`/`swipeBetween`/`dragBetween`，以及 API 26 起的 `*WithOptions` 变体，接受 `TouchOptions`（speed/duration/pressure）做精细控制
   - **TouchOptions 字段校验**：每个 `*WithOptions` 方法只接受 `TouchOptions` 的特定子集（`ValidateTouchOptions` in `frontend_api_handler.cpp:1331`）。传入不支持的字段返回 `ERR_INVALID_INPUT`。各方法允许字段：`clickAtWithOptions`→{pressure}、`longClickAtWithOptions`→{duration,pressure}、`swipeBetweenWithOptions`→{speed,pressure}、`dragBetweenWithOptions`→{speed,duration,pressure}、`mouseDragWithOptions`→{speed,duration}。
   - 带组合键的鼠标拖拽：`mouseDragWithOptions(from, to, touchOptions?, keyOptions?)`（API 26 起），`KeyOptions` 提供 `key1`/`key2` 组合键。`key2` 无 `key1` 返回 `ERR_INVALID_PARAM`（`ParseKeyOptions` in `frontend_api_handler.cpp:1762`）。
   - 触控笔按键操作：`triggerPenKey(key, mode, operation, options?)`（API 26 起），使用 `PenKey`/`PenMode`/`PenKeyOperation`/`PenKeyOperationOptions`
   - **PenKey 合法组合**：由白名单强制（`VALID_PEN_KEY_COMBINATIONS` in `frontend_api_handler.cpp:1105`）。AIR_MOUSE 模式 + AIR_MOUSE 键要求 options 含 `point`。非法组合返回 `ERR_INVALID_PARAM`。
   - **PenKey 特性 gate**：`triggerPenKey` 受 `ARKXTEST_TRIGGER_PEN_KEY_ENABLE` 控制，仅对 `pc`/`tablet`/`phone` 产品特性定义（`BUILD.gn:251`）。不支持的产品上 `IsPenKeySupported` 返回 false，API 返回 `ERR_OPERATION_UNSUPPORTED`（17000005）。
   - **PenKey 内部分发**：`PenKeyAction::IsMouseKeyCombo()`（AIR_MOUSE 键 + AIR_MOUSE 模式）路由到 `InjectMouseEventSequence`；其余组合路由到 `InjectKeyEventSequence`（`ui_driver.cpp:324`）。
   - 多显示器布局转储：`dumpLayout(savePath, displayId?)`（API 26 起）。`savePath` 在客户端转为文件描述符；服务端校验 `displayId` 存在（`CheckDisplayExist`）后通过 `write(fd, ...)` 写 JSON。

### 多屏多用户（API 26 起）

> 当任务涉及 `displayId`、多用户、多显示器、跨屏操作时，必读本节及 § Display ID 传播与多显示器操作。

API 26 之前，uitest server 仅以默认用户（user 0）连接 AAMS（AccessibilityAbilityManagerService，无障碍系统服务）。带 `displayId` 参数的 API 可操作指定屏幕，但**若该屏幕属于非 user 0 用户，操作会静默失败**——AAMS 作用域限于 user 0，看不到该用户的 UI 树。

API 26 引入动态 AAMS 用户切换，支持**多显示器多用户并发场景**（如多用户模式下每个显示器分配给不同用户）：

**机制（`SysUiController::ConvertAAMS` in `system_ui_controller.cpp:1525`）：**

1. **单用户快速路径**：初始化时 `Initialize()` 查询 `TestServerClient::GetUserCounts()`。若仅 1 个用户，`isSingleUser_ = true` 且 `ConvertAAMS` 短路（直接返回 true，不做切换）——与 API 26 前行为一致，零开销。
2. **多用户切换**：当 `isSingleUser_` 为 false 时，每次 `GetUiWindows(displayId)` / `GetWidgetsInWindow(winInfo)` 调用先执行 `ConvertAAMS(displayId)`：
   - 查询 `TestServerClient::GetUserIdByDisplayId(displayId)` 确定显示器所属用户。
   - 若显示器用户与 `currentUser_` 相同，无需切换。
   - 若不同，调用 `DisConnectFromSysAbility()`（断开旧用户 AAMS），再 `ConnectToSysAbility()` 以新用户身份通过 `AccessibilityUITestAbility::Connect(userId)` 重连 AAMS（`system_ui_controller.cpp:1179`）。
3. **Shell `dumpLayout`**：`server_main.cpp:228` 在创建 controller 前也调用 `GetUserIdByDisplayId` + `SetActiveUser(userId)`，确保 shell 命令针对正确用户的显示器。

**关键实现细节：**
- **同一时刻仅一个 AAMS 连接**：server 持有一条 AAMS 连接（对应一个用户）。`ConvertAAMS` 按需切换——跨不同用户显示器的操作由 `dumpMtx` 互斥锁**串行化**（`system_ui_controller.h:109`），非真正并发。
- **`currentUser_` 追踪**：初始化为 `-1`（默认 `ability->Connect()` 无 userId 即 user 0）；连接成功后更新为 `ability->GetCurrentUserId()`（`:1198`）；由 shell/IPC 的 `SetActiveUser(userId)` 更新。
- **TestServer 依赖**：`GetUserIdByDisplayId` / `GetUserCounts` 由 TestServer SA 5502 提供（`ITestServerInterface.idl:38-39`）。多用户支持要求 TestServer 运行且感知显示器到用户的映射。

**受影响 API**：任何最终以 `displayId` 调用 `GetUiWindows` 或 `GetWidgetsInWindow` 的 Driver/Component API——包括 `findComponent`/`dumpLayout`/`screenCap(displayId)`/指定显示器上的坐标操作。

### Display ID 传播与多显示器操作

> 当任务涉及坐标操作、跨屏、输入注入时，必读本节（含跨屏约束表）。

每个接受 `displayId`（或内嵌 `displayId` 的 `Point`）的 API 遵循分层传播路径：

```
ArkTS API（displayId 参数）
  → frontend_api_handler.cpp：从 Point JSON 读取 displayId（key "displayId"，默认 UNASSIGNED=-1）
    → UiDriver：CheckDisplayExist(displayId) 通过 DisplayManager 校验
      → SysUiController：
        - GetUiWindows/GetWidgetsInWindow：ConvertAAMS(displayId) 按需切换用户
        - InjectTouchEventSequence：pointerEvent->SetTargetDisplayId(displayId)
        - InjectMouseEventSequence：pointerEvent->SetTargetDisplayId(displayId)
        - InjectKeyEventSequence：KeyEvent::SetDisplayId(displayId)
```

**`UNASSIGNED` (-1) 解析**：当 `displayId` 未指定（默认 `-1`）时，`GetValidDisplayId`（`system_ui_controller.cpp:659`）解析为 `DisplayManager::GetDefaultDisplayId()`——通常是 display 0（主屏）。适用于所有触控/鼠标/按键注入及坐标转换。

**坐标操作的跨屏约束**（`CheckPointDisplayId` in `frontend_api_handler.cpp:1291`）：

涉及两点（from/to）的操作强制两点属于同一显示器，除非明确允许：

| 操作 | 允许跨屏？ | 行为 |
|-----------|------|------|
| `swipe`/`swipeBetween`/`drag`/`dragBetween`（+ `*WithOptions`） | **否** | `from.displayId != to.displayId` 时返回 `ERR_INVALID_INPUT` |
| `longClickComponentPresent`/`dragComponentPresent`（滑动变体） | **否** | 同上 |
| `penSwipe` | **否** | 同上 |
| `mouseDrag`/`mouseDragWithOptions` | **是** | `allowCrossScreen=true`，可跨不同显示器 |
| 单点操作（`clickAt`、`longClickAt`、`penClick` 等） | N/A | 单一 displayId，无约束 |

当一个点 `displayId=UNASSIGNED` 而另一个有具体值时，`UNASSIGNED` 点自动继承另一个的 displayId（`CheckPointDisplayId:1293-1298`）。

**坐标转换**（`ConvertRelativeToGlobal` in `system_ui_controller.cpp:1329`）：通过 `DisplayManager::ConvertRelativeCoordinateToGlobal` 将每显示器相对坐标转为全局屏幕坐标。当输入子系统要求全局坐标但 API 收到的是显示器局部坐标时需要此转换。

**组件操作的 displayId 透传**（`UiDriver::displayToWindowCacheMap_`）：`UiDriver` 维护 `map<displayId, vector<WindowCacheModel>>`（`ui_driver.cpp:110`）。`UpdateUIWindows(targetDisplay)` 仅获取指定显示器的窗口（`targetDisplay=-1` 时获取所有），`DumpUiHierarchy`/`FindWidgets` 按 `displayId` 查缓存（`:120`、`:169`、`:236`）。组件操作从缓存的 `Window` 取组件 `displayId` 并透传，确保 AAMS 作用域正确。

**输入注入路由**：触控/鼠标/按键事件通过 `PointerEvent::SetTargetDisplayId` / `KeyEvent::SetDisplayId` 携带 `displayId`，使输入子系统将事件路由到正确物理屏。此机制独立于上述 AAMS 用户切换——输入注入可在任何显示器工作，与当前连接的是哪个用户的 UI 树无关。

## 测试运行与验证

构建、推送、运行单元测试和应用测试的命令统一见根 `../AGENTS.md` § 构建和验证（含 uitest 单元测试目标说明：core/ipc/extension 各测什么）。以下为 uitest 特有的验证要求。

### 按改动类型的验证矩阵

不同改动类型必须跑的验证不同，按下表对号入座：

| 改动类型 | 必跑构建目标 | 必跑 unittest | 必跑 shell 回归 | 备注 |
| --- | --- | --- | --- | --- |
| C++ core（`frontend_api_handler`/`widget_selector`/`select_strategy` 等） | `uitestkit` | `uitest_core_unittest` | `dumpLayout` + 至少一条 `uiInput` | 见 § Shell 命令 |
| IPC 通信层（`connection/ipc_transactor.cpp`） | `uitestkit` | `uitest_ipc_unittest` | `start-daemon` 验证连通 | — |
| 扩展执行器（`addon/extension_executor`） | `uitestkit` | `uitest_extension_unittest` | — | — |
| 多显示器/输入注入（`system_ui_controller`/`ui_input`） | `uitestkit` | `uitest_core_unittest` | `dumpLayout -d <displayId>` + `uiInput` 跨屏验证 | 见 § 多屏多用户、§ Display ID 传播 |
| NAPI/ANI/CJ 绑定层 | `uitestkit` | — | `aa test` 应用测试（见根 § 应用测试） | 三绑定一致性：改一处须验证对应绑定编译通过 |
| 新增/改公共 `.d.ts` 签名 | `uitestkit` | — | `aa test` 应用测试 | 向后兼容规则见根 `../AGENTS.md` § 项目约束 |

### Done 定义

模板见根 `../AGENTS.md` § Done 定义。uitest 专属项：
- 构建目标：`uitestkit`
- 运行证据：对应 unittest；涉及 device-side 场景须附 shell 回归输出（`dumpLayout`/`uiInput` 等，命令见 § Shell 命令）
- 约束确认：三绑定同步 / 跨屏约束 / feature gate

### 无法 device-side 验证时的兜底

见根 `../AGENTS.md` § 无法 device-side 验证时的兜底。

## 新增 API

**工作流：**

1. **在 `core/frontend_api_defines.h` 中定义 API**
   - 向 `FrontendApiType` 添加枚举值
   - 定义 JSON 请求/响应 schema

2. **在 `core/frontend_api_handler.cpp` 中实现服务端逻辑**
   - 添加映射到 API 类型的处理函数
   - 调用 operator 或直接实现逻辑

3. **添加客户端绑定**
   - **ArkTS-Static**：在 `ets/ani/ets/@ohos.UiTest.ets` 中添加 native 方法，在 `ets/ani/src/uitest_ani.cpp` 中实现
   - **ArkTS-Dynamic**：在 `napi/uitest_napi.cpp` 中添加

4. **声明 API**：在 `../../../../interface/sdk-js/api/@ohos.UiTest.d.ts` 中声明

5. **更新文档**
   - `../../../../docs/zh-cn/application-dev/application-test/uitest-guidelines.md`
   - `../../../../docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md`

## 目录结构

```
uitest/
├── core/              # 核心测试逻辑
│   ├── ui_driver.cpp/h           # 主驱动
│   ├── ui_model.cpp/h            # 组件数据模型
│   ├── widget_selector.cpp/h     # 组件匹配 (40KB)
│   ├── widget_operator.cpp/h     # 组件操作
│   ├── window_operator.cpp/h     # 窗口管理
│   ├── select_strategy.cpp/h     # 选择算法 (38KB)
│   ├── frontend_api_handler.cpp  # API 请求分发器 (108KB)
│   ├── dump_handler.cpp          # 布局转储
│   └── frontend_api_defines.h    # API 定义与枚举
│
├── server/            # 服务端守护进程
│   ├── server_main.cpp            # 入口，shell 处理
│   └── system_ui_controller.cpp/h # 服务端逻辑 (63KB)
│
├── connection/        # IPC 层
│   └── ipc_transactor.cpp/h       # 基于 CommonEvent 的 IPC
│
├── input/             # 输入注入
│   └── ui_input.cpp/h            # 触控/按键/鼠标/笔事件
│
├── record/            # 录制
│   ├── ui_record.cpp/h           # 录制控制器
│   ├── pointer_tracker.cpp/h     # 手势追踪
│   ├── keyevent_tracker.cpp/h    # 按键事件追踪
│   └── velocity_tracker.cpp/h    # 速度计算
│
├── addon/             # 扩展与截图
│   ├── extension_executor.cpp/h  # 扩展执行器
│   └── screen_copy.cpp/h         # 截图
│
├── ets/ani/           # ArkTS-Static 绑定
│   ├── ets/@ohos.UiTest.ets      # ArkTS API (2020 行)
│   └── src/uitest_ani.cpp        # ANI 实现
│
├── napi/              # ArkTS-Dynamic 绑定
│   ├── uitest_napi.cpp           # NAPI 绑定 (30KB)
│   └── ui_event_observer_napi.cpp/h
│
├── cj/                # Cangjie FFI 绑定
│   ├── uitest_ffi.cpp            # FFI 绑定
│   └── ui_event_observer_impl.cpp/h
│
├── test/              # 单元测试（每个 <classname>_test.cpp 对应 core/ 或 server/ 中同名源文件）
│   ├── ui_driver_test.cpp        # ↔ core/ui_driver.cpp
│   ├── widget_selector_test.cpp  # ↔ core/widget_selector.cpp
│   ├── widget_operator_test.cpp  # ↔ core/widget_operator.cpp
│   ├── frontend_api_handler_test.cpp # ↔ core/frontend_api_handler.cpp
│   ├── select_strategy_test.cpp  # ↔ core/select_strategy.cpp
│   └── ...
│
└── BUILD.gn           # 构建配置
```

## Shell 命令

**位置**：`/system/bin/uitest`。uitest 改动后须用以下 shell 命令回归验证，防止影响现有功能。

**1. 截图**
```bash
hdc shell uitest screenCap -p /data/local/tmp/screenshot.png [-d <displayId>]
```

**2. 布局转储**
```bash
# 转储所有窗口
hdc shell uitest dumpLayout -p /data/local/tmp/layout.json

# 含扩展属性
hdc shell uitest dumpLayout -p /data/local/tmp/layout.json -a

# 指定窗口
hdc shell uitest dumpLayout -p /data/local/tmp/app.json -b com.example.app

# 自定义属性
hdc shell uitest dumpLayout -p /data/local/tmp/custom.json -e customAttr
```

**API 用法**：
dumpLayout API（API 26.0.0）支持格式：
- `dumpLayout(savePath: string, displayId?: int)`——保存布局到指定路径（内部转为文件描述符），可选 display ID 参数

选项：`-i`（不合并）、`-a`（含字体属性）、`-b`（bundleName）、`-w`（windowId）、`-m`（合并）、`-d`（displayId）、`-e`（自定义属性）

**3. 启动守护进程**
```bash
hdc shell uitest start-daemon <token>
# token 格式：bundleName@pid@uid@area
```

**4. 录制 UI 操作**

- 开始录制，将 UI 事件信息写入 csv 文件
```bash
hdc shell uitest uiRecord record -W true -l
```
选项：`-W` <true/false>（是否保存组件信息，true 为保存）、`-l`（每次操作后保存当前布局信息）、`-c` <true/false>（是否将 UI 事件信息打印到控制台，true 为打印）

- 读取录制文件，将文件内容打印到控制台
```
hdc shell uitest uiRecord read
```

**5. 注入输入**
```bash
# 点击
hdc shell uitest uiInput click 500 1000

# 滑动
hdc shell uitest uiInput swipe 100 1000 900 1000 600

# 按键事件
hdc shell uitest uiInput keyEvent Back

# 组合键（Ctrl+C）
hdc shell uitest uiInput keyEvent 17 67

# 输入文本
hdc shell uitest uiInput inputText 500 1000 "Hello"
```

uiInput 子命令：

| 命令 | 说明 |
|------|------|
| `help` | uiInput 命令帮助信息 |
| `click` | 模拟单击 |
| `doubleClick` | 模拟双击 |
| `longClick` | 模拟长按 |
| `fling` | 模拟快速滑动 |
| `swipe` | 模拟滑动 |
| `drag` | 模拟拖拽 |
| `dircFling` | 模拟定向滑动 |
| `inputText` | 在指定坐标处输入文本 |
| `text` | 在当前焦点处输入文本（不指定坐标） |
| `keyEvent` | 模拟物理按键（如键盘、电源键、返回、主页）及组合键操作 |


## 组件层级

格式：`<root>/<window>/<widget>/<child>/...`

示例：`root/0/1/3/5`

由 `ui_model.cpp` 中的 `WidgetHierarchyBuilder` 构建。

## 产品特性

| Flag | 说明 |
|------|------|
| `ARKXTEST_TABLET_FEATURE_ENABLE` | 平板特性 |
| `ARKXTEST_PC_FEATURE_ENABLE` | PC 特性（窗口操作） |
| `ARKXTEST_WATCH_FEATURE_ENABLE` | 手表特性（表冠旋转） |
| `ARKXTEST_KNUCKLE_ACTION_ENABLE` | 指关节手势 |
| `ARKXTEST_ADJUST_WINDOWMODE_ENABLE` | 窗口模式调整 |
| `ARKXTEST_TRIGGER_PEN_KEY_ENABLE` | 触控笔按键操作（`triggerPenKey`）；仅 pc/tablet/phone |
| `HIDUMPER_ENABLED` | HiDumper 集成 |

## 项目约束

根 `../AGENTS.md` § 项目约束 的跨组件通用约束同样适用。以下为 uitest 专属约束：

**禁止**：
- 跳过 `CheckPointDisplayId`（`frontend_api_handler.cpp:1291`）的跨屏坐标校验——swipe/drag/penSwipe 禁止跨屏，仅 mouseDrag 例外（见 § Display ID 传播 跨屏约束表）。
- 在 `ConvertAAMS` 路径移除 `dumpMtx` 互斥锁（`system_ui_controller.h:109`）——AAMS 单连接串行化是多用户正确性前提（见 § 多屏多用户）。
- 改 uitest C++ core 后不同步 ANI（`ets/ani/`）+ NAPI（`napi/`）+ CJ（`cj/`）三套绑定 + `.d.ts` 声明。
- `triggerPenKey` 受 `ARKXTEST_TRIGGER_PEN_KEY_ENABLE` gate（仅 pc/tablet/phone 定义，`BUILD.gn:251`）；其他产品须返回 `ERR_OPERATION_UNSUPPORTED`（17000005），不要移除该 gate。

**改前须确认（ask-before）**：
- 改 `frontend_api_defines.h` 的 `FrontendApiType` 枚举 / IPC JSON schema → 先确认 IPC 协议兼容
- 改 `frontend_error_defines.h` 错误码枚举 → 先检查全部调用方
- 改 `@ohos.UiTest.d.ts` 公共签名 → 向后兼容规则见根 `../AGENTS.md` § 项目约束

## 依赖

完整依赖列表见根 `../AGENTS.md` § 依赖 / `../deps.md`。

**uitest 关键依赖：**`hilog`、`nlohmann_json`、`accessibility`（UI 树访问）、`window_manager`、`input`（输入注入）、`ipc_core`、`image_framework`（截图）、`ace_napi`、`runtime_core:ani`。

## 相关文档

- 父框架：`../AGENTS.md`（根，含构建/部署/术语/跨组件约束）
- 使用指南：`../../../../docs/zh-cn/application-dev/application-test/uitest-guidelines.md`
- API 参考：`../../../../docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md`
- API 声明：`../../../../interface/sdk-js/api/@ohos.UiTest.d.ts`
