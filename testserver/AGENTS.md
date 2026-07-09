# TestServer - AI 知识库

## 概述

testserver 是 OpenHarmony 测试框架的系统服务（SA ID: 5502），为测试工具提供高权限 IPC 接口（剪贴板、窗口管理、时间/时区、字体、位置模拟、性能采集等）。采用客户端-服务端架构，通过 OpenHarmony IDL 定义 IPC 接口，按需启停（首个客户端连接时启动，所有客户端断开或开发者模式关闭时自动停止）。

构建、部署、单元测试命令统一见根 `../AGENTS.md` § 构建和验证（含 `test_server_service`/`test_server_client`/`TestSA_unittest`/`testserver_fuzztest` 构建目标、产物路径、设备推送）。

## 知识路由

改动前按任务/路径/词汇先读对应章节；编辑前须声明 ① 任务属哪一行 ② 已读本文件对应章节 ③ 触发了哪些项目约束。

| 触发条件 | 主要文件 | 先读本文件 |
| --- | --- | --- |
| 新增 IPC 方法 | `src/ITestServerInterface.idl`、`src/Types.idl` | § 新增 IPC 方法 |
| 改服务端逻辑 | `src/service/test_server_service.cpp` | § 架构 + § 项目约束 |
| 改客户端封装 | `src/client/test_server_client.cpp` | § 架构 |
| 改错误码 | `src/utils/test_server_error_code.h` | § 项目约束（ask-before） |
| 改条件编译/产品特性 | `src/BUILD.gn` | § 产品特性 + § 项目约束 |
| 改 SA 权限/生命周期 | `init/testserver.cfg`、`sa_profile/5502.json` | § 项目约束（ask-before） |
| 加单元测试 | `test/unittest/test_server_service_test.cpp` | § 测试 |
| 任务提到 `CallerDetectTimer` / 按需停止 / 调用者追踪 | `src/service/test_server_service.cpp`（`CallerDetectTimer`、`callerCount_`） | § 架构（服务生命周期） + § 项目约束（ask-before: 按需停止条件） |
| 任务提到 `CreateSession` / `OnRemoteDied` / 死亡回调 | `src/service/test_server_service.cpp`、`src/client/session_token.h` | § 架构（调用者追踪） + § 项目约束（禁止: 绕过 CreateSession） |
| 任务提到 `IDFuzzer` / 模糊测试 | `test/fuzztest/` | — |

## 代码风格

### C++ 代码风格

- **头文件保护**：`#ifndef FILE_NAME_H` / `#define` / `#endif`——禁用 `#pragma once`。每个文件须有 Apache 2.0 许可证头。
- **命名空间**：`namespace OHOS::testserver { ... } // namespace OHOS::testserver`。函数体 **4 空格缩进**。
- **命名**：类/函数 PascalCase（`TestServerService`、`SetPasteData`）；成员变量 camelCase 下划线后缀（`callerCount_`、`iTestServerInterface_`）；`constexpr` 常量 UPPER_CASE 或 camelCase（`CALLER_DETECT_DURING`、`TESTSERVER_LOAD_TIMEOUT_MS`）。
- **头文件包含**：项目头文件用 `"quotes"`（`"test_server_service.h"`）；系统/OpenHarmony 头文件用 `"quotes"` 或 `<angle>`（`"hilog/log.h"`、`<common_event_manager.h>`）。条件包含用 `#ifdef ARKXTEST_*_ENABLE` 保护。
- **格式**：开括号换行（类/函数/控制流）；`*` 和 `&` 靠类型侧（`const std::string& text`）；`using namespace` 仅在函数/文件作用域，不在头文件中。
- **智能指针**：IPC 引用计数对象使用 OHOS `sptr`/`wptr`；普通资源用 `shared_ptr`/`unique_ptr`。

### 错误处理与日志

- **IPC 返回值模式**：服务端方法返回 `ErrCode`（IPC 层始终返回 `TEST_SERVER_OK`），逻辑结果通过出参传递：
  ```cpp
  ErrCode SetPasteData(const std::string& text, int32_t& setResult)
  {
      // 成功：setResult = TEST_SERVER_OK; return TEST_SERVER_OK;
      // 失败：setResult = TEST_SERVER_SET_PASTE_DATA_FAILED; return TEST_SERVER_OK;
  }
  ```
