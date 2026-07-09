# PerfTest - AI 知识库

## 项目概述

本项目是 **PerfTest** —— 一个面向 OpenHarmony 应用性能测试的白盒性能测试框架。它提供性能测量能力，可在测试代码执行或特定 UI 场景下采集代码段执行时长、CPU 使用率/负载、内存消耗、列表滑动帧率以及应用启动/页面切换时延等指标。

框架同时支持 N-API（面向 ArkTS-Dynamic 应用）和 ANI（面向 ArkTS-Static 应用）接口。

## 知识路由

改动前按任务/路径/词汇先读对应章节；编辑前须声明 ① 任务属哪一行 ② 已读本文件对应章节 ③ 触发了哪些项目约束。

| 触发条件 | 主要文件（高频落点） | 先读本文件 | 必读根 AGENTS.md |
| --- | --- | --- | --- |
| 新增/改性能指标（PerfMetric） | `collection/include/data_collection.h`（`:29` PerfMetric 枚举） | § 新增性能指标 | — |
| 改 IPC 序列化格式 | `core/include/frontend_api_defines.h`（`:82` ApiCallInfo / `:89` ApiReplyInfo）、`connection/src/api_caller_client.cpp`、`connection/src/api_caller_server.cpp`、`connection/src/ipc_transactor.cpp` | § 修改 IPC 协议 + § 项目约束（IPC 兼容） | — |
| 改服务端接口注册 | `core/src/frontend_api_handler.cpp`（`:120` AddHandler、`:475-543` 注册点） | § 新增服务端接口 | — |
| 改测试执行生命周期 | `core/src/perf_test.cpp`、`core/src/perf_test_strategy.cpp` | § 核心组件 + § 能力调用流程 | — |
| 改 N-API 绑定 | `napi/src/perftest_napi.cpp`、`napi/src/callback_code_napi.cpp` | § 前端绑定 + § 项目约束（两绑定同步） | — |
| 改 ANI 绑定 / ArkTS API | `ani/src/perftest_ani.cpp`、`ani/ets/@ohos.test.PerfTest.ets` | § 前端绑定 + § 构建命令说明 (L170) + § 项目约束（两绑定同步） | — |
| 改 C++ core 后只改了一个绑定 | — | § 项目约束（两绑定，非三绑定） | — |
| 任务提到 `.abc` / ABC 重建 | `ani/ets/@ohos.test.PerfTest.ets` → `out/<product>/obj/.../@ohos.test.PerfTest.abc` | § 构建命令（L170 说明） + § 项目约束（禁止项） | — |
| 任务提到 `actionCode`/`resetCode` 回调 | `napi/src/callback_code_napi.cpp`、`ani/src/callback_code_ani.cpp`、`CodeCallbackContext` | § 回调线程安全 + § 项目约束 | — |
| 任务提到 `timeout` / 30s | `core/include/perf_test_strategy.h`（`:65` timeout_）、`napi/include/callback_code_napi.h`（`:43` timeout） | § 回调线程安全（超时保护） | — |
| 任务提到 HiSysEvent / HiTrace | `collection/src/app_start_time_collection.cpp`、`collection/src/list_swipe_fps_collection.cpp` | § 支持的指标 | — |
| 任务提到 testserver / SA 5502 / ACL | — | § 系统依赖 | testserver/AGENTS.md § 架构 |
| 改公共 `.d.ts` 签名 | `interface/sdk-js/api/@ohos.test.PerfTest.d.ts` | § 新增性能指标（第 6 步） + § 项目约束（改动前须确认） | — |

## 架构

### 三层架构

