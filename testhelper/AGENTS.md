# TestHelper - AI 知识库

## 概述

**TestHelper** 是 arkxtest 的 C++ 命令行工具组件（`/system/bin/testhelper`），通过 `TestServerClient` 委托 testserver SA（ID 5502）暴露系统级测试能力。支持设置/获取系统时间和时区、剪贴板操作、输入法键盘隐藏、字体管理、深色/浅色模式切换，以及通过 GPX 文件进行 GPS 位置模拟。

## 快速开始

### 仓库位置

**重要：** testhelper 位于 arkxtest 框架内，属于 OpenHarmony 仓库：

```
OpenHarmony/
├── build.sh              # 构建脚本（从此处执行）
└── test/testfwk/
    └── arkxtest/
        └── testhelper/   # ← 当前位置
            ├── include/             # 头文件
            ├── src/                 # 源文件
            ├── test/unittest/       # gtest 单元测试
            └── BUILD.gn             # 构建配置
```

构建命令须从 OpenHarmony 根目录执行（见根 `../AGENTS.md` § 构建和验证）。

### 前置条件
- 已配置构建环境的 OpenHarmony 源码
- 用于设备通信的 HDC 工具
- 通过 HDC 连接的目标设备

## 构建系统

构建命令和目标见根 `../AGENTS.md` § 构建和验证。构建产物位于 `out/<product>/testfwk/arkxtest/`。

### 构建配置

- **构建系统**：GN（Generate Ninja）配合 `BUILD.gn`
- **特性标志**（通过 `arkxtest_product_feature` 从 `arkxtest_config.gni` 获取）：
  - `ARKXTEST_FONT_ENABLE` - 字体管理（pc/tablet/phone 配合 graphic_2d）
  - `ARKXTEST_PASTEBOARD_ENABLE` - 剪贴板（当 pasteboard 部件存在时）
- **在以下产品特性上禁用**：`watch`/`glasses`（`install_enable = false`）
- 无独立 lint/typecheck 步骤；正确性通过构建 + 运行 gtest 验证

### 设备部署

构建产物位置：`out/<product>/testfwk/arkxtest/`（相对于 OpenHarmony 根目录）

```bash
# 挂载文件系统为读写模式（写入系统分区所需）
hdc target mount
hdc shell mount -o rw,remount /

# 推送二进制文件
hdc file send out/rk3568/testfwk/arkxtest/testhelper /system/bin/testhelper
hdc shell chmod +x /system/bin/testhelper

# 验证
hdc shell testhelper --version
```

## 知识路由

改动前按任务/路径/词汇先读对应章节；编辑前须声明 ① 任务属哪一行 ② 已读本文件对应章节 ③ 触发了哪些项目约束。

