# TestHelper - AI Knowledge Base

## Overview

**TestHelper** is a C++ command-line tool (`/system/bin/testhelper`) component of arkxtest that exposes system-level test capabilities by delegating to the testserver SA (ID 5502) through `TestServerClient`. It supports setting/getting system time and timezone, clipboard operations, IME keyboard hiding, font management, dark/light mode switching, and GPS location mocking via GPX files.

## Quick Start

### Repository Context

**Important:** testhelper is located within the arkxtest framework, which is part of the OpenHarmony repository:

```
OpenHarmony/
├── build.sh              # Build script (run from here)
└── test/testfwk/
    └── arkxtest/
        └── testhelper/   # ← You are here
            ├── include/             # Headers
            ├── src/                 # Source files
            ├── test/unittest/       # gtest unit tests
            └── BUILD.gn             # Build configuration
```

**All build commands must be run from the OpenHarmony root directory**, not from the testhelper directory.

### Prerequisites
- OpenHarmony source code with build environment configured
- HDC tool for device communication
- Target device connected via HDC

## Build System

**Important:** All build commands must be executed from the **OpenHarmony source code root directory**. Build artifacts are located in `out/<product>/testfwk/arkxtest/` relative to the OpenHarmony root.

### Build Commands

```bash
# Navigate to OpenHarmony root first
cd /path/to/openharmony

# Build the binary + core library
./build.sh --product-name rk3568 --build-target testhelperkit

# Build unit tests
./build.sh --product-name rk3568 --build-target testhelper_unittest
```

### Key Build Targets

| Target | Description |
|--------|-------------|
| `testhelperkit` | Build group (binary + core library) |
| `testhelper` | Executable target (`/system/bin/testhelper`) |
| `testhelper_core` | Static library with handlers, utils, GPX parser |
| `testhelper_unittest` | gtest unit test binary |

### Build Configuration

- **Build System**: GN (Generate Ninja) with `BUILD.gn`
- **Feature Flags** (from `arkxtest_config.gni` via `arkxtest_product_feature`):
  - `ARKXTEST_FONT_ENABLE` - Font management (pc/tablet/phone with graphic_2d)
  - `ARKXTEST_PASTEBOARD_ENABLE` - Clipboard (when pasteboard part present)
- **Disabled on**: `watch`/`glasses` product features (`install_enable = false`)
- No standalone lint/typecheck step; correctness verified by building + running gtest

### Device Deployment

Build artifacts location: `out/<product>/testfwk/arkxtest/` (relative to OpenHarmony root)

```bash
# Mount filesystem as read-write (required for system partition writes)
hdc target mount
hdc shell mount -o rw,remount /

# Push binary
hdc file send out/rk3568/testfwk/arkxtest/testhelper /system/bin/testhelper
hdc shell chmod +x /system/bin/testhelper

# Verify
hdc shell testhelper --version
```

## Architecture

### Command Dispatch Model

```
CLI (argv)
  ↓
testhelper_main.cpp: g_commandHandlers lookup + arg count validation
  ↓
TestHelperCore::Handle*()
  ↓ Validate inputs + TestServerClient::GetInstance().<Call>()
TestServer SA (ID 5502)
```

### Core Components

**Entry Point (`src/testhelper_main.cpp`)**
- `main()` - CLI entry point, command dispatch
- `g_argCountMap` - Argument count validation (min/max/usage per command)
- `g_commandHandlers` - Lambda registry mapping command → handler

**Handler Layer (`src/testhelper_core.cpp`)**
- `TestHelperCore` - Per-command handlers (`HandleGetTime`, `HandleSetTime`, `HandleSetMockedLocations`, etc.)
- Input validation helpers: `ValidateTimeRanges`, `ValidateFontFileFormat`, `ValidateGPXFilePath`, `ValidateTimeInterval`
- `ParseTimeToMs` - Strict time string parsing (`YYYY-MM-DD HH:MM:SS`)

**GPX Parser (`src/gpx_parser.cpp`)**
- libxml2-based GPX 1.1 parser supporting `wpt`, `trk`/`trkpt`, `rte`/`rtept` elements
- `GPXElementData` struct holds parsed point attributes
- Validates coordinates (lat [-90,90], lon [-180,180]), speed (≥0), direction ([0,360])

