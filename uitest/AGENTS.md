# UiTest - AI Knowledge Base

## Overview

uitest is OpenHarmony's UI automation testing framework, operating on a **client-server architecture** with multi-language support (ArkTS-Static, ArkTS-Dynamic, JavaScript, and Cangjie).

## Quick Start

### Repository Context

**Important:** uitest is located within the arkxtest framework, which is part of the OpenHarmony repository:

```
OpenHarmony/
├── build.sh              # Build script (run from here)
├── interface/sdk-js/api/ # API declarations
└── test/testfwk/
    └── arkxtest/
        └── uitest/       # ← You are here
            ├── core/              # C++ core logic
            ├── server/            # Server daemon
            ├── connection/        # IPC layer
            ├── input/             # Input injection
            ├── ets/ani/           # ArkTS-Static bindings
            ├── napi/              # ArkTS-Dynamic bindings
            ├── cj/                # Cangjie bindings
            └── test/              # C++ unit tests
```

**All build commands must be run from the OpenHarmony root directory**, not from the uitest directory.

### Prerequisites
- OpenHarmony source code with build environment configured
- HDC tool for device communication
- Target device connected via HDC

## Build System

**Important:** All build commands must be executed from the **OpenHarmony source code root directory**, not from the uitest directory. Build artifacts are located in `out/<product>/testfwk/arkxtest/` relative to the OpenHarmony root.

### Build Commands

```bash
# Navigate to OpenHarmony root first
cd /path/to/openharmony

# Build all uitest components (client + server)
./build.sh --product-name rk3568 --build-target uitestkit

# Build specific components
./build.sh --product-name rk3568 --build-target uitest_client    # NAPI client library
./build.sh --product-name rk3568 --build-target uitest_server    # Server executable
./build.sh --product-name rk3568 --build-target uitestkit_test   # Unit tests
```

### Key Build Targets

| Target | Description |
|--------|-------------|
| `uitestkit` | Main build group (all components) |
| `uitest_client` | NAPI client library (libuitest.z.so) |
| `uitest_server` | Server executable (installed as `uitest`) |
| `uitest_ani` | ANI bindings for ArkTS-Static (libuitest_ani.so) |
| `cj_ui_test_ffi` | CJ FFI bindings (libcj_ui_test_ffi.z.so) |

### Build Configuration

- **Build System**: GN (Generate Ninja) with BUILD.gn files
- **Product Features**: Conditional compilation via `arkxtest_product_feature`
  - `tablet` → `ARKXTEST_TABLET_FEATURE_ENABLE`
  - `pc` → `ARKXTEST_PC_FEATURE_ENABLE`
  - `watch` → `ARKXTEST_WATCH_FEATURE_ENABLE`
  - `phone/tablet` → `ARKXTEST_KNUCKLE_ACTION_ENABLE`

### Device Deployment

Build artifacts location: `out/<product>/testfwk/arkxtest/` (relative to OpenHarmony root)

**Deploy to device:**
```bash
# All commands assume you're in OpenHarmony root directory

# Mount filesystem as read-write (required for system partition writes)
hdc target mount
hdc shell mount -o rw,remount /

# Push uitest server executable
hdc file send out/rk3568/testfwk/arkxtest/uitest /system/bin/uitest

# Push NAPI client library
hdc file send out/rk3568/testfwk/arkxtest/libuitest.z.so /system/lib/module/libuitest.z.so

# Push ANI client library
hdc file send out/rk3568/testfwk/arkxtest/libuitest_ani.so /system/lib/libuitest_ani.so

# Set executable permission
hdc shell chmod +x /system/bin/uitest
```

**Installation locations:**
- Server executable: `/system/bin/uitest`
- Client library: `/system/lib/module/libuitest.z.so`
- ANI library: `/system/lib/libuitest_ani.so`

## Code Style

### C++ Code Style

