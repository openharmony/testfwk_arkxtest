# Arkxtest - AI 知识库

## 项目定位

本仓库对应 OpenHarmony `test/testfwk/arkxtest`，是自动化测试框架。优先按这些目录定位问题：

- `jsunit/`：Hypium 单元测试框架（`@ohos/hypium`），ArkTS-Dynamic（`src/`）+ ArkTS-Static（`src_static/`）双树。
- `uitest/`：UI 自动化测试框架，客户端-服务端架构，C++ core + ANI/NAPI/CJ 三套绑定。
- `perftest/`：白盒性能测试框架，N-API/ANI 前端 + IPC + core。
- `testserver/`：系统测试 SA（ID 5502），提供高权限接口。
- `testhelper/`：C++ CLI（`/system/bin/testhelper`），委托 testserver SA 提供系统能力。

框架为编写测试用例、执行测试、生成报告提供 API，用于 OpenHarmony 应用和系统接口测试。**本框架是测试框架本身，不是被测应用。**

## 仓库结构

arkxtest 位于 OpenHarmony 仓库内：

```
OpenHarmony/
├── build.sh              # 构建脚本（从此处执行）
├── test/xts/acts/        # XTS 测试套
├── interface/sdk-js/api/ # API 声明
└── test/testfwk/
    └── arkxtest/         # ← 本仓库
        ├── jsunit/
        ├── uitest/
        ├── perftest/
        ├── testserver/
        └── testhelper/
```

**所有构建命令从 OpenHarmony 根目录执行**，不在本子目录执行。

### 详细目录结构

```
arkxtest/
├── jsunit/              # 单元测试框架（Hypium）
│   ├── src/           # ArkTS-Dynamic 实现
│   │   ├── module/    # 核心模块（assert, mock, report, config）
│   │   ├── core.js    # 核心引擎
│   │   └── service.js # 测试服务
│   ├── src_static/    # ArkTS-Static 实现
│   └── package.json     # 包配置（@ohos/hypium，无 scripts/dependencies）
│
├── uitest/             # UI 自动化测试框架
│   ├── core/            # C++ 核心逻辑（UiDriver, WidgetSelector, WidgetOperator）
│   ├── server/          # 服务端守护进程（system_ui_controller）
│   ├── connection/      # IPC 通信层
│   ├── input/           # 输入注入（touch, keyboard, mouse）
│   ├── record/          # UI 录制/回放
│   ├── addon/           # 扩展（截图, 自定义扩展）
│   ├── ets/ani/         # ArkTS-Static 绑定（ANI）
│   ├── napi/            # ArkTS-Dynamic 绑定（N-API）
│   ├── cj/              # Cangjie FFI 绑定
│   └── test/            # C++ 单元测试（gtest）
│
├── perftest/           # 性能测试框架
│   ├── core/            # 核心测试逻辑
│   ├── collection/      # 性能数据采集
│   ├── connection/      # IPC 通信
│   ├── napi/            # N-API 绑定（ArkTS-Dynamic）
│   └── ani/             # ANI 绑定（ArkTS-Static）
│
├── testserver/         # 中心测试服务（IPC 枢纽, SA ID: 5502）
│   ├── src/            # 服务实现（ITestServerInterface.idl）
│   ├── init/           # 服务初始化（testserver.cfg）
│   └── sa_profile/     # 服务配置（5502.json）
│
├── testhelper/         # C++ CLI 工具，委托 TestServer
│   ├── include/        # 头文件（testhelper_core.h, common_utils.h, gpx_parser.h）
│   ├── src/            # 命令处理器, GPX 解析器, 工具函数
│   └── test/unittest/  # gtest 单元测试
│
└── hamock/             # Mock 框架（jsunit 的一部分）
```

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

```sh
# 各子组件构建目标
./build.sh --product-name rk3568 --build-target uitestkit              # uitest 全组件（client + server）
./build.sh --product-name rk3568 --build-target perftestkit            # perftest
./build.sh --product-name rk3568 --build-target testhelperkit          # testhelper CLI（二进制 + 核心库）
./build.sh --product-name rk3568 --build-target test_server_service    # testserver SA 守护进程
./build.sh --product-name rk3568 --build-target test_server_client     # testserver 客户端库
```