**Utilities (`src/common_utils.cpp/.h`)**
- Logging macros: `LOG_I`/`LOG_D`/`LOG_W`/`LOG_E` (hilog-backed on device, no-op off-device)
- `PrintToConsole` - User-facing stdout output
- `SafeStoi` - Safe string-to-int conversion
- `GenLogTag` - Compile-time log tag generation from file/function names
- Exit/parse constants: `EXIT_SUCCESS(0)`, `EXIT_FAILURE(1)`, `EXIT_SERVICE_UNAVAILABLE(2)`, `EXIT_NOT_SUPPORTED(3)`, `PARSE_*`

### Implementation Details by Feature

#### Time Operations

**get-time (`HandleGetTime`)**: Gets `MiscServices::TimeServiceClient::GetInstance()` singleton, returns `EXIT_SERVICE_UNAVAILABLE` if null; calls `GetWallTimeMs(timeMs)` for wall-clock ms, divides by `MS_PER_SECOND(1000)` to seconds, formats via `strftime` to `YYYY-MM-DD HH:MM:SS` and outputs via `PrintToConsole`.

**set-time (`HandleSetTime`)**: First parses/validates the time string via `ParseTimeToMs` (see below), reporting format vs value errors separately on failure; dispatches via `TestServerClient::GetInstance().SetTime(timeMs)`, succeeds on `TEST_SERVER_OK`, otherwise `EXIT_SERVICE_UNAVAILABLE`.

**ParseTimeToMs (strict time parsing)**: Implements four-stage validation —
1. `std::regex_match` against `^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$` enforcing double-digit month/day/hour/min/sec;
2. `strptime` parse requiring the end pointer to point at `'\0'`;
3. `ValidateTimeRanges` range check (year ≥1970, month 1-12, day 1-31, hour 0-23, min/sec 0-59);
4. **mktime round-trip check**: converts `tm` via `mktime`→`localtime` back, comparing all fields to detect invalid dates (Feb30, Apr31, etc.); also caps `seconds ≤ MAX_TIMESTAMP_SECONDS(2038-01-19 03:14:07 UTC)`.

**get-timezone / set-timezone**: `get` via `TimeServiceClient::GetInstance().GetTimeZone`; `set` via `TestServerClient::SetTimezone`, distinguishing `TEST_SERVER_INVALID_TIMEZONE_ID` (returns `EXIT_FAILURE`) from service-unavailable.

#### Clipboard Operations

**get-pastedata / set-pastedata / clear-pastedata**: All via the corresponding `TestServerClient` methods, gated by `ARKXTEST_PASTEBOARD_ENABLE` (compiled when the part is present). `set-pastedata` pre-validates text length ≤128MB (`128*1024*1024`) locally, returning `EXIT_FAILURE` on overflow. On `TEST_SERVER_NOT_SUPPORTED`, returns `EXIT_FAILURE` with a device-not-supported message.

#### Keyboard Operation

**hide-keyboard (`HandleHideKeyboard`)**: Calls `TestServerClient::GetInstance().HideKeyboard()`, distinguishing three failures: `TEST_SERVER_NOT_SUPPORTED` (no IMF), `TEST_SERVER_NO_ACTIVE_IME` (no active input method) — both return `EXIT_FAILURE`; others return `EXIT_SERVICE_UNAVAILABLE`.

#### Font Management

All three commands (`get-fontname`/`install-font`/`uninstall-font`) are gated by `ARKXTEST_FONT_ENABLE`; when disabled the `#ifndef` branch returns `EXIT_NOT_SUPPORTED`.

**get-fontname (`HandleGetFontname`)**: Three-stage path validation —
1. Prefix must be `/data/local/tmp/` (`find(prefix) != 0` rejected);
2. `ValidateFontFileFormat` checks extension is `.ttf` or `.ttc`;
3. Filename after stripping prefix must not contain `..` (prevent traversal);
Then `open(O_RDONLY)`, `fdsan_exchange_owner_tag` to bind fd ownership; calls `Rosen::FontToolSet::GetInstance().GetFontFullName(fd)` to get font full names; finally `fdsan_close_with_tag` (closed on every branch). Returns the first font name.

**install-font (`HandleInstallFont`)**: Same path validation; opens fd only to verify readability then immediately `fdsan_close_with_tag`; actual install via `TestServerClient::InstallFont(fontPath)`, distinguishing `TEST_SERVER_FONT_ALREADY_INSTALLED`.

**uninstall-font (`HandleUninstallFont`)**: First checks `fontName` non-empty; via `TestServerClient::UninstallFont`, distinguishing `TEST_SERVER_FONT_NOT_FOUND`.

#### Dark/Light Mode