- **Include guards**: `#ifndef FILE_NAME_H` / `#define` / `#endif` — never `#pragma once`. Apache 2.0 license header on every file (`Copyright (c) YYYY Huawei Device Co., Ltd.`).
- **Namespace**: `namespace OHOS::uitest { ... } // namespace OHOS::uitest`. Body indented **4 spaces**. `using namespace std;` and `using namespace nlohmann;` are common at the top.
- **Naming**: PascalCase classes/functions/structs (`UiDriver`, `FindWidgets`, `ApiCallErr`); trailing-underscore members (`uiController_`, `code_`); `UPPER_CASE` constexpr constants (`INDEX_ZERO`, `NAPI_MAX_BUF_LEN`). **No `m_` prefix.** Plain struct data fields often have no suffix (`bundleName`, `text`).
- **Includes**: system → third-party (`nlohmann/json.hpp`, `napi/native_api.h`) → local project headers (`ui_driver.h`, `common_utilities_hpp.h`). Conditional includes guarded by `#ifdef ARKXTEST_*_ENABLE`.
- **Smart pointers**: idiomatic `unique_ptr`/`shared_ptr` + `make_unique`/`move`. Use OHOS `sptr`/`wptr` for IPC ref-counted objects.
- **Modern**: `override` on virtual overrides; `~Foo() = default`. No exceptions for flow control.

### Error Handling & Logging

- **No exceptions for control flow.** uitest uses an out-param `ApiCallErr& err` (last meaningful param; `frontend_error_defines.h:67`, holds `ErrCode code_` + `message_`). Error codes defined in `frontend_error_defines.h:20` (`ERR_INVALID_INPUT=401`, `ERR_OPERATION_UNSUPPORTED=17000005`, etc.).
- **`DCHECK(cond)` is fatal**: logs `LOG_E` + `_Exit(0)` (`common_utilities_hpp.h:164`). Use ONLY for invariants, never recoverable errors.
- **Logging**: `LOG_D/I/W/E(fmt, args...)` macros (auto-generate tag from file/function). Format strings use HiLog privacy tags: `%{public}s`, `%{public}d`. Define `LOG_TAG` per BUILD.gn target (`UiTestKit_Base`, `UiTestKit_Server`, etc.).

```cpp
#ifndef MY_CLASS_H
#define MY_CLASS_H
#include "common_utilities_hpp.h"
namespace OHOS::uitest {
class MyClass {
public:
    void DoWork(ApiCallErr &err);
private:
    std::unique_ptr<Helper> helper_;
};
} // namespace OHOS::uitest
#endif // MY_CLASS_H
```

### ArkTS / JavaScript Style (bindings & API)

- **`.ets` (ArkTS-Static)** files in `ets/ani/ets/` use `'use static'`-style full type annotations, native function declarations via `export native function`, and `loadLibraryWithPermissionCheck`. Import OHOS kits (`@ohos.hilog`, `@ohos.base`).
- **NAPI `.cpp`** (`napi/uitest_napi.cpp`): register module via `napi_module_register`; convert JS↔C++ with helper functions (`JsStrToCppStr`). Guard optional features with `#ifdef ARKXTEST_API_METRICS_ENABLE`.
- **Strings**: single quotes in JS; template literals for logs.

## Architecture

### Client-Server Model

```
Test Application (ArkTS/JS)
    ↓ NAPI/ANI Bindings
uitest_client (libuitest.z.so/libuitest_ani.so)
    ↓ IPC (CommonEvent)
uitest_server (uitest daemon)
    ↓ Accessibility/Window Manager
System Under Test
```

### Key Technologies

- **N-API**: For ArkTS-Dynamic bindings (`napi/uitest_napi.cpp`)
- **ANI**: For ArkTS-Static bindings (`ets/ani/src/uitest_ani.cpp`)
- **IPC**: CommonEventService-based communication (`connection/ipc_transactor.cpp`)
- **Accessibility**: For UI tree traversal and component inspection
- **Input Injection**: For simulating touch/keyboard/mouse events

### Component Flow

1. **Component Lookup**: `Driver.findComponent(ON.text('Hello'))` → IPC → Server → `WidgetSelector` → UI Model
   - Relative positioning via `On`: `isBefore`/`isAfter`/`within` (by `On` matcher), `beforeComponent`/`afterComponent`/`withinComponent` (by concrete `Component` object, since API 26)
   - `beforeComponent`/`afterComponent` direction is counter-intuitive: `beforeComponent` calls `AddRearLocator` (target is *behind* the given component in tree order), `afterComponent` calls `AddFrontLocator` (target is *ahead*). `withinComponent` calls `AddParentLocator`.
   - The `Component` passed to these methods is re-validated via `RetrieveWidget`; if it no longer exists in the current UI, `ERR_INVALID_PARAM` is returned.
