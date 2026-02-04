# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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
2. **Component Operation**: `Component.click()` → IPC → Server → `WidgetOperator` → Input System

## Running Tests

### Framework Unit Tests (C++ TDD)

**Important:** Run all commands from the OpenHarmony root directory.

**Build Tests:**
```bash
# From OpenHarmony root
./build.sh --product-name rk3568 --build-target uitestkit_test
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
5. **API Version**: Module support API 8+, with milestone features at API 9, 10, 11, 18, 20, 22

## Related Documentation

- Parent framework: `../CLAUDE.md`
- Usage guide: `../../../../docs/zh-cn/application-dev/application-test/uitest-guidelines.md`
- API reference: `../../../../docs/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md`
- API declaration: `../../../../interface/sdk-js/api/@ohos.UiTest.d.ts`