### 关键构建目标

| Target | 层级 | 说明 |
|--------|------|------|
| `uitestkit` | 主构建组 | uitest 全部组件（client + server） |
| `uitest_client` | 子组件 | uitest NAPI 客户端库（libuitest.z.so） |
| `uitest_server` | 子组件 | uitest 服务端可执行文件（安装为 `/system/bin/uitest`） |
| `uitest_ani` | 子组件 | uitest ANI 绑定（libuitest_ani.so，ArkTS-Static） |
| `cj_ui_test_ffi` | 子组件 | CJ FFI 绑定（libcj_ui_test_ffi.z.so） |
| `perftestkit` | 主构建组 | perftest 全部组件 |
| `testhelperkit` | 主构建组 | TestHelper 全部组件（二进制 + 核心库） |
| `testhelper` | 子组件 | TestHelper 可执行文件（`/system/bin/testhelper`） |
| `testhelper_unittest` | 单元测试 | TestHelper gtest 单元测试二进制 |
| `test_server_service` | 子组件 | testserver 守护进程（SA ID: 5502） |
| `test_server_client` | 子组件 | testserver 客户端库 |

### 构建配置

- **构建系统**：GN（Generate Ninja）+ BUILD.gn 文件
- **产品特性**：通过 `arkxtest_product_feature` 条件编译
  - `tablet` → `ARKXTEST_TABLET_FEATURE_ENABLE`
  - `pc` → `ARKXTEST_PC_FEATURE_ENABLE`
  - `watch` → `ARKXTEST_WATCH_FEATURE_ENABLE`
  - `phone/tablet` → `ARKXTEST_KNUCKLE_ACTION_ENABLE`

### 框架单元测试（C++ TDD）

```bash
# 构建测试
./build.sh --product-name rk3568 --build-target uitestkit_test            # uitest 全部单元测试
./build.sh --product-name rk3568 --build-target uitest_core_unittest      # uitest 单目标：核心逻辑
./build.sh --product-name rk3568 --build-target uitest_ipc_unittest       # uitest 单目标：IPC 通信
./build.sh --product-name rk3568 --build-target uitest_extension_unittest # uitest 单目标：扩展执行器
./build.sh --product-name rk3568 --build-target testserver_unittest
./build.sh --product-name rk3568 --build-target perftest_unittest
./build.sh --product-name rk3568 --build-target testhelper_unittest

# 推送到设备并运行
hdc file send out/rk3568/tests/unittest/arkxtest/uitest/uitest_core_unittest /data/local/tmp/
hdc shell ./uitest_core_unittest
# 带过滤器运行单个用例
hdc shell ./uitest_core_unittest --gtest_filter=WidgetSelectorTest.*
hdc shell /data/local/tmp/testhelper_unittest --gtest_filter=ParseTimeToMsTest.ValidTime_Normal
```

**uitest 单元测试目标说明**：
- `uitest_core_unittest`：核心逻辑（WidgetSelector 组件匹配、WidgetOperator 组件操作、UiDriver 驱动、UiModel 数据模型等）
- `uitest_ipc_unittest`：IPC 通信
- `uitest_extension_unittest`：扩展执行器

测试文件位于：`uitest/test/`、`testserver/test/`、`perftest/test/`、`testhelper/test/`。新增 uitest 测试须在 `uitest/BUILD.gn` 的 source list 注册（`testonly = true`，`module_out_path = "arkxtest/uitest"`），每个 `ohos_unittest` 目标对应一个 `<classname>_test.cpp`。

### XTS 测试（ArkTS）

**注意**：XTS 测试套构建命令也从 OpenHarmony 源码根目录执行。

```bash
# 构建 XTS 测试套
# ArkTS-Dynamic
./test/xts/acts/build.sh product-name <product> system_size=standard suite=<testsuite_path>:<target>
# ArkTS-Static
./test/xts/acts/build.sh product-name <product> system_size=standard xts_suitetype=hap_static suite=<testsuite_path>:<target>

# 安装 HAP
hdc install out/rk3568/suites/haps/<target>.hap

# 运行
# ArkTS-Dynamic
hdc shell aa test -p <bundleName> -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <className>
# ArkTS-Static
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <className>
```

