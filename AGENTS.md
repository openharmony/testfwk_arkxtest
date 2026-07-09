# Arkxtest - AI Knowledge Base

## Overview

**arkxtest** is OpenHarmony's automated testing framework, consisting of the following components:

1. **JsUnit (Hypium)** - JavaScript/ArkTS unit test framework
2. **UiTest** - UI automation testing framework for component location and operation
3. **PerfTest** - White-box performance testing framework
4. **TestServer** - System ability service (SA ID: 5502) providing high-permission system interfaces
5. **TestHelper** - C++ CLI tool (`/system/bin/testhelper`) delegating to TestServer for system-level test operations

The framework provides APIs for writing test cases, executing them, and generating test reports for OpenHarmony applications and system interfaces.

## Quick Start

### Prerequisites
- OpenHarmony source code with build environment configured
- HDC tool for device communication
- Target device connected via HDC

### Repository Structure

arkxtest is located within the OpenHarmony repository:

```
OpenHarmony/
├── build.sh              # Build script (run from here)
├── test/xts/acts/        # XTS test suites
├── interface/sdk-js/api/ # API declarations
└── test/testfwk/
    └── arkxtest/         # ← You are here
        ├── jsunit/
        ├── uitest/
        ├── perftest/
        ├── testserver/
        └── testhelper/
```

All build commands should be run from the OpenHarmony root directory.

```bash
# Navigate to OpenHarmony root first
cd /path/to/openharmony

# Then run build commands
./build.sh --product-name rk3568 --build-target uitestkit
```

## Build System

**Important:** All build commands must be executed from the **OpenHarmony source code root directory**, not from the arkxtest directory. The arkxtest framework is located at `test/testfwk/arkxtest/` within the OpenHarmony repository.

### Build Commands

```bash
# Build all uitest components (client + server)
./build.sh --product-name rk3568 --build-target uitestkit

# Build perftest
./build.sh --product-name rk3568 --build-target perftestkit

# Build test helper CLI
./build.sh --product-name rk3568 --build-target testhelperkit

# Build test server
./build.sh --product-name rk3568 --build-target test_server_service
./build.sh --product-name rk3568 --build-target test_server_client
```

### Key Build Targets

| Target | Description |
|--------|-------------|
| `uitestkit` | Main uitest build group (all uitest components) |
| `cj_ui_test_ffi` | CJ FFI bindings (libcj_ui_test_ffi.z.so) |
| `perftestkit` | Performance testing framework |
| `testhelperkit` | TestHelper build group (binary + core library) |
| `testhelper` | TestHelper executable (`/system/bin/testhelper`) |
| `testhelper_unittest` | TestHelper gtest unit test binary |
| `test_server_service` | Test server daemon (SA ID: 5502) |
| `test_server_client` | Test server client library |

### Build Configuration

- **Build System**: GN (Generate Ninja) with BUILD.gn files
- **Product Features**: Conditional compilation via `arkxtest_product_feature`
  - `tablet` → `ARKXTEST_TABLET_FEATURE_ENABLE`
  - `pc` → `ARKXTEST_PC_FEATURE_ENABLE`
  - `watch` → `ARKXTEST_WATCH_FEATURE_ENABLE`
  - `phone/tablet` → `ARKXTEST_KNUCKLE_ACTION_ENABLE`

## Architecture

### Directory Structure