- **客户端**：先检查 `iTestServerInterface_ == nullptr`，返回 `TEST_SERVER_GET_INTERFACE_FAILED`。
- **错误码**：统一使用 `test_server_error_code.h` 中 `TEST_SERVER_` 前缀的枚举值，禁止使用原始整数。常用错误码：

| 错误码 | 名称 | 描述 |
|--------|------|------|
| 0 | `TEST_SERVER_OK` | 操作成功 |
| -1 | `TEST_SERVER_GET_INTERFACE_FAILED` | 获取 SA 接口失败 |
| 19000001 | `TEST_SERVER_ADD_DEATH_RECIPIENT_FAILED` | 添加死亡接收器失败 |
| 19000002 | `TEST_SERVER_CREATE_PASTE_DATA_FAILED` | 创建剪贴板数据失败 |
| 19000003 | `TEST_SERVER_SET_PASTE_DATA_FAILED` | 设置剪贴板数据失败 |
| 19000004 | `TEST_SERVER_PUBLISH_EVENT_FAILED` | 发布公共事件失败 |
| 19000005 | `TEST_SERVER_SPDAEMON_PROCESS_FAILED` | SmartPerf 进程操作失败 |
| 19000006 | `TEST_SERVER_COLLECT_PROCESS_INFO_FAILED` | 采集进程信息失败 |
| 19000007 | `TEST_SERVER_OPERATE_WINDOW_FAILED` | 窗口操作失败 |
| 19000008 | `TEST_SERVER_DATASHARE_FAILED` | DataShare 查询失败 |
- **日志**：`HiLog::Debug/Info/Warn/Error(LABEL, format, ...)`，格式串使用 `%{public}s`、`%{public}d`。函数入口记录：`HiLog::Debug(LABEL, "%{public}s called.", __func__)`。HiLog 标签定义为 `static constexpr HiLogLabel`（`LABEL_SERVICE`、`LABEL_TIMER`、`LABEL`）。

### 测试

- **框架**：Google Test + Google Mock，使用 OpenHarmony `HWTEST_F` / `HWTEST` / `HWMTEST` 宏。
- **测试夹具**：`SetUp()` 调用 `TestServerMockPermission::MockProcess("testserver")`；`TearDown()` 卸载 SA。
- **Mock**：通过 `TestServerServiceMock` 的 `MOCK_METHOD0` mock `IsRootVersion()`/`IsDeveloperMode()`。
- **条件编译测试**：`#ifdef ARKXTEST_*` 覆盖功能启用/禁用两条路径。
- **测试命名**：类 `<Feature>Test`（`ServiceTest`、`ClientTest`）；方法 `test<描述>`（`testSetPasteData`）。
- **测试规模**：`TestSize.Level1`。新测试源文件须在 `test/unittest/BUILD.gn` 注册。

## 架构

### 客户端-服务端模型

```
测试工具（testhelper / uitest / perftest）
    ↓ 调用 TestServerClient
TestServerClient（libtest_server_client.z.so）
    ↓ IPC（OpenHarmony IDL Proxy/Stub）
TestServerService（SA 5502，libtest_server_service.z.so）
    ↓ 系统服务（窗口管理/剪贴板/时间/位置等）
被测系统能力
```

### 服务生命周期

1. **按需启动**：首个客户端通过 `LoadSystemAbility` 触发 `OnStart()`，校验 root 或开发者模式后 `Publish(this)` 注册服务。
2. **调用者追踪**：`CreateSession()` 注册客户端死亡回调并递增 `callerCount_`；客户端死亡时 `OnRemoteDied` 递减计数。
3. **自动停止**：`CallerDetectTimer`（10 秒超时）在 `callerCount_ == 0` 时调用 `UnloadSystemAbility`；开发者模式关闭时由 `stop-on-demand` 配置触发停止。

### 条件编译

功能按产品配置条件启用（详见 `src/BUILD.gn`）：

| 宏 | 条件 | 功能 |
|------|------|------|
| `ARKXTEST_PASTEBOARD_ENABLE` | `distributeddatamgr_pasteboard` 组件存在 | 剪贴板操作 |
| `ARKXTEST_KNUCKLE_ACTION_ENABLE` | `phone`/`tablet`/`pc` | DataShare 查询 |
| `ARKXTEST_IMF_ENABLE` | `inputmethod_imf` 组件存在 | 键盘隐藏 |
| `ARKXTEST_FONT_ENABLE` | `pc`/`tablet`/`phone` + `global_font_manager` | 字体安装/卸载 |
| `ARKXTEST_VIEW_MODE_ENABLE` | `arkui_ui_appearance` 组件存在 | 深色/浅色模式 |
| `ARKXTEST_LOCATION_ENABLE` | `location_location` 组件存在 | 位置模拟 |