### 验证要求

涉及真实窗口、显示、输入注入、SA 启动、Shell 命令（uitest/testhelper）的改动，需要补充 device-side 证据：构建 + 推送 + 运行对应 unittest 或 Shell 命令验证。

jsunit 无独立构建/lint，验证方式是消费方测试应用能构建并运行。

提交使用 `git commit -s`，并保留 `Co-Authored-By: Agent`。

### Done 定义

完成回复须含：
1. **改动清单**：改了哪些文件，属哪个子组件
2. **构建证据**：对应构建目标通过的输出摘录（命令见 § 构建和验证）
3. **运行证据**：对应 unittest / `aa test` / Shell 命令运行输出摘录；涉及 device-side 场景的须附推送+运行证据
4. **约束确认**：触发了 § 项目约束 的哪些条（.d.ts 兼容 / feature flag 范围），是否遵守

### 无法 device-side 验证时的兜底

无设备或 CI 环境下无法跑 unittest/shell 时：
- **不得省略**构建验证（构建可在主机完成）
- 须在回复中**显式标注**："以下项未做 device-side 验证"并列出，说明阻塞原因
- 不得用"应当通过""预计正常"等措辞替代实际运行证据

## 子组件简介

各子组件的详细架构、特性清单、API 工作流均在下级 `AGENTS.md` 中，本节只给一句话定位 + 改动前必读的关键约束提示：

| 组件 | 定位 | 详细文档 | 改动前必读约束提示 |
| --- | --- | --- | --- |
| **jsunit** (Hypium) | JS/ArkTS 单元测试框架，`@ohos/hypium`，断言/Mock/数据驱动 | `jsunit/AGENTS.md` | `src/`(.js) 与 `src_static/`(.ets) **双树对等**，改一侧必须同步另一侧 |
| **uitest** | UI 自动化测试框架，客户端-服务端架构，NAPI/ANI/CJ 三套绑定 | `uitest/AGENTS.md` | 改 C++ core 须同步 ANI(`ets/ani/`)+NAPI(`napi/`)+CJ(`cj/`)+`.d.ts`；勿跳 `CheckPointDisplayId` 跨屏校验、勿拆 `dumpMtx` 串行化 |
| **perftest** | 白盒性能测试框架，采集 DURATION/CPU/MEMORY/FPS 等指标 | `perftest/AGENTS.md` | 改 `ani/ets/@ohos.test.PerfTest.ets` 后须删旧 `.abc` 重建（见 L170） |
| **testserver** | 测试 SA（ID 5502），高权限系统能力服务，按需启停 | `testserver/AGENTS.md` | 禁改 `gen/` 下 IDL 自动生成代码；改 IPC 方法须四步走（IDL→service→client→test） |
| **testhelper** | C++ CLI（`/system/bin/testhelper`），委托 TestServer 提供时间/剪贴板/字体/位置模拟等命令 | `testhelper/AGENTS.md` | 所有输入校验须在服务调用前完成；`watch`/`glasses` 产品特性上禁用 |