```
arkxtest/
├── jsunit/              # Unit test framework (Hypium)
│   ├── src/           # ArkTS-Dynamic implementation
│   │   ├── module/    # Core modules (assert, mock, report, config)
│   │   ├── core.js    # Core framework
│   │   └── service.js # Test service
│   ├── src_static/    # ArkTS-Static implementation
│   └── package.json     # NPM package configuration
│
├── uitest/             # UI automation testing framework
│   ├── core/            # C++ core logic (UiDriver, WidgetSelector, WidgetOperator)
│   ├── server/          # Server daemon (system_ui_controller)
│   ├── connection/      # IPC communication layer
│   ├── input/           # Input injection (touch, keyboard, mouse)
│   ├── record/          # UI recording/replay functionality
│   ├── addon/           # Extensions (screen capture, custom extensions)
│   ├── ets/ani/         # ArkTS-Static bindings (ANI)
│   ├── napi/            # ArkTS-Dynamic bindings (N-API)
│   ├── cj/              # Cangjie FFI bindings
│   └── test/            # C++ unit tests (gtest)
│
├── perftest/           # Performance testing framework
│   ├── core/            # Core perf testing logic
│   ├── collection/      # Performance data collection
│   ├── connection/      # IPC for perf testing
│   ├── napi/            # N-API bindings for ArkTS-Dynamic
│   └── ani/             # ANI bindings for ArkTS-Static
│
├── testserver/         # Central test server (IPC hub, SA ID: 5502)
│   ├── src/            # Server implementation (ITestServerInterface.idl)
│   ├── init/           # Service initialization (testserver.cfg)
│   └── sa_profile/     # Service profile (5502.json)
│
├── testhelper/         # C++ CLI tool delegating to TestServer
│   ├── include/        # Headers (testhelper_core.h, common_utils.h, gpx_parser.h)
│   ├── src/            # Command handlers, GPX parser, utilities
│   └── test/unittest/  # gtest unit tests
│
└── hamock/             # Mock framework (part of jsunit)
```
### jsunit (Hypium) - Unit Test Framework

A JavaScript/ArkTS unit testing framework distributed as an npm package (`@ohos/hypium`), supporting both ArkTS-Dynamic (`src/`) and ArkTS-Static (`src_static/`) environments with a unified API surface.

**Core Features:**

- **Basic Process Support**: `describe`, `it`, `beforeAll`, `beforeEach`, `afterEach`, `afterAll`, plus extended hooks (`beforeEachIt`, `afterEachIt`, `beforeItSpecified`, `afterItSpecified`)
- **Assertion Library**: 25+ assertion methods (`assertEqual`, `assertTrue`, `assertContain`, `assertClose`, `assertInstanceOf`, `assertDeepEquals`, `assertMatchObj`, etc.)
- **Mock Capability**: Function stubbing with `when()`/`afterReturn()`/`afterThrow()`, call verification with `verify().times()`, 14 `ArgumentMatchers` (type + fuzzy regex/substring/array matchers)
- **Data Driving**: Parameterized tests with `dataProvider`
- **Async Testing**: Native Promise handling with `assertPromiseIsResolved`/`assertPromiseIsRejected`
- **Special Features**: Test filtering, skipping (`xdescribe`/`xit` + `throw SkipError`), timeout, random execution, stress testing, parallel worker execution
- **Failure Capture**: `-s snapshotWhenFail true` auto-captures screenshot + layout dump on failure
- **Tag-based Filtering**: `-s tag <expr>` with comma (OR) / plus (AND) operators

**Usage:**

```javascript
import { describe, it, expect, beforeAll, beforeEach, afterEach, afterAll } from '@ohos/hypium'

export default async function abilityTest() {
  describe('ActsAbilityTest', function () {
    it('assertContain_success', 0, function () {
      let a = 'abc'
      let b = 'b'
      expect(a).assertContain(b)
    })
  })
}
```

### Uitest - UI Test Framework

uitest is OpenHarmony's UI automation testing framework, operating on a **client-server architecture** with multi-language support (ArkTS-Static, ArkTS-Dynamic, JavaScript, and Cangjie).

**Core Features:**
- Provides APIs for ArkTS-Dynamic and static languages, supporting UI test for different language projects
- Driver class APIs for UI automation (create, findComponent, pressBack, click, swipe, etc.)
- On class APIs for component matching (id, text, type, enabled, clickable, etc.)
- Component class APIs for component operations (click, inputText, scrollSearch, etc.)
- UiWindow class APIs for window operations (getBundleName, focus, moveTo, resize, close)
- UIEventObserver class APIs for listening to UI events (toastShow, dialogShow, windowChange, componentEventOccur)
- Shell command support for quick UI operations (screenCap, dumpLayout, uiRecord, uiInput)
- Multi-display & multi-user support (since API 26): dynamic AAMS user switching for multi-display multi-user concurrent scenarios