2. **Component Operation**: `Component.click()` → IPC → Server → `WidgetOperator` → Input System
   - Coordinate-based operations: `clickAt`/`doubleClickAt`/`longClickAt`/`swipeBetween`/`dragBetween`, plus `*WithOptions` variants (since API 26) accepting `TouchOptions` (speed/duration/pressure) for finer control
   - **TouchOptions field validation**: each `*WithOptions` method only accepts a specific subset of `TouchOptions` fields (`ValidateTouchOptions` in `frontend_api_handler.cpp:1331`). Passing an unsupported field returns `ERR_INVALID_INPUT`. Per-method allowed fields: `clickAtWithOptions`→{pressure}, `longClickAtWithOptions`→{duration,pressure}, `swipeBetweenWithOptions`→{speed,pressure}, `dragBetweenWithOptions`→{speed,duration,pressure}, `mouseDragWithOptions`→{speed,duration}.
   - Mouse drag with keys: `mouseDragWithOptions(from, to, touchOptions?, keyOptions?)` (since API 26), `KeyOptions` provides `key1`/`key2` for combo keys. `key2` without `key1` returns `ERR_INVALID_PARAM` (`ParseKeyOptions` in `frontend_api_handler.cpp:1762`).
   - Pen key operations: `triggerPenKey(key, mode, operation, options?)` (since API 26), using `PenKey`/`PenMode`/`PenKeyOperation`/`PenKeyOperationOptions`
   - **PenKey valid combinations**: enforced by a whitelist (`VALID_PEN_KEY_COMBINATIONS` in `frontend_api_handler.cpp:1105`). AIR_MOUSE mode + AIR_MOUSE key requires `point` in options. Invalid combos return `ERR_INVALID_PARAM`.
   - **PenKey feature gate**: `triggerPenKey` is gated by `ARKXTEST_TRIGGER_PEN_KEY_ENABLE`, which is only defined for `pc`/`tablet`/`phone` product features (`BUILD.gn:251`). On unsupported products, `IsPenKeySupported` returns false and the API returns `ERR_OPERATION_UNSUPPORTED` (17000005).
   - **PenKey internal dispatch**: `PenKeyAction::IsMouseKeyCombo()` (AIR_MOUSE key + AIR_MOUSE mode) routes to `InjectMouseEventSequence`; all other combos route to `InjectKeyEventSequence` (`ui_driver.cpp:324`).
   - Multi-display layout dump: `dumpLayout(savePath, displayId?)` (since API 26). The `savePath` is converted to a file descriptor on the client side; the server writes JSON via `write(fd, ...)` after validating `displayId` exists (`CheckDisplayExist`).

### Multi-User & Multi-Display Adaptation (since API 26)

Before API 26, the uitest server connected to AAMS (Accessibility Manager Service) under the default user (user 0) only. Any API with a `displayId` parameter could operate on the specified screen, but **if that screen belonged to a non-user-0 user, the operation would silently fail** — AAMS was scoped to user 0 and could not see that user's UI tree.

API 26 introduces dynamic AAMS user switching to support **multi-display multi-user concurrent scenarios** (e.g., each display assigned to a different user via multi-user mode):

**Mechanism (`SysUiController::ConvertAAMS` in `system_ui_controller.cpp:1525`):**

1. **Single-user fast path**: At init, `Initialize()` queries `TestServerClient::GetUserCounts()`. If only 1 user exists, `isSingleUser_ = true` and `ConvertAAMS` short-circuits (returns true without any switching) — identical to pre-API 26 behavior, zero overhead.
2. **Multi-user switching**: When `isSingleUser_` is false, every `GetUiWindows(displayId)` / `GetWidgetsInWindow(winInfo)` call first invokes `ConvertAAMS(displayId)`:
   - Queries `TestServerClient::GetUserIdByDisplayId(displayId)` to find which user owns the display.
   - If the display's user matches `currentUser_`, no switch needed.
   - If different, calls `DisConnectFromSysAbility()` (disconnect AAMS for old user) then `ConnectToSysAbility()` which re-connects AAMS via `AccessibilityUITestAbility::Connect(userId)` under the new user (`system_ui_controller.cpp:1179`).