**set-viewmode (`HandleSetViewMode`)**: `mode` must be `"dark"` or `"light"`, else `EXIT_FAILURE`; first probes capability via `GetViewMode()` — `"NOT_SUPPORTED"` reports unsupported, a value equal to target `mode` reports already-set (both `EXIT_FAILURE`); then dispatches `SetViewMode(mode)`.

#### Location Mocking

**enable-location-mock / disable-location-mock**: Via `TestServerClient` `EnableLocationMock`/`DisableLocationMock`; `TEST_SERVER_NOT_SUPPORTED` means device unsupported.

**set-mocked-locations (`HandleSetMockedLocations`)**: Core flow —
1. `xmlInitParser()` initializes libxml2, `xmlCleanupParser()` called **on every return path**;
2. `ValidateGPXFilePath`: prefix `/data/local/tmp/`, extension `.gpx`, reject `..`, `access(F_OK)` existence check;
3. `ValidateTimeInterval`: interval must be 1-3600 seconds;
4. `GPXParser::ParseFile` parses location points;
5. Empty list or `> MAX_MOCK_LOCATIONS(1000)` errors;
6. `TestServerClient::SetMockedLocations(locations, timeInterval)` dispatches.

#### GPX Parser Implementation Details (`GPXParser`)

- `ParseFile`: `xmlReadFile` (`XML_PARSE_NOBLANKS | XML_PARSE_NONET`) reads the doc, `xmlDocGetRootElement` gets root; `ValidateGPXRoot` requires root element `gpx`; `ProcessGPXChildren` iterates children dispatching to `wpt`/`trk`/`rte` handlers; finally `xmlFreeDoc`.
- Element dispatch: `wpt`→`ProcessWaypoint`; `trk`→`ProcessTrack`→`trkseg`→`trkpt`; `rte`→`ProcessRoute`→`rtept`, all unified to `ProcessPoint`.
- `ProcessPoint`: gets `lat`/`lon` attributes via `xmlGetProp` then `xmlFree`; `ParsePointData` iterates child nodes for `ele`/`time` and `speed`/`course|direction` (supports both top-level and `<extensions>` containers via `strcasestr` fuzzy match).
- Numeric conversion uses local `SafeStod` (`strtod` + NaN check).
- `ValidateCoordinates`: lat∈[-90,90], lon∈[-180,180]; `ParseOptionalFields`: speed≥0, direction∈[0,360]; `ele` parse failure silently falls back to default altitude 0.
- `FillLocation` populates `TestServerLocation` (accuracy default 10.0, timeStamp/timeSinceBoot set 0, isFromMock set true).

#### CLI Dispatch Implementation Details (`testhelper_main.cpp`)

- `g_argCountMap`: `{command → (minArgs, maxArgs, usage)}`; `ValidateArgCount` calls `_Exit(EXIT_FAILURE)` on mismatch.
- `g_commandHandlers`: `{command → lambda(core, argc, argv)→int32_t}`.
- `set-time`: joins `argv[2..]` with spaces (accommodates shell splitting `"YYYY-MM-DD HH:MM:SS"`).
- `set-mocked-locations`: `argv[2]` is the path, optional `argv[3]` via `SafeStoi` for interval (default 1), non-integer errors out.

## Running Tests

### Framework Unit Tests (C++ gtest)

**Important:** Run all commands from the OpenHarmony root directory.

```bash
# Build (from OpenHarmony root)
./build.sh --product-name rk3568 --build-target testhelper_unittest

# Push test binary to device
hdc file send out/rk3568/tests/unittest/arkxtest/testhelper/testhelper_unittest /data/local/tmp/

# Run all tests
hdc shell /data/local/tmp/testhelper_unittest

# Run with filter (single test case / suite)
hdc shell /data/local/tmp/testhelper_unittest --gtest_filter=ParseTimeToMsTest.ValidTime_Normal
hdc shell /data/local/tmp/testhelper_unittest --gtest_filter=CommonUtilsTest.*
```

**Test Targets:**
- `testhelper_core_test.cpp` - Time parsing, clipboard, font, keyboard, timezone tests
- `common_utils_test.cpp` - `GenLogTag`, `PrintToConsole` utility tests

## Adding New Commands

1. Add handler `int32_t TestHelperCore::HandleXxx(...)` in `testhelper_core.h/.cpp` with input validation + `TestServerClient::GetInstance().<Call>()`.
2. Register argv contract in `g_argCountMap` and a lambda in `g_commandHandlers` in `testhelper_main.cpp`.
3. Add a line to `HELP_MSG`.
4. Add gtest cases in `test/unittest/testhelper_core_test.cpp` covering valid, invalid-format, boundary, and service-unavailable paths.