```
┌─────────────────────────────────────────────────────────────┐
│                     Frontend Layer                          │
│  ┌─────────────────┐              ┌─────────────────┐       │
│  │   N-API Client  │              │   ANI Client    │       │
│  │(perftest_client)│              │ (perftest_ani)  │       │
│  └────────┬────────┘              └────────┬────────┘       │
└───────────┼────────────────────────────────┼────────────────┘
            │                                │
            └───────────┬────────────────────┘
                        │ IPC Communication
┌───────────────────────┼──────────────────────────────────────┐
│                       │             Connection Layer         │
│  ┌────────────────────▼────────────────────┐                 │
│  │         IPC Transactor                  │                 │
│  │  - ApiCallerProxy (client side)         │                 │
│  │  - ApiCallerStub (server side)          │                 │
│  └────────────────────┬────────────────────┘                 │
└───────────────────────┼──────────────────────────────────────┘
                        │
┌───────────────────────┼──────────────────────────────────────┐
│                       │             Core Layer               │
│  ┌────────────────────▼────────────────────┐                 │
│  │              PerfTest                   │                 │
│  │  - PerfTestStrategy (test configuration)│                 │
│  │  - FrontendApiHandler (API management)  │                 │
│  └────────────────────┬────────────────────┘                 │
│                       │                                      │
│  ┌────────────────────▼────────────────────┐                 │
│  │         Data Collection Layer           │                 │
│  │  - DurationCollection                   │                 │
│  │  - CpuCollection                        │                 │
│  │  - MemoryCollection                     │                 │
│  │  - AppStartTimeCollection               │                 │
│  │  - PageSwitchTimeCollection             │                 │
│  │  - ListSwipeFpsCollection               │                 │
│  └─────────────────────────────────────────┘                 │
└──────────────────────────────────────────────────────────────┘
```

### 核心组件

**核心层**（`core/`）：
- `PerfTest`：主测试编排类，管理测试执行生命周期
- `PerfTestStrategy`：测试策略对象，定义待测指标、actionCode、resetCode、迭代次数、被测应用包名以及单代码段执行超时时间
- `FrontendApiHandler`：管理 API 注册与参数解析
- `common utils`：通用工具与 API 定义

**连接层**（`connection/`）：
- `IApiCaller`：管理客户端与服务端之间的 IPC 通信
- `ApiCallerProxy`：IPC 客户端，发起对服务端的调用；同时执行服务端崩溃检测，服务端退出后在客户端侧返回异常
- `ApiCallerStub`：IPC 服务端，接收客户端请求；同时执行客户端退出检测，客户端断开后服务端自动退出

**采集层**（`collection/`）：
- `DataCollection`：所有指标采集器的基类
- 各指标类型的专用采集器（CPU、内存、时长、帧率等）
- 通过 HiSysEvent 获取基于系统事件的指标
- 通过包名或 PID 进行进程监控

**前端绑定**：
- `napi/`：面向 ArkTS-Dynamic 接口的 N-API 绑定；`CallbackCodeNapi` 用于回调 ArkTS 代码段并等待完成
- `ani/`：面向 ArkTS-Static 接口的 ANI 绑定；`CallbackCodeAni` 用于回调 ArkTS 代码段并等待完成

**服务端守护进程**（`core/src/start_daemon.cpp`）：
- 以独立进程方式启动 PerfTest 的入口点
- 客户端连接时按需启动
- 所有客户端断开后自动停止

### 支持的指标

框架采集以下性能指标（定义于 `PerfMetric` 枚举）：

- `DURATION`：测试代码执行时长
- `CPU_LOAD`：系统 CPU 负载
- `CPU_USAGE`：进程 CPU 使用率百分比
- `MEMORY_RSS`：常驻集大小（进程实际占用的物理内存，包括私有内存和完整的共享库内存，共享库会被重复计算）
- `MEMORY_PSS`：比例集大小（按比例分配的内存大小，私有内存全额计入，共享内存按共享进程数均摊，更准确地反映进程实际内存使用量）
- `APP_START_RESPONSE_TIME`：应用启动响应时延
- `APP_START_COMPLETE_TIME`：应用启动完成时延
- `PAGE_SWITCH_COMPLETE_TIME`：页面/路由切换完成时延
- `LIST_SWIPE_FPS`：列表滚动时的帧率

### 能力调用流程

1. **客户端初始化**：前端（N-API/ANI）以 `PerfTestStrategy` 为参数创建 `PerfTest`
2. **连接**：客户端调用元能力提供的接口启动服务端，并使用连接令牌通过 IPC 连接到服务端守护进程
3. **测试执行**：服务端迭代测试轮次，执行 action 代码并采集指标
4. **回调管理**：通过回调机制执行 action 代码（JS/ArkTS 函数）
5. **结果采集**：各采集器在每轮迭代中采集数据
6. **结果获取**：客户端调用 `getMeasureResult()` 获取聚合统计结果
7. **清理**：释放资源，销毁回调

### 重要设计模式

**策略模式**：`PerfTestStrategy` 封装测试配置

**工厂模式**：`PerfTest::create()` 根据策略创建实例

**回调模式**：action/reset 代码作为回调从前端传递到后端

**模板方法**：`DataCollection` 基类定义采集生命周期

**代理模式**：`ApiCallerProxy` 提供透明的 IPC 通信