完整的跨组件约束见下文 [§ 项目约束](#项目约束)，各子模块专属约束见对应下级 `AGENTS.md` § 项目约束。

## 知识路由

背景知识放在各子目录 AGENTS.md。改动前按场景读取对应文件：

改动前须声明 ① 任务属哪一行 ② 已读对应下级 AGENTS.md 哪些章节 ③ 触发了哪些项目约束。

| 场景 | 先读 |
| --- | --- |
| 添加 uitest 新 API（Driver/On/Component/UiWindow） | `uitest/AGENTS.md` § 新增 API |
| 添加 testhelper CLI 命令 | `testhelper/AGENTS.md` § 新增命令 |
| 添加 TestServer IPC 方法（IDL） | `testserver/AGENTS.md` § Adding New IPC Methods |
| 添加性能指标（PerfMetric） | `perftest/AGENTS.md` § Adding New Performance Metrics |
| 修改断言/Mock/TestRunner/Tag 过滤 | `jsunit/AGENTS.md` § Key APIs / § TestRunner |
| AAMS（无障碍系统服务）、多用户 UI 树访问 | `uitest/AGENTS.md` § 多屏多用户；AAMS 本体见 `/foundation/barrierfree/accessibility/AGENTS.md` |
| 跨屏坐标操作约束（swipe/drag/mouseDrag） | `uitest/AGENTS.md` § Display ID 传播与多显示器操作（跨屏约束表） |
| 文件描述符安全（fdsan）、GPX 解析、输入校验 | `testhelper/AGENTS.md` § Important Context / § Project Constraints |
| SA 5502 生命周期、权限、ACL、testserver.cfg | `testserver/AGENTS.md` § Architecture / § Configuration Files / § Project Constraints |
| ABC 模块重建（perftest `.ets` 改动后） | `perftest/AGENTS.md` L170 / § Project Constraints |
| NAPI/ANI/CJ 多语言绑定同步 | `uitest/AGENTS.md` § 新增 API（第 3 步）/ § 项目约束 |
| device-side 部署、hdc 命令验证 | 根 § 构建和验证；各子 `AGENTS.md` 的部署/运行小节 |

## 术语速查

OpenHarmony 特有缩写（通用术语如 IPC/API/CPU 不收）。释义只解释术语本身，不描述在本仓库的使用场景；下级 AGENTS.md 不再复述这些缩写定义：

| 缩写 | 释义 |
| --- | --- |
| AAMS | AccessibilityAbilityManagerService，无障碍系统服务（本体在 `/foundation/barrierfree/accessibility`） |
| ABC | Ark Bytecode，ArkTS 编译产物字节码（`.abc`） |
| ANI | ArkTS Native Interface，ArkTS-Static 语言的原生（native）调用接口 |
| NAPI | Node-API，ArkTS-Dynamic 语言的原生（native）调用接口（C/C++ 与 JS 互操作标准） |
| FFI | Foreign Function Interface，跨语言函数调用接口（Cangjie 侧使用） |
| SA | System Ability，系统能力 |
| samgr | System Ability Manager，系统能力管理器（SA 的注册与发现） |
| CommonEvent | 公共事件服务（Common Event Service） |
| HiSysEvent | 系统事件打点与查询服务 |
| HiTrace | 分布式链路追踪框架 |
| HiLog | 系统日志服务 |
| HAP | Harmony Ability Package，应用安装包（`.hap`） |
| HDC | HarmonyOS Device Connector，设备通信与调试工具 |
| OHPM | OH Package Manager，OpenHarmony 包管理器 |
| XTS | X Test Suite，OpenHarmony 兼容性测试套 |
| GPX | GPS Exchange Format，GPS 数据交换格式（XML） |
| IME/IMF | Input Method Editor / Input Method Framework，输入法编辑器/输入法框架 |
| socperf | SoC Performance，SoC 性能调度服务 |
| SmartPerf/SpDaemon | 性能调优工具及其守护进程 |
| ACL | Access Control List，访问控制列表 |
| fdsan | file descriptor sanitizer，文件描述符安全检查机制 |
| ARKXTEST_*_ENABLE | 产品特性标志（feature flag），条件编译开关 |
| 板侧 / device-side | 设备侧（在真实设备上执行），区别于主机侧（host）模拟环境 |

## 项目约束

各子模块的专属约束（禁改项、不变量、输入校验前置等）分别在对应下级 `AGENTS.md` 的 § 项目约束 中声明。以下仅列出**跨组件通用约束**：

- 改公共 API `.d.ts` 签名前确认向后兼容：类中新增同名方法时，入参个数不同是兼容的；同名方法参数类型/顺序与已有范围非包含关系视为不兼容。类属性修改：可读写属性可选变必选、可读写属性必选变可选、只读属性必选变可选、删除成员，均为不兼容变更。涉及 `@ohos.UiTest.d.ts`（uitest）、`@ohos.test.PerfTest.d.ts`（perftest）。
- 改 `ARKXTEST_*_ENABLE` feature flag 前确认影响的产品范围（uitest 7 个、testhelper 2 个、testserver 2 个，影响面不同）。
- **禁止改变现有 API 的行为，任何功能扩展须通过新增接口（方法/参数）实现，保持向后兼容**。所有对外开放的接口（包括 `.d.ts` 声明和实现）均须保持行为不变。
- 同时改动多个子组件（如 uitest + testserver IPC、testhelper + testserver 错误码）时，须确认接口版本一致——客户端和服务端必须从相同源码构建，不得只推送一侧。

## 部署与故障排查

### 部署工作流

1. **构建组件**
   ```bash
   cd /path/to/openharmony
   ./build.sh --product-name rk3568 --build-target uitestkit
   ```

2. **验证构建产物**
   ```bash
   ls -lh out/rk3568/testfwk/arkxtest/
   ```
   预期输出位置：`out/<product>/testfwk/arkxtest/`（OpenHarmony 根目录下）

3. **部署到设备**
   ```bash
   # 挂载系统分区为读写
   hdc target mount
   hdc shell mount -o rw,remount /

    # 推送 uitest server 和 client
    hdc file send out/rk3568/testfwk/arkxtest/uitest /system/bin/uitest
    hdc file send out/rk3568/testfwk/arkxtest/libuitest.z.so /system/lib/module/libuitest.z.so
    hdc file send out/rk3568/testfwk/arkxtest/libuitest_ani.so /system/lib/libuitest_ani.so
    hdc shell chmod +x /system/bin/uitest

    # 推送 testhelper CLI（详细步骤见 testhelper/AGENTS.md）
    hdc file send out/rk3568/testfwk/arkxtest/testhelper /system/bin/testhelper
    hdc shell chmod +x /system/bin/testhelper
    ```

   **uitest 安装位置**：服务端可执行文件 `/system/bin/uitest`；NAPI 客户端库 `/system/lib/module/libuitest.z.so`；ANI 客户端库 `/system/lib/libuitest_ani.so`。

4. **验证服务状态**
   ```bash
   # 检查 uitest 守护进程能否启动
   hdc shell uitest start-daemon test@1234@1000@0

   # 检查 testserver SA（SA ID: 5502）
   hdc shell smgr list | grep 5502
   ```

### Shell 命令与 CLI 工具

两个 device-side CLI 二进制——每次 uitest/testhelper 改动后都要验证：

**1. uitest**（`/system/bin/uitest`）- UI 快捷操作
```bash
hdc shell uitest screenCap -p /data/local/tmp/screenshot.png     # 截图
hdc shell uitest dumpLayout -p /data/local/tmp/layout.json       # 导出 UI 树
hdc shell uitest uiInput click 500 1000                          # 注入点击
hdc shell uitest uiInput swipe 100 1000 900 1000 600             # 注入滑动
hdc shell uitest uiInput keyEvent Back                           # 注入按键
hdc shell uitest uiInput inputText 500 1000 "Hello"              # 注入文本
hdc shell uitest uiRecord record -W true -l                      # 录制 UI 操作
hdc shell uitest start-daemon test@1234@1000@0                   # 启动守护进程
```
完整命令列表见 `uitest/AGENTS.md` 的 "Shell Commands"。

**2. testhelper**（`/system/bin/testhelper`）- 委托 TestServer SA 5502 提供系统能力
```bash
hdc shell testhelper get-time
hdc shell testhelper set-time "2026-02-28 14:30:00"
hdc shell testhelper set-pastedata "text"
hdc shell testhelper set-viewmode dark
```
完整命令列表见 `testhelper/AGENTS.md` § Shell 命令。

### 故障排查

**构建问题**
- **GN not found**：确保 OpenHarmony 构建环境已正确配置 `./build.sh --product-name <product> --ccache --build-target gcc`
- **权限拒绝**：检查输出目录写权限
- **build.sh not found**：必须从 OpenHarmony 根目录执行构建命令，不是 arkxtest 目录

**设备部署问题**
- **只读文件系统**：使用 `hdc target mount` 和 `hdc shell mount -o rw,remount /`
- **system/ 权限拒绝**：部分设备需要额外步骤修改系统分区。确保已启用开发者模式。
- **HDC 连接失败**：用 `hdc list` 检查设备是否已连接

**服务未启动**
- **uitest 守护进程失败**：用 `hdc shell hilog | grep uitest` 检查日志
- **testserver SA 缺失**：确认 `/system/profile/testserver.json` 存在 5502.json，`/system/etc/init/testserver.cfg` 存在 testserver.cfg
- **SA 未注册**：用 `hdc shell smgr list` 检查 SA 5502 是否运行

**测试失败**
- **Widget 未找到**：用 `hdc shell uitest dumpLayout -p /data/local/tmp/layout.json` 检查 UI 层级
- **IPC 超时**：用 `hdc shell ps -ef | grep uitest` 检查服务端守护进程是否运行
- **库未找到**：确认所有共享库已推送到正确的系统目录

**常见路径问题**
- 构建产物在 `out/<product>/testfwk/arkxtest/`（OpenHarmony 根目录下）
- API 声明在 `interface/sdk-js/api/`（OpenHarmony 根目录下）
- 测试套在 `test/xts/acts/`（OpenHarmony 根目录下）

## 依赖

### 关键依赖（共 29 个）

| 依赖 | 用途 |
|------------|---------|
| `accessibility` | UI 组件树访问（uitest core） |
| `window_manager` | 窗口管理 |
| `input` | 输入事件注入 |
| `image_framework` | 截图/图像处理 |
| `ipc` | 进程间通信 |
| `soc_perf` | 性能数据采集（perftest） |
| `napi` | ArkTS 运行时绑定 |
| `runtime_core` | ArkTS 运行时环境（ANI） |
| `hilog` | 日志 |
| `common_event_service` | 系统事件 |
| `safwk` / `samgr` | 系统能力框架（testserver） |
| `pasteboard` | 剪贴板操作 |
| `libxml2` | GPX XML 解析（testhelper） |
| `time_service` | 时间/时区服务（testhelper） |
| `graphic_2d` | 字体管理（testhelper，feature-gated） |
| `hiview` / `hisysevent` | 系统事件监控 |

## 资源占用
- **ROM**：500KB
- **RAM**：100KB

完整依赖列表见 `deps.md`。

## 重要上下文

1. **多语言支持**：所有 C++ 功能必须在 ANI（ArkTS-Static）和 NAPI（ArkTS-Dynamic）中都有绑定
2. **IPC 通信**：所有客户端操作通过 IPC 到服务端守护进程
3. **API 版本**：模块支持从 API 8 起，里程碑功能在 API 9、10、11、18、20、22、23、25
4. **系统能力**：需要 `SystemCapability.Test.UiTest` 和 `SystemCapability.Test.PerfTest`
5. **适配系统**：仅 Standard 系统类型
6. **测试框架**：这是测试框架本身，不是被测应用

## 子组件专属文档

- **JsUnit**：`jsunit/AGENTS.md` - Hypium 框架架构、断言/Mock API、测试执行
- **Uitest**：`uitest/AGENTS.md` - 详细 uitest 架构、API 工作流、Shell 命令
- **PerfTest**：`perftest/AGENTS.md` - 性能测试架构、指标、采集策略
- **TestServer**：`testserver/AGENTS.md` - 系统能力服务、IPC 接口、生命周期管理
- **TestHelper**：`testhelper/AGENTS.md` - CLI 工具架构、命令处理器、实现细节
- **依赖**：`deps.md` - 完整外部依赖列表

## 相关文档
**README**：
  - `README_zh.md` - 中文文档
  - `README_en.md` - 英文文档
  - `jsunit/README.md` - Hypium 框架文档
- **使用指南**：
  - `docs/zh-cn/application-dev/application-test/perftest-guideline.md` - PerfTest 使用指南
  - `docs/zh-cn/application-dev/application-test/uitest-guidelines.md` - UiTest 使用指南
  - `docs/zh-cn/application-dev/application-test/unittest-guidelines.md` - 单元测试框架使用指南
- **API 参考**：
  - `docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md` - UiTest API 参考
  - `docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-perftest.md` - PerfTest API 参考
- **API 声明**：
  - `interface/sdk-js/api/@ohos.UiTest.d.ts` - UiTest API 声明
  - `interface/sdk-js/api/@ohos.test.PerfTest.d.ts` - PerfTest API 声明
- `LICENSE` - Apache License 2.0