3. **Shell `dumpLayout`**: `server_main.cpp:228` also calls `GetUserIdByDisplayId` + `SetActiveUser(userId)` before creating the controller, ensuring the shell command targets the correct user's display.

**Key implementation details:**
- **Single AAMS connection at a time**: The server holds one AAMS connection (to one user). `ConvertAAMS` switches it as needed — operations across different users' displays are **serialized** by `dumpMtx` mutex (`system_ui_controller.h:109`), not truly concurrent.
- **`currentUser_` tracking**: Initialized to `-1` (defaults to `ability->Connect()` without userId = user 0); updated to `ability->GetCurrentUserId()` after successful connect (`:1198`); updated via `SetActiveUser(userId)` from shell/IPC.
- **TestServer dependency**: `GetUserIdByDisplayId` / `GetUserCounts` are provided by TestServer SA 5502 (`ITestServerInterface.idl:38-39`). Multi-user support requires TestServer to be running and aware of display-to-user mappings.

**Affected APIs**: Any Driver/Component API that ultimately calls `GetUiWindows` or `GetWidgetsInWindow` with a `displayId` — including `findComponent`/`dumpLayout`/`screenCap(displayId)`/coordinate operations on a specified display.

### Display ID Propagation & Multi-Display Operations

Every API that accepts a `displayId` (or `Point` with embedded `displayId`) follows a layered propagation path:

```
ArkTS API (displayId param)
  → frontend_api_handler.cpp: reads displayId from Point JSON (key "displayId", default UNASSIGNED=-1)
    → UiDriver: CheckDisplayExist(displayId) validates via DisplayManager
      → SysUiController:
        - GetUiWindows/GetWidgetsInWindow: ConvertAAMS(displayId) switches user if needed
        - InjectTouchEventSequence: pointerEvent->SetTargetDisplayId(displayId)
        - InjectMouseEventSequence: pointerEvent->SetTargetDisplayId(displayId)
        - InjectKeyEventSequence:KeyEvent::SetDisplayId(displayId)
```

**`UNASSIGNED` (-1) resolution**: When `displayId` is not specified (default `-1`), `GetValidDisplayId` (`system_ui_controller.cpp:659`) resolves it to `DisplayManager::GetDefaultDisplayId()` — typically display 0 (main screen). This applies to all touch/mouse/key injection and coordinate conversion.

**Cross-screen constraints for coordinate operations** (`CheckPointDisplayId` in `frontend_api_handler.cpp:1291`):

Operations involving two points (from/to) enforce that both points belong to the same display, unless explicitly allowed:

| Operation | Cross-screen allowed? | Behavior |
|-----------|-----------------------|----------|
| `swipe`/`swipeBetween`/`drag`/`dragBetween` (+ `*WithOptions`) | **No** | `ERR_INVALID_INPUT` if `from.displayId != to.displayId` |
| `longClickComponentPresent`/`dragComponentPresent` (swipe variants) | **No** | Same constraint |
| `penSwipe` | **No** | Same constraint |
| `mouseDrag`/`mouseDragWithOptions` | **Yes** | `allowCrossScreen=true`, points can be on different displays |
| Single-point ops (`clickAt`, `longClickAt`, `penClick`, etc.) | N/A | Single displayId, no constraint |

When one point has `displayId=UNASSIGNED` and the other has a specific value, the `UNASSIGNED` point inherits the other's displayId automatically (`CheckPointDisplayId:1293-1298`).

**Coordinate conversion** (`ConvertRelativeToGlobal` in `system_ui_controller.cpp:1329`): converts a per-display relative coordinate to a global screen coordinate using `DisplayManager::ConvertRelativeCoordinateToGlobal`. Required when the input subsystem expects global coordinates but the API received display-local coordinates.