## 目录结构

```
perftest/
├── core/
│   ├── include/
│   │   ├── perf_test.h                  # 主测试编排类
│   │   ├── perf_test_strategy.h         # 策略配置（timeout_:65）
│   │   ├── frontend_api_defines.h       # IPC 序列化结构（ApiCallInfo:82/ApiReplyInfo:89）
│   │   └── frontend_api_handler.h       # API 注册与分发
│   └── src/
│       ├── perf_test.cpp                # 测试执行主逻辑
│       ├── perf_test_strategy.cpp       # 策略配置解析
│       ├── frontend_api_handler.cpp     # API 注册与分发（高频）
│       └── start_daemon.cpp             # 守护进程入口
├── collection/
│   ├── include/data_collection.h        # 采集器基类（PerfMetric 枚举:29）
│   └── src/
│       ├── duration_collection.cpp      # 代码段执行时长
│       ├── cpu_collection.cpp           # CPU 使用率/负载
│       ├── memory_collection.cpp        # RSS/PSS
│       ├── app_start_time_collection.cpp # 应用启动时延（HiSysEvent）
│       ├── page_switch_time_collection.cpp # 页面切换时延（HiSysEvent）
│       └── list_swipe_fps_collection.cpp  # 列表滑动帧率（HiTrace）
├── connection/
│   └── src/
│       ├── ipc_transactor.cpp           # IPC 序列化/反序列化
│       ├── api_caller_client.cpp        # 客户端连接管理
│       └── api_caller_server.cpp        # 服务端请求处理
├── napi/
│   ├── src/perftest_napi.cpp            # N-API 模块注册
│   ├── src/callback_code_napi.cpp       # ArkTS 回调处理（线程安全）
│   └── src/perftest_exporter.js         # JS 侧初始化
├── ani/
│   ├── ets/@ohos.test.PerfTest.ets      # ArkTS API 定义（改后须删旧 .abc）
│   └── src/
│       ├── perftest_ani.cpp             # ANI 原生方法绑定
│       └── callback_code_ani.cpp        # ArkTS 回调处理（线程安全）
└── test/unittest/
    ├── data_collection_test.cpp         # 采集器测试
    ├── ipc_transactor_test.cpp          # IPC 通信测试
    ├── frontend_api_handler_test.cpp    # API 注册与分发测试
    └── perf_test_test.cpp               # 测试编排测试
```

## 代码组织

### 按功能分类的源文件

**测试编排**：
- `core/src/perf_test.cpp`：主测试执行逻辑
- `core/src/perf_test_strategy.cpp`：策略配置解析
- `core/src/frontend_api_handler.cpp`：前端 API 注册与分发

**数据采集**：
- `collection/src/duration_collection.cpp`：获取代码段执行时长
- `collection/src/cpu_collection.cpp`：通过 `/proc/stat` 获取 CPU 指标（使用率、负载）
- `collection/src/memory_collection.cpp`：通过 `/proc/[pid]/status` 获取内存指标（PSS、RSS）
- `collection/src/app_start_time_collection.cpp`：通过 HiSysEvent 获取应用启动时延（响应时延、完成时延）
- `collection/src/page_switch_time_collection.cpp`：通过 HiSysEvent 获取页面切换时延（完成时延）
- `collection/src/list_swipe_fps_collection.cpp`：在代码段执行期间捕获并解析 HiTrace，计算列表滑动帧率

**IPC 通信**：
- `connection/src/ipc_transactor.cpp`：IPC 序列化/反序列化
- `connection/src/api_caller_client.cpp`：客户端连接管理
- `connection/src/api_caller_server.cpp`：服务端请求处理

**前端绑定**：
- `napi/src/perftest_napi.cpp`：N-API 模块注册与导出
- `napi/src/callback_code_napi.cpp`：ArkTS 代码段回调处理（调用用户定义的 actionCode、resetCode 并等待完成）
- `ani/src/perftest_ani.cpp`：ANI 原生方法绑定
- `ani/src/callback_code_ani.cpp`：ArkTS 代码段回调处理（调用用户定义的 actionCode、resetCode 并等待完成）
- `napi/src/perftest_exporter.js`：JS 侧初始化与守护进程启动
- `ani/ets/@ohos.test.PerfTest.ets`：ArkTS 侧 API 定义

**服务端守护进程**：
- `core/src/start_daemon.cpp`：守护进程入口点，提供启动 perftest-daemon 的命令