**Client-Server Model:**
```
Test Application (ArkTS/JS)
    ↓ NAPI/ANI Bindings
uitest_client (libuitest.z.so/libuitest_ani.so)
    ↓ IPC (CommonEvent)
uitest_server (uitest daemon)
    ↓ Accessibility/Window Manager
System Under Test
```

For detailed architecture, multi-display adaptation, shell commands, and deployment, see `uitest/AGENTS.md`.

### PerfTest Architecture - White-box Performance Test Framework

**Core Features:**
- Code-segment based performance test
- Scenario-based performance test
- Metrics collection: DURATION (elapsed time), CPU_USAGE (CPU usage), MEMORY (memory), LIST_SWIPE_FPS (list swipe frame rate), APP_START_TIME (app startup time), PAGE_SWITCH_TIME (page switch time)
- Test strategy configuration (metrics, actionCode, resetCode, iterations, timeout)
- Statistical analysis (max, min, average)

**Three-Layer Architecture:**
```
Frontend Layer (N-API/ANI Clients)
    ↓ IPC Communication
Connection Layer (ApiCallerProxy/Stub)
    ↓
Core Layer (PerfTest, PerfTestStrategy)
    ↓
Data Collection Layer (Duration, CPU, Memory, FPS, etc.)
```

**Supported Metrics:**
- `DURATION` - Execution time
- `CPU_LOAD` - System CPU load
- `CPU_USAGE` - Process CPU usage
- `MEMORY_RSS/PSS` - Memory consumption
- `APP_START_RESPONSE/COMPLETE_TIME` - App startup latency
- `PAGE_SWITCH_COMPLETE_TIME` - Page switch latency
- `LIST_SWIPE_FPS` - Scrolling frame rate

### TestServer Architecture - Test SA

**Core Features:**
- Test scenario-specific SA, supporting user version test capabilities, providing system-level capabilities for test frameworks, with security controls to protect the system
- This module is used by other test tools, not directly visible to 2D developers

System service (SA ID: 5502) that provides high-permission interfaces:
- Window management operations
- Clipboard operations
- System event publishing
- CPU frequency locking
- Performance monitoring (CPU/memory)
- SmartPerf process control

Lifecycle: On-demand start (first client connection), auto-stop when all clients exit or developer mode disabled.

### TestHelper Architecture - C++ CLI Tool

**TestHelper** is a C++ command-line tool (`/system/bin/testhelper`) that delegates to the TestServer SA (ID 5502) through `TestServerClient` to expose system-level test capabilities. It is disabled on `watch`/`glasses` product features.

**Command Dispatch Model:**
```
CLI (argv)
  ↓
testhelper_main.cpp: g_commandHandlers lookup + arg count validation
  ↓
TestHelperCore::Handle*()
  ↓ Validate inputs + TestServerClient::GetInstance().<Call>()
TestServer SA (ID 5502)
```

**Supported Commands:**
- **Time Operations**: `get-time`, `set-time` (strict `YYYY-MM-DD HH:MM:SS` parsing with mktime round-trip validation), `get-timezone`, `set-timezone`
- **Clipboard Operations**: `get-pastedata`, `set-pastedata` (max 128MB), `clear-pastedata` (gated by `ARKXTEST_PASTEBOARD_ENABLE`)
- **Keyboard Operation**: `hide-keyboard`
- **Font Management**: `get-fontname`, `install-font`, `uninstall-font` (gated by `ARKXTEST_FONT_ENABLE`; path must be under `/data/local/tmp/`)
- **Dark/Light Mode**: `set-viewmode <dark|light>`
- **Location Mocking**: `enable-location-mock`, `disable-location-mock`, `set-mocked-locations <gpx> [interval]` (libxml2 GPX 1.1 parser, max 1000 locations, interval 1-3600 seconds)

**Input Validation**: All inputs validated before service calls — path prefix (`/data/local/tmp/`), reject `..` traversal, check extensions (`.ttf`/`.ttc`/`.gpx`), check ranges. Resource safety via fdsan-managed file descriptors and libxml2 cleanup on every exit path.

For detailed implementation, see `testhelper/AGENTS.md`.