## Directory Structure

```
testhelper/
├── include/
│   ├── testhelper_core.h          # TestHelperCore class declaration
│   ├── common_utils.h             # Logging macros, constants, utilities
│   └── gpx_parser.h               # GPXParser class declaration
├── src/
│   ├── testhelper_main.cpp        # CLI entry, command dispatch
│   ├── testhelper_core.cpp        # Command handlers + validation
│   ├── common_utils.cpp           # PrintToConsole, SafeStoi
│   └── gpx_parser.cpp             # libxml2 GPX 1.1 parser
├── test/
│   └── unittest/
│       ├── testhelper_core_test.cpp
│       └── common_utils_test.cpp
└── BUILD.gn                        # Build configuration
```

## Key Files Reference

| Component | Path |
|-----------|------|
| CLI entry | `src/testhelper_main.cpp` |
| Command handlers | `src/testhelper_core.cpp` |
| Logging/constants | `src/common_utils.cpp` / `include/common_utils.h` |
| GPX parser | `src/gpx_parser.cpp` / `include/gpx_parser.h` |
| Tests | `test/unittest/` |
| Build config | `BUILD.gn` |

## Shell Commands

**Location:** `/system/bin/testhelper`

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

## Code Style

- **License**: Every `.cpp`/`.h` starts with Apache-2.0 header `Copyright (c) <year> Huawei Device Co., Ltd.`
- **Namespace**: All code lives in `namespace OHOS::testhelper { ... }` (4-space indent inside)
- **Naming**: Classes/structs PascalCase (`TestHelperCore`, `GPXParser`); methods PascalCase (`Handle*`, `Validate*`, `Parse*`); struct fields/locals lowercase; constants `constexpr` UPPER_SNAKE_CASE
- **Types**: Fixed-width types only (`int32_t`, `int64_t`, `uint8_t`, `size_t`); pass strings as `const std::string&`
- **Includes**: System (`<...>`) → third-party (`<libxml/...>`) → local (`"..."`) → feature-gated (`#ifdef ARKXTEST_FONT_ENABLE`)
- **Formatting**: 4-space indent, no tabs, opening brace on own line for functions, `} else if` on one line, ~120 char lines
- **Logging**: `LOG_I/D/W/E(fmt, args...)` with `%{public}s`/`%{public}d` specifiers; user output via `PrintToConsole()` (errors prefixed `"Error: "`); `LOG_TAG "TestHelper"`, `LOG_DOMAIN 0xD003130`
- **Error handling**: Handlers return `int32_t` exit codes; validate inputs before service calls; manage fds with fdsan; free libxml2 allocations (`xmlFree`/`xmlFreeDoc`/`xmlCleanupParser`) on every path; never throw exceptions

## Dependencies

| Dependency | Purpose |
|------------|---------|
| `hilog:libhilog` | Logging |
| `c_utils:utils` | Common utilities |
| `time_service:time_client` | Time/timezone service |
| `libxml2:libxml2` | GPX XML parsing |
| `ability_runtime:wantagent_innerkits` | Ability runtime |
| `ability_base:configuration` | Ability base |
| `graphic_2d:rosen_text` / `2d_graphics` | Font management (feature-gated) |
| `pasteboard:pasteboard_client` / `pasteboard_data` | Clipboard (feature-gated) |
| `../testserver/src:test_server_client` | TestServer SA client |

## Important Context

1. **CLI Tool**: This is a command-line interface that delegates to TestServer SA 5502, not a library
2. **Input Validation**: All inputs must be validated before service calls — path prefix (`/data/local/tmp/`), reject `..` traversal, check extensions (`.ttf`/`.ttc`/`.gpx`), check ranges
3. **Feature Gating**: Font and pasteboard features are conditionally compiled based on product features
4. **Resource Safety**: fdsan-managed file descriptors and libxml2 allocations must be cleaned on every exit path
5. **Time Parsing**: `ParseTimeToMs` enforces strict `YYYY-MM-DD HH:MM:SS` format with double-digit fields and validates via mktime round-trip
6. **GPX Limits**: Max 1000 mock locations, time interval 1-3600 seconds

## Related Documentation

- Parent framework: `../AGENTS.md`
- TestServer: `../testserver/` - System ability service (SA ID 5502)
- Dependencies: `../deps.md` - Complete list of external dependencies