**Per-display window cache** (`UiDriver::displayToWindowCacheMap_`): `UiDriver` maintains a `map<displayId, vector<WindowCacheModel>>` (`ui_driver.cpp:110`). `UpdateUIWindows(targetDisplay)` fetches windows for the specified display only (or all displays when `targetDisplay=-1`), and `DumpUiHierarchy`/`FindWidgets` look up the cache by `displayId` (`:120`, `:169`, `:236`). Widget operations retrieve the widget's `displayId` from its cached `Window` and pass it through for correct AAMS scoping.

**Input injection routing**: Touch/mouse/key events carry `displayId` via `PointerEvent::SetTargetDisplayId` / `KeyEvent::SetDisplayId`, so the input subsystem routes the event to the correct physical screen. This is independent of the AAMS user switching (above) — input injection works on any display regardless of which user's UI tree is currently connected.

## Running Tests

### Framework Unit Tests (C++ TDD)

**Important:** Run all commands from the OpenHarmony root directory.

**Build Tests:**
```bash
# Build all unittests
./build.sh --product-name rk3568 --build-target uitestkit_test

# Build a single unittest target (NOT the whole uitestkit_test group)
./build.sh --product-name rk3568 --build-target uitest_core_unittest
./build.sh --product-name rk3568 --build-target uitest_ipc_unittest
./build.sh --product-name rk3568 --build-target uitest_extension_unittest
```

**Run Tests:**
```bash
# Push test binary to device (paths relative to OpenHarmony root)
hdc file send out/rk3568/tests/unittest/arkxtest/uitest/uitest_core_unittest /data/local/tmp/

# Run tests
hdc shell ./uitest_core_unittest

# Run with filter
hdc shell ./uitest_core_unittest --gtest_filter=WidgetSelectorTest.*
```

**Test Targets:**
- `uitest_core_unittest` - Core logic (WidgetSelector, WidgetOperator, UiDriver, UiModel, etc.)
- `uitest_ipc_unittest` - IPC communication
- `uitest_extension_unittest` - Extension executor

Test sources live in `test/` — one `<classname>_test.cpp` per `ohos_unittest` target. New tests must be registered in `BUILD.gn` source lists (`testonly = true`, `module_out_path = "arkxtest/uitest"`).

### Application UI Tests (ArkTS)

**Important:** Run all build commands from the OpenHarmony root directory.

**Build Test Application:**
```bash
# From OpenHarmony root
# ArkTS-Dynamic
./test/xts/acts/build.sh product-name <product> system_size=standard suite=<testsuite_path>:<target>

# ArkTS-Static
./test/xts/acts/build.sh product-name <product> system_size=standard xts_suitetype=hap_static suite=<testsuite_path>:<target>

# Install HAP (output is in out/<product>/suites/haps/ relative to OpenHarmony root)
hdc install out/rk3568/suites/haps/<target>.hap
```

**Run Tests:**
```bash
# ArkTS-Dynamic
hdc shell aa test -p <bundleName> -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <className>

# ArkTS-Static
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <className>
```

## Adding New APIs

**Workflow:**

1. **Define API in `core/frontend_api_defines.h`**
   - Add enumerators to `FrontendApiType`
   - Define JSON request/response schemas

2. **Implement Server Logic in `core/frontend_api_handler.cpp`**
   - Add handler function mapped to API type
   - Call operators or implement logic directly

3. **Add Client Bindings**
   - **ArkTS-Static**: Add native method to `ets/ani/ets/@ohos.UiTest.ets`, implement in `ets/ani/src/uitest_ani.cpp`
   - **ArkTS-Dynamic**: Add to `napi/uitest_napi.cpp`

4. **Declare API** in `../../../../interface/sdk-js/api/@ohos.UiTest.d.ts`

5. **Update Documentation**
   - `../../../../docs/zh-cn/application-dev/application-test/uitest-guidelines.md`
   - `../../../../docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md`

## Directory Structure