## Running Tests

### Framework Unit Tests (C++ TDD)

**Build Tests:**
```bash
./build.sh --product-name rk3568 --build-target uitestkit_test
./build.sh --product-name rk3568 --build-target testserver_unittest
./build.sh --product-name rk3568 --build-target perftest_unittest
./build.sh --product-name rk3568 --build-target testhelper_unittest

```

**Run Tests:**
```bash
# Push test binary to device
hdc file send out/rk3568/tests/unittest/arkxtest/uitest/uitest_core_unittest /data/local/tmp/

# Run tests
hdc shell ./uitest_core_unittest

# Run with filter
hdc shell ./uitest_core_unittest --gtest_filter=WidgetSelectorTest.*
hdc shell /data/local/tmp/testhelper_unittest --gtest_filter=ParseTimeToMsTest.ValidTime_Normal
```

**Test Files:** `uitest/test/`, `testserver/test/`, `perftest/test/`, `testhelper/test/`

### Application Tests (ArkTS)

**Note:** Test application build commands should also be executed from the OpenHarmony source code root directory.

**Build Test Application:**
```bash
# ArkTS-Dynamic
./test/xts/acts/build.sh product-name <product> system_size=standard suite=<testsuite_path>:<target>

# ArkTS-Static
./test/xts/acts/build.sh product-name <product> system_size=standard xts_suitetype=hap_static suite=<testsuite_path>:<target>

# Install HAP
hdc install out/rk3568/suites/haps/<target>.hap
```

**Run Tests:**
```bash
# ArkTS-Dynamic
hdc shell aa test -p <bundleName> -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <className>

# ArkTS-Static
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <className>
```

## Deployment Workflow

### 1. Build Components
```bash
cd /path/to/openharmony
./build.sh --product-name rk3568 --build-target uitestkit
```

### 2. Verify Build Artifacts
```bash
ls -lh out/rk3568/testfwk/arkxtest/
```

Expected output location: `out/<product>/testfwk/arkxtest/` in the OpenHarmony root directory.

### 3. Deploy to Device
```bash
# Mount system partition as read-write
hdc target mount
hdc shell mount -o rw,remount /

# Push uitest server and client (see uitest/AGENTS.md for detailed deployment steps)
hdc file send out/rk3568/testfwk/arkxtest/uitest /system/bin/uitest
hdc file send out/rk3568/testfwk/arkxtest/libuitest.z.so /system/lib/module/libuitest.z.so
hdc shell chmod +x /system/bin/uitest

# Push testhelper CLI (see testhelper/AGENTS.md for detailed deployment steps)
hdc file send out/rk3568/testfwk/arkxtest/testhelper /system/bin/testhelper
hdc shell chmod +x /system/bin/testhelper
```

### 4. Verify Service Status
```bash
# Check if uitest daemon can start
hdc shell uitest start-daemon test@1234@1000@0

# Check testserver SA (SA ID: 5502)
hdc shell smgr list | grep 5502
```

### 5. Run Tests
See "Running Tests" section above for specific test execution commands.

## Shell Commands & CLI Tools

Two on-device CLI binaries — verify both after every uitest/testhelper change:

**1. uitest** (`/system/bin/uitest`) - UI 快捷操作
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

**2. testhelper** (`/system/bin/testhelper`) - 委托 TestServer SA 5502 提供系统能力
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
完整命令列表见 `testhelper/AGENTS.md` 的 "Shell Commands"。

## Troubleshooting

### Build Issues
- **GN not found**: Ensure OpenHarmony build environment is properly configured with `./build.sh --product-name <product> --ccache --build-target gcc`
- **Permission denied**: Check write permissions for output directory
- **build.sh not found**: You must run build commands from OpenHarmony root directory, not from arkxtest directory