### 高频/高风险改动路径

- `core/src/frontend_api_handler.cpp`（API 注册与参数分发，新增接口高频落点）
- `ani/ets/@ohos.test.PerfTest.ets`（ArkTS API 定义，改后须删旧 `.abc` 重建，见 § 构建命令 L170）
- `core/include/frontend_api_defines.h`（`ApiCallInfo` `:82`/`ApiReplyInfo` `:89` 序列化结构，改影响 IPC 兼容）
- `core/src/perf_test.cpp`（测试执行主逻辑，生命周期改动高风险）
- `napi/src/callback_code_napi.cpp` + `ani/src/callback_code_ani.cpp`（回调机制，须成对改动）

## 构建系统

本项目使用 **GN（Generate Ninja）** 作为构建系统，集成于 OpenHarmony 构建体系。

### 构建命令

```bash
# 构建所有 PerfTest 组件（core、IPC、client、server、ANI）
./build.sh --product-name <product> --build-target perftestkit

# 构建单个组件
./build.sh --product-name <product> --build-target perftest_server     # 可执行守护进程（perftest）
./build.sh --product-name <product> --build-target perftest_client     # N-API 共享库（libperftest.z.so）
./build.sh --product-name <product> --build-target perftest_ani        # ANI 共享库（libperftest_ani.so）
./build.sh --product-name <product> --build-target perftest_abc        # ABC 模块（@ohos.test.PerfTest.abc）

# 构建单元测试
./build.sh --product-name <product> --build-target perftest_unittest
```

**注意：** 若修改了 `ani/ets/@ohos.test.PerfTest.ets` 文件，需在重新构建前删除 `out/<product>/obj/test/testfwk/arkxtest/perftest/@ohos.test.PerfTest.abc`，否则更改不会生效。

### 构建产物位置

- 可执行守护进程（perftest）：`out/<product>/testfwk/arkxtest/perftest`
- N-API 共享库（libperftest.z.so）：`out/<product>/testfwk/arkxtest/libperftest.z.so`
- ANI 共享库（libperftest_ani.so）：`out/<product>/testfwk/arkxtest/libperftest_ani.so`
- ABC 模块（@ohos.test.PerfTest.abc）：`out/<product>/obj/test/testfwk/arkxtest/perftest/@ohos.test.PerfTest.abc`

- 单元测试：`out/<product>/tests/unittest/arkxtest/perftest/perftest_unittest.txt`

### 设备安装位置

- 服务端守护进程：`/system/bin/perftest`
- N-API 客户端：`/system/lib/module/test/libperftest.z.so`
- ANI 库：`/system/lib/libperftest_ani.so`
- ABC 模块：`/system/framework/@ohos.test.PerfTest.abc`

### 测试命令

```bash
# 在设备上运行单元测试
hdc file send perftest_unittest /data/local/tmp/
hdc shell chmod +x /data/local/tmp/perftest_unittest
hdc shell /data/local/tmp/perftest_unittest

# 使用指定测试过滤器运行
hdc shell /data/local/tmp/perftest_unittest --gtest_filter=PerfTestTest.*
```

### 测试结果解读

单元测试使用 Google Test 框架输出：
- `[  PASSED  ]`：测试通过
- `[  FAILED  ]`：测试失败
- `[  RUN     ]`：测试正在运行
- 运行时统计信息位于输出末尾

输出示例：
```
[==========] Running 10 tests from 2 test suites.
[----------] Global test environment set-up.
[----------] 5 tests from PerfTestTest
[ RUN      ] PerfTestTest.Create
[       OK ] PerfTestTest.Create (15 ms)
[ RUN      ] PerfTestTest.Execute
[       OK ] PerfTestTest.Execute (23 ms)
[----------] 5 tests from PerfTestTest (45 ms total)
[----------] 5 tests from DataCollectionTest
[ RUN      ] DataCollectionTest.CpuCollection
[       OK ] DataCollectionTest.CpuCollection (8 ms)
[----------] 5 tests from DataCollectionTest (38 ms total)
[----------] Global test environment tear-down
[==========] 10 tests from 2 test suites ran. (123 ms total)
[  PASSED  ] 10 tests.
```

## 验证

### 按改动类型的验证矩阵