```
uitest/
├── core/              # Core testing logic
│   ├── ui_driver.cpp/h           # Main driver
│   ├── ui_model.cpp/h            # Widget data model
│   ├── widget_selector.cpp/h     # Widget matching (40KB)
│   ├── widget_operator.cpp/h     # Widget operations
│   ├── window_operator.cpp/h     # Window management
│   ├── select_strategy.cpp/h     # Selection algorithms (38KB)
│   ├── frontend_api_handler.cpp  # API request dispatcher (108KB)
│   ├── dump_handler.cpp          # Layout dump
│   └── frontend_api_defines.h    # API definitions & enums
│
├── server/            # Server daemon
│   ├── server_main.cpp            # Entry point, shell handler
│   └── system_ui_controller.cpp/h # Server logic (63KB)
│
├── connection/        # IPC layer
│   └── ipc_transactor.cpp/h       # CommonEvent-based IPC
│
├── input/             # Input injection
│   └── ui_input.cpp/h            # Touch/key/mouse/pen events
│
├── record/            # Recording
│   ├── ui_record.cpp/h           # Recording controller
│   ├── pointer_tracker.cpp/h     # Gesture tracking
│   ├── keyevent_tracker.cpp/h    # Key event tracking
│   └── velocity_tracker.cpp/h    # Velocity calculation
│
├── addon/             # Extensions & screen capture
│   ├── extension_executor.cpp/h  # Extension executor
│   └── screen_copy.cpp/h         # Screen capture
│
├── ets/ani/           # ArkTS-Static bindings
│   ├── ets/@ohos.UiTest.ets      # ArkTS API (2020 lines)
│   └── src/uitest_ani.cpp        # ANI implementation
│
├── napi/              # ArkTS-Dynamic bindings
│   ├── uitest_napi.cpp           # NAPI bindings (30KB)
│   └── ui_event_observer_napi.cpp/h
│
├── cj/                # Cangjie FFI bindings
│   ├── uitest_ffi.cpp            # FFI bindings
│   └── ui_event_observer_impl.cpp/h
│
├── test/              # Unit tests
│   ├── ui_driver_test.cpp        # Driver tests
│   ├── widget_selector_test.cpp  # Widget matching tests
│   ├── widget_operator_test.cpp  # Widget operation tests
│   ├── frontend_api_handler_test.cpp # API handler tests
│   ├── select_strategy_test.cpp  # Selection algorithm tests
│   └── ...
│
└── BUILD.gn           # Build configuration
```

## Key Files Reference

| Component | Path |
|-----------|------|
| Core logic | `core/` |
| Server | `server/system_ui_controller.cpp` |
| IPC | `connection/ipc_transactor.cpp` |
| ArkTS API | `ets/ani/ets/@ohos.UiTest.ets` |
| ANI bindings | `ets/ani/src/uitest_ani.cpp` |
| NAPI bindings | `napi/uitest_napi.cpp` |
| CJ bindings | `cj/uitest_ffi.cpp` |
| Input | `input/ui_input.cpp` |
| Recording | `record/` |
| Screen capture | `addon/screen_copy.cpp` |
| Tests | `test/` |
| Build config | `BUILD.gn` |

## Shell Commands
                                       
During app development, if you need to quickly perform operations such as screen capture, screen recording, injecting UI simulations, or obtaining the widget tree, you can use shell commands to more conveniently complete
the corresponding tests. After uitest development, the following shell commands need to be verified to prevent newly developed content from affecting existing functionality.        
  
**Location:** `/system/bin/uitest`

**1. Screen Capture**
```bash
hdc shell uitest screenCap -p /data/local/tmp/screenshot.png [-d <displayId>]
```

**2. Dump Layout**
```bash
# Dump all windows
hdc shell uitest dumpLayout -p /data/local/tmp/layout.json

# With extended attributes
hdc shell uitest dumpLayout -p /data/local/tmp/layout.json -a

# Specific window
hdc shell uitest dumpLayout -p /data/local/tmp/app.json -b com.example.app

# Custom attribute
hdc shell uitest dumpLayout -p /data/local/tmp/custom.json -e customAttr
```

**API Usage:**
The dumpLayout API (API 26.0.0) supports the format:
- `dumpLayout(savePath: string, displayId?: int)` - Saves layout to specified path (converted to file descriptor internally), with optional display ID parameter

Options: `-i` (no merge), `-a` (include font attrs), `-b` (bundleName), `-w` (windowId), `-m` (merge), `-d` (displayId), `-e` (custom attr)

**3. Start Daemon**
```bash
hdc shell uitest start-daemon <token>
# token format: bundleName@pid@uid@area
```