| 触发条件 | 主要文件（高频落点） | 先读本文件 |
| --- | --- | --- |
| 新增/改 CLI 命令 | `src/testhelper_main.cpp`（`g_commandHandlers:91`、`g_argCountMap:52`）、`src/testhelper_core.cpp`（`Handle*` 方法） | § 新增命令 |
| 改命令调度/argv 校验 | `src/testhelper_main.cpp`（`ValidateArgCount:73`、`g_commandHandlers:91`） | § 核心组件 + § CLI 调度实现细节 |
| 改时间解析/格式 | `src/testhelper_core.cpp`（`ParseTimeToMs:129`、`ValidateTimeRanges:122`） | § 时间操作（ParseTimeToMs） |
| 改 GPX 解析 | `src/gpx_parser.cpp`（`GPXParser::ParseFile`）、`src/testhelper_core.cpp`（`HandleSetMockedLocations:528`、`ValidateGPXFilePath:444`） | § 位置模拟 + § GPX 解析器实现细节 |
| 改字体管理 | `src/testhelper_core.cpp`（`HandleGetFontname`/`HandleInstallFont`/`HandleUninstallFont`） | § 字体管理 + § 项目约束（特性标志） |
| 改剪贴板操作 | `src/testhelper_core.cpp`（`get/set/clear-pastedata` 处理器） | § 剪贴板操作 + § 项目约束（特性标志） |
| 改日志/工具函数 | `src/common_utils.cpp`（`LOG_I/D/W/E`、`PrintToConsole`、`SafeStoi`、`GenLogTag`） | § 核心组件（工具函数） |
| 任务提到 `fdsan` / 文件描述符安全 | `src/testhelper_core.cpp`（`fdsan_exchange_owner_tag`/`fdsan_close_with_tag`） | § 项目约束（禁止项） + § 重要上下文 |
| 任务提到 `ParseTimeToMs` / 时间格式 | `src/testhelper_core.cpp`（`:129` 四阶段校验） | § 时间操作（ParseTimeToMs） |
| 任务提到 `GPX` / 位置模拟 | `src/gpx_parser.cpp`、`src/testhelper_core.cpp`（`:528`） | § 位置模拟 + § GPX 解析器 |
| 任务提到 `TestServerClient` / SA 5502 | `src/testhelper_core.cpp`（`TestServerClient::GetInstance().<Call>()`） | § 架构（命令调度模型） |
| 改公共行为/退出码 | `include/common_utils.h`（`EXIT_*` 常量） | § 核心组件（工具函数） |
| 改 `ARKXTEST_FONT_ENABLE` / `ARKXTEST_PASTEBOARD_ENABLE` | `BUILD.gn`（特性标志） | § 构建配置 + § 项目约束（改前须确认） |

## 架构

### 命令调度模型

```
CLI (argv)
  ↓
testhelper_main.cpp: g_commandHandlers 查找 + 参数数量校验
  ↓
TestHelperCore::Handle*()
  ↓ 校验输入 + TestServerClient::GetInstance().<Call>()
TestServer SA (ID 5502)
```