| 改动类型 | 必跑构建目标 | 必跑 unittest | 备注 |
| --- | --- | --- | --- |
| C++ core（`perf_test`/`frontend_api_handler`/collection） | `perftestkit` | `perftest_unittest` | 推送+运行见 § 测试命令 |
| IPC 通信层（`ipc_transactor`/`api_caller_*`） | `perftestkit` | `perftest_unittest`（`--gtest_filter=IpcTransactorTest.*`） | — |
| N-API 绑定（`napi/`） | `perftestkit` | — | 须验证消费方应用 `aa test` |
| ANI 绑定 / `.ets`（`ani/`） | `perftestkit` + 删旧 `.abc`（见 L170） | — | 须验证消费方应用 `aa test` |
| 新增 PerfMetric | `perftestkit` | `perftest_unittest` | 须确认级联完整（枚举→collector→注册→.ets→.d.ts） |
| 改公共 `.d.ts` 签名 | `perftestkit` | — | 向后兼容：类中新增同名方法时，入参个数不同是兼容的；同名方法参数类型/顺序与已有范围非包含关系视为不兼容；类属性：可读写属性可选↔必选、只读属性必选变可选、删除成员，均为不兼容 |

### Done 定义

完成回复须含：
1. **改动清单**：改了哪些文件，对应上表哪一行；N-API 与 ANI 绑定是否成对改动（两绑定同步确认）
2. **构建证据**：`perftestkit`（或对应单组件目标）构建通过的输出摘录；涉及 `.ets` 改动的须确认已删旧 `.abc`
3. **运行证据**：`perftest_unittest` 推送+运行输出摘录；涉及绑定层的须附 `aa test` 应用测试输出
4. **约束确认**：触发了 § 项目约束 的哪些条（两绑定同步 / IPC 序列化兼容 / 回调 timeout / ABC 重建），是否遵守

### 无法 device-side 验证时的兜底

无设备或 CI 环境下无法跑 unittest/shell 时：
- **不得省略**构建验证（`perftestkit` 构建可在主机完成）
- 须在回复中**显式标注**："以下项未做 device-side 验证"并列出，说明阻塞原因
- 不得用"应当通过""预计正常"等措辞替代实际运行证据

## 常见开发任务

### 新增性能指标

1. 在 `collection/include/data_collection.h` 的 `PerfMetric` 枚举中添加新枚举值
2. 创建继承自 `DataCollection` 的新采集器类
3. 实现 `StartCollection()` 和 `StopCollectionAndGetResult()`
4. 在 `FrontendApiHandler` 的前端类定义中注册
5. 在 `ani/ets/@ohos.test.PerfTest.ets` 中添加 TypeScript 枚举
6. 在接口文档 `"../../../../interface/sdk-js/api/@ohos.test.PerfTest.d.ts"` 的 `PerfMetric` 接口中添加新的指标枚举值定义

### 修改 IPC 协议

1. 更新 `core/include/frontend_api_defines.h` 中的 `ApiCallInfo` 和 `ApiReplyInfo` 结构体
2. 修改 `connection/src/api_caller_client.cpp` 中的序列化逻辑
3. 修改 `connection/src/api_caller_server.cpp` 中的反序列化逻辑
4. 更新 `connection/src/ipc_transactor.cpp` 中的 `OnRemoteRequest()`

### 新增服务端接口

1. 在 `core/include/perf_test.h` 和 `.cpp` 文件中或其他新建的 `.h/.cpp` 文件中添加接口处理逻辑
2. 在 `core/src/frontend_api_handler.cpp` 中添加静态方法处理接口调用，通过 `AddHandler()` 方法注册接口

### 单元测试

单元测试使用 Google Test 框架。测试文件位于 `test/unittest/`：

- `data_collection_test.cpp`：数据采集器测试
- `ipc_transactor_test.cpp`：IPC 通信测试
- `frontend_api_handler_test.cpp`：API 注册与分发测试
- `perf_test_test.cpp`：测试编排测试

## 项目约束

根目录 `../AGENTS.md` § 项目约束 中的跨组件约束同样适用。perftest 专属约束：