**4. Record UI Operations**

- Start recording, Write Ui event information into csv file
```bash
hdc shell uitest uiRecord record -W true -l
```
Options: `-W` <true/false> (whether save widget information, true means to save), 
         `-l` (Save the current layout information after each operation)
         `-c` <true/false> (whether print the Ui event information to the console, true means to print)

- Read recorded file, print file content to the console
```
hdc shell uitest uiRecord read
```

**5. Inject Input**
```bash
# Click
hdc shell uitest uiInput click 500 1000

# Swipe
hdc shell uitest uiInput swipe 100 1000 900 1000 600

# Key event
hdc shell uitest uiInput keyEvent Back

# Combined keys (Ctrl+C)
hdc shell uitest uiInput keyEvent 17 67

# Input text
hdc shell uitest uiInput inputText 500 1000 "Hello"
```
Command Options:
  │ Command     │ Required │ Description                            │
  │ help        │ Yes      │ Help information for uiInput commands. │                                                  
  │ click       │ Yes      │ Simulate single click operation.       │                                                  
  │ doubleClick │ Yes      │ Simulate double click operation.       │                                                  
  │ longClick   │ Yes      │ Simulate long press operation.         │                                                  
  │ fling       │ Yes      │ Simulate fling operation.              │                                                  
  │ swipe       │ Yes      │ Simulate swipe operation.              │                                                  
  │ drag        │ Yes      │ Simulate drag operation.               │                                                  
  │ dircFling   │ Yes      │ Simulate directional swipe operation.  │                                                  
  │ inputText   │ Yes      │ Simulate text input operation at specified coordinates.│
  │ text        │ Yes      │ Simulate text input operation at current focused location, without specifying coordinates.│
  │ keyEvent    │ Yes      │ Simulate physical key events (e.g., keyboard, power key, back, home) and combined key operations. │
  

## Widget Hierarchy

Format: `<root>/<window>/<widget>/<child>/...`

Example: `root/0/1/3/5`

Built by `WidgetHierarchyBuilder` in `ui_model.cpp`

## Product-Specific Features

| Flag | Description |
|------|-------------|
| `ARKXTEST_TABLET_FEATURE_ENABLE` | Tablet features |
| `ARKXTEST_PC_FEATURE_ENABLE` | PC features (window ops) |
| `ARKXTEST_WATCH_FEATURE_ENABLE` | Watch features (crown rotation) |
| `ARKXTEST_KNUCKLE_ACTION_ENABLE` | Knuckle gestures |
| `ARKXTEST_ADJUST_WINDOWMODE_ENABLE` | Window mode adjustment |
| `ARKXTEST_TRIGGER_PEN_KEY_ENABLE` | Pen key operations (`triggerPenKey`); pc/tablet/phone only |
| `HIDUMPER_ENABLED` | HiDumper integration |

## Dependencies

| Dependency | Purpose |
|------------|---------|
| `hilog:libhilog` | Logging |
| `json:nlohmann_json_static` | JSON parsing |
| `accessibility:accessibleability` | Accessibility (UI tree access) |
| `window_manager:libwm` | Window manager |
| `input:libmmi-client` | Input injection |
| `ipc:ipc_core` | IPC |
| `image_framework:image_native` | Image processing (screenshots) |
| `napi:ace_napi` | NAPI |
| `runtime_core:ani` | ANI |

## Important Context

1. **Testing Framework**: This is the test framework, not the object under test
2. **Multi-Language**: Features implemented in C++ must have bindings in ANI (ArkTS-Static) and NAPI (ArkTS-Dynamic)
3. **System Integration**: Deeply integrated with OpenHarmony system services (accessibility, window manager, input)
4. **IPC Communication**: All client operations go through IPC to server daemon
5. **API Version**: Module support API 8+, with milestone features at API 9, 10, 11, 18, 20, 22, 23, 25, 26

## Related Documentation

- Parent framework: `../CLAUDE.md`
- Usage guide: `../../../../docs/zh-cn/application-dev/application-test/uitest-guidelines.md`
- API reference: `../../../../docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md`
- API declaration: `../../../../interface/sdk-js/api/@ohos.UiTest.d.ts`