## 目录结构

```
testserver/
├── src/
│   ├── ITestServerInterface.idl   # IPC 接口定义
│   ├── Types.idl                  # IPC 数据类型定义
│   ├── BUILD.gn                   # 构建配置（含条件编译）
│   ├── service/
│   │   ├── test_server_service.h  # 服务端声明
│   │   └── test_server_service.cpp # 服务端实现
│   ├── client/
│   │   ├── test_server_client.h   # 客户端声明
│   │   ├── test_server_client.cpp # 客户端实现
│   │   └── session_token.h        # 会话令牌（死亡回调）
│   └── utils/
│       └── test_server_error_code.h # 错误码定义
├── init/
│   └── testserver.cfg             # 服务启动配置（uid/gid/权限）
├── sa_profile/
│   └── 5502.json                  # SA 配置（按需停止策略）
└── test/
    ├── unittest/                  # gtest 单元测试
    │   ├── test_server_service_test.cpp
    │   ├── test_server_client_test.cpp
    │   ├── mock_permission.cpp/h
    │   └── start_test_server.cpp/h
    └── fuzztest/                  # 模糊测试
        ├── createsession_fuzzer/
        ├── loadtestserver_fuzzer/
        └── setpastedata_fuzzer/
```

### 高频/高风险改动路径

- `src/ITestServerInterface.idl`（IPC 接口定义，新增/改方法的高频落点，改动触发 proxy/stub 重新生成）
- `src/service/test_server_service.cpp`（服务端核心逻辑，所有 IPC 方法的服务端实现）
- `src/client/test_server_client.cpp`（客户端封装，所有 IPC 方法的客户端入口）
- `src/utils/test_server_error_code.h`（错误码定义，影响所有调用方的错误判断）
- `init/testserver.cfg` + `sa_profile/5502.json`（SA 权限/生命周期配置，改动影响服务启停和权限）

## 新增 IPC 方法

1. 在 `src/ITestServerInterface.idl` 添加方法签名（类型定义放 `Types.idl`）
2. 在 `src/service/test_server_service.h` 和 `.cpp` 实现（override stub）
3. 在 `src/client/test_server_client.h` 和 `.cpp` 添加客户端封装
4. 在 `src/utils/test_server_error_code.h` 添加错误码（如需）
5. 在 `test/unittest/test_server_service_test.cpp` 添加单元测试
6. 重新构建——IPC proxy/stub 从 IDL 自动生成

## 项目约束

根 `../AGENTS.md` § 项目约束 的跨组件通用约束同样适用。以下为 testserver 专属约束：

**禁止**：
- 编辑 `out/<product>/gen/test/testfwk/arkxtest/testserver/src/` 下的 IPC proxy/stub 自动生成代码——始终编辑 IDL 源文件（见 § 新增 IPC 方法）。
- 绕过 `CreateSession()` 调用者追踪——它是按需启停生命周期的基础，新入口点必须注册调用者。
- 禁止改变现有 API 的行为，任何功能扩展须通过新增接口（方法/参数）实现，保持向后兼容。

**改前须确认（ask-before）**：
- 改 `test_server_error_code.h` 错误码 → 先检查所有调用方，使用已有错误码而非原始整数。
- 改 `init/testserver.cfg` 的 uid/gid/capabilities 或 `5502.json` 的 ACL → 需与维护者确认（影响 SA 权限）。
- 改按需停止条件（`CallerDetectTimer` 超时 / 开发者模式门控） → 影响服务生命周期。
- 改 `ARKXTEST_*_ENABLE` feature flag → 先确认影响的产品范围。

## 验证

### 按改动类型的验证矩阵