**禁止项**：
- 编辑 `ani/ets/@ohos.test.PerfTest.ets` 后，未在重新构建前删除过时的 `out/<product>/obj/test/testfwk/arkxtest/perftest/@ohos.test.PerfTest.abc` —— 更改将不会生效（见 § 构建命令，L170 处说明）。
- 修改 perftest C++ core 后仅同步了一个绑定。perftest 有**两个**绑定（非三个）：N-API（`napi/`，ArkTS-Dynamic）+ ANI（`ani/`，ArkTS-Static）—— 不存在仓颉绑定。两者均须与 core 保持同步。
- 修改 ArkTS 回调机制（`actionCode`/`resetCode`、`CodeCallbackContext` 线程同步）时，未保留 30 秒默认超时 / `PerfTestStrategy::timeout` 可配置行为（`core/include/perf_test_strategy.h:65`）。
- 移除或削弱 `CodeCallbackContext` 中的 `std::condition_variable` + `ThreadLock` 同步机制（`napi/include/callback_code_napi.h:39`、`ani/include/callback_code_ani.h:37`）—— 跨线程回调通知必须保持阻塞（见 § 回调线程安全实现细节）。回调通过 `napi_call_threadsafe_function` / `ani_call_threadsafe_function` 以 `napi_tsblocking` 模式在客户端（ArkTS 主）线程上执行；服务端线程必须 `wait()` 直到被通知。

**改动前须确认**：
- 改 `@ohos.test.PerfTest.d.ts` 公共签名 → 类中新增同名方法时，入参个数不同是兼容的；同名方法参数类型/顺序与已有范围非包含关系视为不兼容；类属性：可读写属性可选↔必选、只读属性必选变可选、删除成员，均为不兼容。
- 改 IPC 序列化格式（`core/include/frontend_api_defines.h` 中的 `ApiCallInfo`/`ApiReplyInfo`）→ 影响客户端/服务端兼容性。
- 新增 `PerfMetric` 枚举值 → 须级联到：采集器类、`FrontendApiHandler` 注册、`@ohos.test.PerfTest.ets` 枚举以及 `@ohos.test.PerfTest.d.ts`（见 § 新增性能指标）。

## 重要说明

### ArkTS 回调方法处理

框架使用复杂的回调机制从 C++ 后端执行 JS/ArkTS 代码：
- `actionCode`：用户定义的待执行测试代码段
- `resetCode`：可选的迭代间环境重置代码段
- 通过 `CodeCallbackContext` 管理回调，带线程同步
- 同时支持 N-API 和 ANI 回调

### 线程安全

- 服务端守护进程运行在独立进程中
- IPC 调用是同步的，但可能阻塞
- 回调执行需要谨慎的线程同步
- 使用 `ThreadLock` 等待异步回调完成

### 回调线程安全实现细节

框架使用以下机制确保跨线程回调调用的安全性：

**CodeCallbackContext 同步原语**：
- 使用 `std::condition_variable` 和 `std::mutex` 进行线程同步
- `ThreadLock` 类封装等待/通知逻辑，防止死锁

**N-API 线程安全调用**：
```cpp
// 跨线程安全的 N-API 回调调用
napi_call_threadsafe_function(napi_threadsafe_function func, void* data, napi_threadsafe_function_call_mode mode)
```
- `mode` 参数控制调用行为（阻塞/非阻塞）
- 使用 `napi_tsblocking` 模式等待回调完成

**ANI 线程安全调用**：
```cpp
// 跨线程安全的 ANI 回调调用
ani_call_threadsafe_function(ani_threadsafe_function func, void* data, ani_threadsafe_function_call_mode mode)
```
- 与 N-API 类似的机制，适配 ArkTS-Static 场景

**超时保护**：
- 默认等待时间：30 秒
- 可通过 `PerfTestStrategy::timeout` 配置
- 超时后返回错误，避免无限等待

**线程模型**：
```
┌─────────────────┐         ┌─────────────────┐
│  Client Thread  │         │  Server Thread  │
│  (ArkTS Main)   │         │  (Daemon)       │
└────────┬────────┘         └────────┬────────┘
         │                           │
         │  1. Send actionCode       │
         │──────────────────────────>│
         │                           │
         │  2. Wait (condition_var)  │
         │<──────────────────────────│
         │                           │
         │  3. Execute callback      │
         │  (threadsafe_function)    │
         │                           │
         │  4. Notify completion     │
         │──────────────────────────>│
         │                           │
         │  5. Return result         │
         │<──────────────────────────│
```

### 错误处理

所有函数使用 `ApiCallErr` 结构体进行错误报告：
- `errorCode`：数值型错误码
- `errorMessage`：字符串型错误描述

### 系统依赖

- 需要 HiSysEvent 服务获取应用启动/页面切换事件
- 使用 `/proc` 文件系统获取 CPU/内存指标
- 依赖 testserver SA（5502）调用高权限验权接口（PerfTest 进程无法直接申请 ACL 权限）
- 需要开发者模式以访问进程