> TestServer SA 的详细架构（生命周期、调用者追踪、按需启停）见 [`testserver/AGENTS.md` § 架构](../testserver/AGENTS.md#架构)。

### 核心组件

**入口点（`src/testhelper_main.cpp`）**
- `main()` - CLI 入口点，命令调度
- `g_argCountMap` - 参数数量校验（每个命令的最小/最大参数数/用法）
- `g_commandHandlers` - Lambda 注册表，映射命令 → 处理器

**处理器层（`src/testhelper_core.cpp`）**
- `TestHelperCore` - 各命令处理器（`HandleGetTime`、`HandleSetTime`、`HandleSetMockedLocations` 等）
- 输入校验辅助函数：`ValidateTimeRanges`、`ValidateFontFileFormat`、`ValidateGPXFilePath`、`ValidateTimeInterval`
- `ParseTimeToMs` - 严格时间字符串解析（`YYYY-MM-DD HH:MM:SS`）

**GPX 解析器（`src/gpx_parser.cpp`）**
- 基于 libxml2 的 GPX 1.1 解析器，支持 `wpt`、`trk`/`trkpt`、`rte`/`rtept` 元素
- `GPXElementData` 结构体保存解析后的点位属性
- 校验坐标（纬度 [-90,90]，经度 [-180,180]）、速度（≥0）、方向（[0,360]）

**工具函数（`src/common_utils.cpp/.h`）**
- 日志宏：`LOG_I`/`LOG_D`/`LOG_W`/`LOG_E`（设备上基于 hilog，设备外无操作）
- `PrintToConsole` - 面向用户的标准输出
- `SafeStoi` - 安全的字符串转整数
- `GenLogTag` - 从文件名/函数名编译时生成日志标签
- 退出/解析常量：`EXIT_SUCCESS(0)`、`EXIT_FAILURE(1)`、`EXIT_SERVICE_UNAVAILABLE(2)`、`EXIT_NOT_SUPPORTED(3)`、`PARSE_*`

### 各特性实现细节

#### 时间操作

**get-time（`HandleGetTime`）**：获取 `MiscServices::TimeServiceClient::GetInstance()` 单例，为空时返回 `EXIT_SERVICE_UNAVAILABLE`；调用 `GetWallTimeMs(timeMs)` 获取挂钟毫秒数，除以 `MS_PER_SECOND(1000)` 转为秒，通过 `strftime` 格式化为 `YYYY-MM-DD HH:MM:SS`，并通过 `PrintToConsole` 输出。

**set-time（`HandleSetTime`）**：首先通过 `ParseTimeToMs`（见下文）解析/校验时间字符串，失败时分别报告格式错误与值错误；通过 `TestServerClient::GetInstance().SetTime(timeMs)` 调度，返回 `TEST_SERVER_OK` 时成功，否则返回 `EXIT_SERVICE_UNAVAILABLE`。

**ParseTimeToMs（严格时间解析）**：实现四阶段校验 ——
1. `std::regex_match` 匹配 `^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$`，强制月/日/时/分/秒为双位数字；
2. `strptime` 解析，要求结束指针指向 `'\0'`；
3. `ValidateTimeRanges` 范围检查（年 ≥1970，月 1-12，日 1-31，时 0-23，分/秒 0-59）；
4. **mktime 往返校验**：通过 `mktime`→`localtime` 将 `tm` 转换回来，比较所有字段以检测无效日期（2月30日、4月31日等）；同时限制 `seconds ≤ MAX_TIMESTAMP_SECONDS(2038-01-19 03:14:07 UTC)`。

**get-timezone / set-timezone**：`get` 通过 `TimeServiceClient::GetInstance().GetTimeZone`；`set` 通过 `TestServerClient::SetTimezone`，区分 `TEST_SERVER_INVALID_TIMEZONE_ID`（返回 `EXIT_FAILURE`）与服务不可用。

#### 剪贴板操作

**get-pastedata / set-pastedata / clear-pastedata**：均通过对应的 `TestServerClient` 方法，由 `ARKXTEST_PASTEBOARD_ENABLE` 门控（当部件存在时编译）。`set-pastedata` 在本地预校验文本长度 ≤128MB（`128*1024*1024`），溢出时返回 `EXIT_FAILURE`。收到 `TEST_SERVER_NOT_SUPPORTED` 时，返回 `EXIT_FAILURE` 并附带设备不支持消息。

#### 键盘操作

**hide-keyboard（`HandleHideKeyboard`）**：调用 `TestServerClient::GetInstance().HideKeyboard()`，区分三种失败情况：`TEST_SERVER_NOT_SUPPORTED`（无 IMF）、`TEST_SERVER_NO_ACTIVE_IME`（无活跃输入法）—— 均返回 `EXIT_FAILURE`；其他情况返回 `EXIT_SERVICE_UNAVAILABLE`。

#### 字体管理

三个命令（`get-fontname`/`install-font`/`uninstall-font`）均由 `ARKXTEST_FONT_ENABLE` 门控；禁用时 `#ifndef` 分支返回 `EXIT_NOT_SUPPORTED`。

**get-fontname（`HandleGetFontname`）**：三阶段路径校验 ——
1. 前缀必须为 `/data/local/tmp/`（`find(prefix) != 0` 则拒绝）；
2. `ValidateFontFileFormat` 检查扩展名为 `.ttf` 或 `.ttc`；
3. 去除前缀后的文件名不得包含 `..`（防止路径遍历）；
然后 `open(O_RDONLY)`，通过 `fdsan_exchange_owner_tag` 绑定 fd 所有权；调用 `Rosen::FontToolSet::GetInstance().GetFontFullName(fd)` 获取字体全名；最后 `fdsan_close_with_tag`（每个分支均关闭）。返回第一个字体名。

**install-font（`HandleInstallFont`）**：相同路径校验；仅打开 fd 验证可读性后立即 `fdsan_close_with_tag`；实际安装通过 `TestServerClient::InstallFont(fontPath)`，区分 `TEST_SERVER_FONT_ALREADY_INSTALLED`。

**uninstall-font（`HandleUninstallFont`）**：首先检查 `fontName` 非空；通过 `TestServerClient::UninstallFont`，区分 `TEST_SERVER_FONT_NOT_FOUND`。

#### 深色/浅色模式

**set-viewmode（`HandleSetViewMode`）**：`mode` 必须为 `"dark"` 或 `"light"`，否则 `EXIT_FAILURE`；先通过 `GetViewMode()` 探测能力 —— `"NOT_SUPPORTED"` 报告不支持，值等于目标 `mode` 报告已设置（均 `EXIT_FAILURE`）；然后调度 `SetViewMode(mode)`。

#### 位置模拟

**enable-location-mock / disable-location-mock**：通过 `TestServerClient` 的 `EnableLocationMock`/`DisableLocationMock`；`TEST_SERVER_NOT_SUPPORTED` 表示设备不支持。

**set-mocked-locations（`HandleSetMockedLocations`）**：核心流程 ——
1. `xmlInitParser()` 初始化 libxml2，`xmlCleanupParser()` 在**每个返回路径**调用；
2. `ValidateGPXFilePath`：前缀 `/data/local/tmp/`，扩展名 `.gpx`，拒绝 `..`，`access(F_OK)` 存在性检查；
3. `ValidateTimeInterval`：间隔必须为 1-3600 秒；
4. `GPXParser::ParseFile` 解析位置点；
5. 空列表或 `> MAX_MOCK_LOCATIONS(1000)` 报错；
6. `TestServerClient::SetMockedLocations(locations, timeInterval)` 调度。

#### GPX 解析器实现细节（`GPXParser`）

- `ParseFile`：`xmlReadFile`（`XML_PARSE_NOBLANKS | XML_PARSE_NONET`）读取文档，`xmlDocGetRootElement` 获取根元素；`ValidateGPXRoot` 要求根元素为 `gpx`；`ProcessGPXChildren` 遍历子节点分发给 `wpt`/`trk`/`rte` 处理器；最后 `xmlFreeDoc`。
- 元素调度：`wpt`→`ProcessWaypoint`；`trk`→`ProcessTrack`→`trkseg`→`trkpt`；`rte`→`ProcessRoute`→`rtept`，统一至 `ProcessPoint`。
- `ProcessPoint`：通过 `xmlGetProp` 获取 `lat`/`lon` 属性后 `xmlFree`；`ParsePointData` 遍历子节点获取 `ele`/`time` 以及 `speed`/`course|direction`（通过 `strcasestr` 模糊匹配支持顶层和 `<extensions>` 容器）。
- 数值转换使用本地 `SafeStod`（`strtod` + NaN 检查）。
- `ValidateCoordinates`：纬度∈[-90,90]，经度∈[-180,180]；`ParseOptionalFields`：速度≥0，方向∈[0,360]；`ele` 解析失败时静默回退到默认海拔 0。
- `FillLocation` 填充 `TestServerLocation`（精度默认 10.0，timeStamp/timeSinceBoot 设为 0，isFromMock 设为 true）。

#### CLI 调度实现细节（`testhelper_main.cpp`）

- `g_argCountMap`：`{命令 → (minArgs, maxArgs, usage)}`；`ValidateArgCount` 不匹配时调用 `_Exit(EXIT_FAILURE)`。
- `g_commandHandlers`：`{命令 → lambda(core, argc, argv)→int32_t}`。
- `set-time`：将 `argv[2..]` 用空格拼接（适应 shell 拆分 `"YYYY-MM-DD HH:MM:SS"`）。
- `set-mocked-locations`：`argv[2]` 为路径，可选 `argv[3]` 通过 `SafeStoi` 获取间隔（默认 1），非整数则报错。

## 运行测试

### 框架单元测试（C++ gtest）

```bash
# 构建（从 OpenHarmony 根目录）
./build.sh --product-name rk3568 --build-target testhelper_unittest

# 推送测试二进制到设备
hdc file send out/rk3568/tests/unittest/arkxtest/testhelper/testhelper_unittest /data/local/tmp/

# 运行所有测试
hdc shell /data/local/tmp/testhelper_unittest

# 带过滤器运行（单个用例/套件）
hdc shell /data/local/tmp/testhelper_unittest --gtest_filter=ParseTimeToMsTest.ValidTime_Normal
hdc shell /data/local/tmp/testhelper_unittest --gtest_filter=CommonUtilsTest.*
```

**测试目标：**
- `testhelper_core_test.cpp` - 时间解析、剪贴板、字体、键盘、时区测试
- `common_utils_test.cpp` - `GenLogTag`、`PrintToConsole` 工具函数测试

## 验证

### 按改动类型的验证矩阵

| 改动类型 | 必跑构建目标 | 必跑 unittest | 必跑 shell 回归 | 备注 |
| --- | --- | --- | --- | --- |
| C++ core（`testhelper_core.cpp` 处理器/校验） | `testhelperkit` | `testhelper_unittest` | `--version` + 对应子命令 | 见 § Shell 命令 |
| GPX 解析（`gpx_parser.cpp`） | `testhelperkit` | `testhelper_unittest`（`--gtest_filter=GPXParser*`） | `set-mocked-locations /data/local/tmp/route.gpx 2` | — |
| CLI 调度（`testhelper_main.cpp`） | `testhelperkit` | `testhelper_unittest`（`--gtest_filter=CommonUtilsTest.*`） | `help` + 各子命令 argv 校验 | — |
| 工具函数（`common_utils.cpp`） | `testhelperkit` | `testhelper_unittest`（`--gtest_filter=CommonUtilsTest.*`） | — | — |
| 特性标志改动 | `testhelperkit` | — | 对应命令在目标产品上验证 | 见 § 构建配置 |

### 完成定义

模板见根 `../AGENTS.md` § Done 定义。testhelper 专属项：
- 构建目标：`testhelperkit`（或 `testhelper_unittest`）
- 运行证据：`testhelper_unittest`；涉及设备侧的须附 shell 回归输出（命令见 § Shell 命令）
- 约束确认：输入校验前置 / fdsan 资源安全 / watch/glasses 禁用 / 时间格式契约

### 无法设备侧验证时的兜底

见根 `../AGENTS.md` § 无法 device-side 验证时的兜底。

## 新增命令

新增 testhelper CLI 命令须同步修改以下 6 处：

| 步骤 | 文件（行号） | 内容 |
| --- | --- | --- |
| 1. 声明参数个数 | `src/testhelper_main.cpp`（`g_argCountMap:52`） | 添加 `{"命令名", std::make_tuple(minArgs, maxArgs, "usage")}` |
| 2. 注册处理器 | `src/testhelper_main.cpp`（`g_commandHandlers:91`） | 添加 `{"命令名", lambda → ValidateArgCount + core.HandleXxx}` |
| 3. 更新帮助 | `src/testhelper_main.cpp`（`HELP_MSG:27`） | 在 Commands 列表添加命令说明 |
| 4. 声明方法 | `include/testhelper_core.h`（`TestHelperCore` 类） | 添加 `int32_t HandleXxx(...);` |
| 5. 实现方法 | `src/testhelper_core.cpp` | 实现 `HandleXxx`，输入校验须在 testserver SA 调用前完成 |
| 6. 补充测试 | `test/unittest/testhelper_core_test.cpp` | 为新命令添加 gtest 用例，覆盖合法、非法格式、边界和服务不可用路径 |

### 约束（新增命令必读）

- **禁止跳过 `ValidateArgCount`**：处理器入口必须先校验参数个数。
- **禁止在服务调用后做输入校验**：所有参数校验（格式、范围、路径前缀 `/data/local/*` 等）须在 `HandleXxx` 调用 `TestServerClient` 前完成。
- **产品特性禁用检查**：`watch`/`glasses` 上禁用的命令须检查 `ARKXTEST_*_ENABLE`。

## 目录结构

```
testhelper/
├── include/
│   ├── testhelper_core.h          # TestHelperCore 类声明
│   ├── common_utils.h             # 日志宏、常量、工具函数
│   └── gpx_parser.h               # GPXParser 类声明
├── src/
│   ├── testhelper_main.cpp        # CLI 入口，命令调度
│   ├── testhelper_core.cpp        # 命令处理器 + 校验
│   ├── common_utils.cpp           # PrintToConsole, SafeStoi
│   └── gpx_parser.cpp             # libxml2 GPX 1.1 解析器
├── test/
│   └── unittest/
│       ├── testhelper_core_test.cpp
│       └── common_utils_test.cpp
└── BUILD.gn                        # 构建配置
```

## 关键文件参考

| 组件 | 路径 |
|------|------|
| CLI 入口 | `src/testhelper_main.cpp` |
| 命令处理器 | `src/testhelper_core.cpp` |
| 日志/常量 | `src/common_utils.cpp` / `include/common_utils.h` |
| GPX 解析器 | `src/gpx_parser.cpp` / `include/gpx_parser.h` |
| 测试 | `test/unittest/` |
| 构建配置 | `BUILD.gn` |

### 高频/高风险改动路径

- `src/testhelper_core.cpp`（命令处理器 + 输入校验，所有命令的校验逻辑高频落点）
- `src/testhelper_main.cpp`（CLI 调度 + argv 校验，命令注册核心）
- `src/gpx_parser.cpp`（GPX 解析，libxml2 资源管理高风险）
- `include/common_utils.h`（退出码/日志宏，公共行为定义）

## Shell 命令

**位置：** `/system/bin/testhelper`

```bash
hdc shell testhelper --version
hdc shell testhelper help
hdc shell testhelper get-time
hdc shell testhelper set-time "2026-02-28 14:30:00"
hdc shell testhelper get-timezone
hdc shell testhelper set-timezone Asia/Shanghai
hdc shell testhelper get-pastedata
hdc shell testhelper set-pastedata "text"
hdc shell testhelper clear-pastedata
hdc shell testhelper hide-keyboard
hdc shell testhelper get-fontname /data/local/tmp/font.ttf
hdc shell testhelper install-font /data/local/tmp/font.ttf
hdc shell testhelper uninstall-font "FontName"
hdc shell testhelper set-viewmode dark
hdc shell testhelper enable-location-mock
hdc shell testhelper disable-location-mock
hdc shell testhelper set-mocked-locations /data/local/tmp/route.gpx 2
```

## 代码风格

- **许可证**：每个 `.cpp`/`.h` 以 Apache-2.0 头开始 `Copyright (c) <year> Huawei Device Co., Ltd.`
- **命名空间**：所有代码位于 `namespace OHOS::testhelper { ... }`（内部 4 空格缩进）
- **命名**：类/结构体 PascalCase（`TestHelperCore`、`GPXParser`）；方法 PascalCase（`Handle*`、`Validate*`、`Parse*`）；结构体字段/局部变量小写；常量 `constexpr` UPPER_SNAKE_CASE
- **类型**：仅使用固定宽度类型（`int32_t`、`int64_t`、`uint8_t`、`size_t`）；字符串以 `const std::string&` 传递
- **包含顺序**：系统头文件（`<...>`）→ 第三方（`<libxml/...>`）→ 本地（`"..."`）→ 特性门控（`#ifdef ARKXTEST_FONT_ENABLE`）
- **格式化**：4 空格缩进，无制表符，函数左大括号独占一行，`} else if` 在同一行，约 120 字符行宽
- **日志**：`LOG_I/D/W/E(fmt, args...)` 使用 `%{public}s`/`%{public}d` 说明符；用户输出通过 `PrintToConsole()`（错误前缀 `"Error: "`）；`LOG_TAG "TestHelper"`，`LOG_DOMAIN 0xD003130`
- **错误处理**：处理器返回 `int32_t` 退出码；服务调用前校验输入；使用 fdsan 管理文件描述符；每个路径释放 libxml2 分配（`xmlFree`/`xmlFreeDoc`/`xmlCleanupParser`）；不抛出异常

## 依赖

| 依赖 | 用途 |
|------|------|
| `hilog:libhilog` | 日志 |
| `c_utils:utils` | 通用工具 |
| `time_service:time_client` | 时间/时区服务 |
| `libxml2:libxml2` | GPX XML 解析 |
| `ability_runtime:wantagent_innerkits` | Ability 运行时 |
| `ability_base:configuration` | Ability 基础 |
| `graphic_2d:rosen_text` / `2d_graphics` | 字体管理（特性门控） |
| `pasteboard:pasteboard_client` / `pasteboard_data` | 剪贴板（特性门控） |
| `../testserver/src:test_server_client` | TestServer SA 客户端 |

## 项目约束

根目录 `../AGENTS.md` § 项目约束 的跨组件约束同样适用。testhelper 专属约束：

**禁止项**：
- 将输入校验移到服务调用之后。所有输入校验（路径前缀 `/data/local/tmp/`、拒绝 `..` 遍历、检查扩展名 `.ttf`/`.ttc`/`.gpx`、检查范围）必须在 `TestServerClient::<Call>()` 之前完成 —— 不得延迟到服务端（见 § 重要上下文）。
- 在任何退出路径上泄漏文件描述符或 libxml2 分配。使用 fdsan 管理的 fd（`fdsan_exchange_owner_tag`/`fdsan_close_with_tag`），并在每个返回分支释放 libxml2 分配（`xmlFree`/`xmlFreeDoc`/`xmlCleanupParser`）。
- 移除 `watch`/`glasses` 产品特性禁用守卫（`install_enable = false`）。testhelper 不得在这些产品上构建/安装。

**改前须确认**：
- 修改 `ParseTimeToMs` 中的严格时间格式 `YYYY-MM-DD HH:MM:SS` 解析（`testhelper_core.cpp:129`，四阶段校验含 mktime 往返）—— 影响 `set-time` 契约。
- 修改 `ARKXTEST_FONT_ENABLE` / `ARKXTEST_PASTEBOARD_ENABLE` 特性标志 —— 确认受影响的产品（通用规则见根 `../AGENTS.md` § 项目约束）。
- 修改 `g_argCountMap` 参数契约（每个命令的最小/最大参数数，`testhelper_main.cpp:52`）—— 影响所有命令入口校验；`ValidateArgCount` 不匹配时调用 `_Exit(EXIT_FAILURE)`。
- 修改退出码常量（`include/common_utils.h` 中的 `EXIT_SUCCESS`/`EXIT_FAILURE`/`EXIT_SERVICE_UNAVAILABLE`/`EXIT_NOT_SUPPORTED`）—— 影响所有处理器使用的 CLI 契约。
- 修改 `ValidateGPXFilePath` 路径前缀 `/data/local/tmp/` 或 `..` 遍历检查（`testhelper_core.cpp:444`）—— 影响安全边界。

## 重要上下文

1. **CLI 工具**：这是一个委托 TestServer SA 5502 的命令行接口，不是库
2. **输入校验**：所有输入必须在服务调用前校验 —— 路径前缀（`/data/local/tmp/`）、拒绝 `..` 遍历、检查扩展名（`.ttf`/`.ttc`/`.gpx`）、检查范围
3. **特性门控**：字体和剪贴板特性根据产品特性条件编译
4. **资源安全**：fdsan 管理的文件描述符和 libxml2 分配必须在每个退出路径清理
5. **时间解析**：`ParseTimeToMs` 强制执行严格的 `YYYY-MM-DD HH:MM:SS` 格式，双位数字字段，并通过 mktime 往返校验
6. **GPX 限制**：最多 1000 个模拟位置，时间间隔 1-3600 秒

## 相关文档

- 父框架：`../AGENTS.md`
- TestServer：`../testserver/` - 系统能力服务（SA ID 5502）
- 依赖：`../deps.md` - 完整外部依赖列表