### Device Deployment Issues
- **Read-only filesystem**: Use `hdc target mount` and `hdc shell mount -o rw,remount /`
- **Permission denied on system/**: Some devices require additional steps for system partition modification. Ensure developer mode is enabled.
- **HDC connection failed**: Check `hdc list` to verify device is connected

### Service Not Starting
- **uitest daemon fails**: Check hilog logs with `hdc shell hilog | grep uitest`
- **testserver SA missing**: Verify 5502.json is in `/system/profile/testserver.json` and testserver.cfg is in `/system/etc/init/testserver.cfg`
- **SA not registered**: Use `hdc shell smgr list` to check if SA 5502 is running

### Test Failures
- **Widget not found**: Use `hdc shell uitest dumpLayout -p /data/local/tmp/layout.json` to inspect UI hierarchy
- **IPC timeout**: Check if server daemon is running with `hdc shell ps -ef | grep uitest`
- **Library not found**: Verify all shared libraries are pushed to correct system directories

### Common Path Issues
- Build artifacts are in `out/<product>/testfwk/arkxtest/` (relative to OpenHarmony root)
- API declarations are in `interface/sdk-js/api/` (relative to OpenHarmony root)
- Test suites are in `test/xts/acts/` (relative to OpenHarmony root)

## Dependencies

### Key Dependencies (29 total)

| Dependency | Purpose |
|------------|---------|
| `accessibility` | UI component tree access (uitest core) |
| `window_manager` | Window management |
| `input` | Input event injection |
| `image_framework` | Screenshots/image processing |
| `ipc` | Inter-process communication |
| `soc_perf` | Performance data collection (perftest) |
| `napi` | JavaScript/ArkTS runtime binding |
| `runtime_core` | ArkTS runtime environment (ANI) |
| `hilog` | Logging |
| `common_event_service` | System events |
| `safwk` / `samgr` | System ability framework (testserver) |
| `pasteboard` | Clipboard operations |
| `libxml2` | GPX XML parsing (testhelper) |
| `time_service` | Time/timezone service (testhelper) |
| `graphic_2d` | Font management (testhelper, feature-gated) |
| `hiview` / `hisysevent` | System event monitoring |

## Resource Usage
- **ROM**: 500KB
- **RAM**: 100KB

See `deps.md` for complete dependency list.

## Important Context

1. **Multi-Language Support**: All C++ features must have bindings in both ANI (ArkTS-Static) and NAPI (ArkTS-Dynamic)
2. **IPC Communication**: All client operations go through IPC to server daemon
3. **API Version**: Module support starts at API 8, with milestone features at API 9, 10, 11, 18, 20, 22, 23, 25
4. **System Capabilities**: Requires `SystemCapability.Test.UiTest` and `SystemCapability.Test.PerfTest`
5. **Adapted System**: Standard system type only
6. **Testing Framework**: This is the test framework itself, not an application under test

## Component-Specific Documentation

- **JsUnit**: `jsunit/AGENTS.md` - Hypium framework architecture, assertion/mock APIs, test execution
- **Uitest**: `uitest/AGENTS.md` - Detailed uitest architecture, API workflow, shell commands
- **PerfTest**: `perftest/CLAUDE.md` - Performance testing architecture, metrics, collection strategies
- **TestServer**: `testserver/CLAUDE.md` - System ability service, IPC interface, lifecycle management
- **TestHelper**: `testhelper/AGENTS.md` - CLI tool architecture, command handlers, implementation details
- **Dependencies**: `deps.md` - Complete list of external dependencies

## Related Documentation
**README**:
  - `README_zh.md` - Chinese documentation
  - `README_en.md` - English documentation
  - `jsunit/README.md` - Hypium framework documentation
- **Use guidelines**:
  - `docs/zh-cn/application-dev/application-test/perftest-guideline.md` - PerfTest Usage Guide
  - `docs/zh-cn/application-dev/application-test/uitest-guidelines.md` - UiTest Usage Guide
  - `docs/zh-cn/application-dev/application-test/unittest-guidelines.md` - Unit Testing Framework Usage Guide
- **API reference**: 
  - `docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md` - UiTest API Reference
  - `docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-perftest.md` - PerfTest API Reference
- **API declaration**: 
  - `interface/sdk-js/api/@ohos.UiTest.d.ts` - UiTest API Declaration
  - `interface/sdk-js/api/@ohos.test.PerfTest.d.ts` - PerfTest API Declaration
- `LICENSE` - Apache License 2.0