| 改动类型 | 必跑构建目标 | 必跑 unittest | 必跑 device-side 验证 | 备注 |
| --- | --- | --- | --- | --- |
| IDL 接口（`ITestServerInterface.idl`/`Types.idl`） | `test_server_service` + `test_server_client` | `testserver_unittest` | `smgr list \| grep 5502` 确认 SA 注册 | proxy/stub 自动生成，须重新构建 |
| 服务端逻辑（`test_server_service.cpp`） | `test_server_service` | `testserver_unittest` | `hilog \| grep C03110` 检查日志 | 条件编译功能须验证启用/禁用两条路径 |
| 客户端封装（`test_server_client.cpp`） | `test_server_client` | `testserver_unittest` | — | 确认客户端与服务端从相同源码构建 |
| 错误码（`test_server_error_code.h`） | `test_server_service` + `test_server_client` | `testserver_unittest` | — | 检查所有调用方是否使用新错误码 |
| 条件编译（`BUILD.gn` feature flag） | `test_server_service` | `testserver_unittest` | 对应功能在目标产品上验证 | 确认受影响的产品范围 |
| SA 配置（`testserver.cfg`/`5502.json`） | `test_server_service` | — | `cat /system/etc/init/testserver.cfg` + `smgr list` | 推送后须重启设备或手动启动 SA |

### Done 定义

完成回复须含：
1. **改动清单**：改了哪些文件，对应上表哪一行
2. **构建证据**：`test_server_service` / `test_server_client`（或对应目标）构建通过的输出摘录
3. **运行证据**：`testserver_unittest` 推送+运行输出摘录；涉及 device-side 场景的须附 SA 启动/日志输出
4. **约束确认**：触发了 § 项目约束 的哪些条（生成代码边界 / 调用者追踪 / 错误码 / 生命周期），是否遵守

### 无法 device-side 验证时的兜底

无设备或 CI 环境下无法跑 unittest/shell 时：
- **不得省略**构建验证（`test_server_service` / `test_server_client` 构建可在主机完成）
- 须在回复中**显式标注**："以下项未做 device-side 验证"并列出，说明阻塞原因
- 不得用"应当通过""预计正常"等措辞替代实际运行证据

## 故障排查

### 服务未启动

**SA 未注册**：确认 5502.json 位于 `/system/profile/testserver.json`
```bash
cat /system/profile/testserver.json
```

**权限拒绝**：检查 testserver.cfg 是否有正确的 uid/gid
```bash
cat /system/etc/init/testserver.cfg
```

**检查服务日志**
```bash
hdc shell hilog | grep C03110
```

**验证开发者模式**
```bash
hdc shell param get const.security.developermode.state
```
应返回 `true`。如果不是，请在设置中启用开发者模式。

### 客户端连接问题

**访问拒绝**：确认 SA 配置文件中的 ACL 权限
```bash
cat /system/profile/testserver.json
```

**版本不匹配**：确保客户端和服务端从相同源码构建

### 常见问题

| 现象 | 可能原因 | 解决方案 |
|------|----------|----------|
| 返回 `TEST_SERVER_GET_INTERFACE_FAILED` (-1) | SA 未启动或注册失败 | 检查开发者模式是否已启用，查看 hilog 日志 |
| 返回 19000002/19000003（剪贴板相关） | pasteboard 服务不可用 | 检查 `ARKXTEST_PASTEBOARD_ENABLE` 条件编译配置 |
| 返回 19000007（窗口操作失败） | window manager 服务异常 | 确认 window_manager 服务正常运行 |
| 返回 19000008（DataShare 失败） | DataShare 服务不可用 | 检查产品配置是否支持 DataShare |
| samgr 未找到服务 | SA 配置文件缺失或错误 | 确认 5502.json 存在于 `/system/profile/testserver.json` |
| 权限拒绝错误 | testserver.cfg 的 uid/gid 错误 | 检查 `init/testserver.cfg` 配置 |

## 依赖

完整依赖列表见根 `../AGENTS.md` § 依赖 / `../deps.md`。

**testserver 关键依赖**：`safwk`（系统能力框架）、`ipc_core`（IPC）、`samgr`（SA 管理）、`window_manager`（窗口操作）、`common_event_service`（事件发布）、`hiview:libucollection_utility`（CPU/内存采集）、`soc_perf`（频率锁定）、`pasteboard`（剪贴板，可选）、`time_service`（时间/时区）、`data_share`（DataShare，可选）、`location`（位置模拟，可选）。

## 相关文档

- 父框架：`../AGENTS.md`（根，含构建/部署/术语/跨组件约束）
- testhelper（主要客户端）：`../testhelper/AGENTS.md`
