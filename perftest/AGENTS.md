# CLAUDE.md

This file provides guidance for Claude Code (claude.ai/code) when working in this code repository.

## Project Overview

This is **PerfTest** - a white-box performance testing framework for OpenHarmony application performance testing. It provides performance measurement capabilities to collect metrics such as code segment execution duration, CPU usage/load, memory consumption, list scrolling FPS, and application startup/page switching latency during test code execution or specific UI scenarios.

The framework supports both N-API (for ArkTS-Dynamic applications) and ANI (for ArkTS-Static applications) interfaces.

## Architecture

### Three-Layer Architecture

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

### Core Components

**Core Layer** (`core/`):
- `PerfTest`: Main test orchestration class, manages test execution lifecycle
- `PerfTestStrategy`: Test strategy object, defines metrics to test, actionCode, resetCode, iteration count, package name of app under test, and timeout for single code segment execution
- `FrontendApiHandler`: Manages API registration and parameter parsing
- `common utils`: Common utilities and API definitions

**Connection Layer** (`connection/`):
- `IApiCaller`: Manages IPC communication between client and server
- `ApiCallerProxy`: IPC client, initiates calls to server; also performs server crash detection, returns exception on client after server exits
- `ApiCallerStub`: IPC server, receives client requests; also performs client exit detection, server automatically exits after client disconnects

**Collection Layer** (`collection/`):
- `DataCollection`: Base class for all metric collectors
- Specialized collectors for each metric type (CPU, memory, duration, FPS, etc.)
- Uses HiSysEvent to obtain system event-based metrics
- Process monitoring via bundle name or PID

**Frontend Bindings**:
- `napi/`: N-API bindings for ArkTS-Dynamic interface; `CallbackCodeNapi` for calling back ArkTS code segments and waiting for completion
- `ani/`: ANI bindings for ArkTS-Static interface; `CallbackCodeAni` for calling back ArkTS code segments and waiting for completion

**Server Daemon** (`core/src/start_daemon.cpp`):
- Entry point for starting PerfTest as a standalone process
- Starts on-demand when clients connect
- Automatically stops when all clients disconnect

### Supported Metrics

The framework collects the following performance metrics (defined in `PerfMetric` enum):

- `DURATION`: Execution time of test code
- `CPU_LOAD`: System CPU load
- `CPU_USAGE`: Process CPU usage percentage
- `MEMORY_RSS`: Resident Set Size (actual physical memory occupied by the process, including private memory and complete shared library memory, shared libraries are counted multiple times)
- `MEMORY_PSS`: Proportional Set Size (proportionally allocated memory size, private memory counted in full, shared memory divided equally among sharing processes, more accurately reflects actual process memory usage)
- `APP_START_RESPONSE_TIME`: Application startup response latency
- `APP_START_COMPLETE_TIME`: Application startup completion latency
- `PAGE_SWITCH_COMPLETE_TIME`: Page/route switch completion latency
- `LIST_SWIPE_FPS`: Frame rate during list scrolling

### Capability Invocation Flow

1. **Client Initialization**: Frontend (N-API/ANI) creates `PerfTest` with `PerfTestStrategy` as parameter
2. **Connection**: Client calls interface provided by meta-capability to start server, and uses connection token to connect to server daemon via IPC
3. **Test Execution**: Server iterates test rounds, executes action code and collects metrics
4. **Callback Management**: Action code (JS/ArkTS functions) executed via callback mechanism
5. **Result Collection**: Each collector collects data per iteration
6. **Result Retrieval**: Client calls `getMeasureResult()` to get aggregated statistics
7. **Cleanup**: Release resources, destroy callbacks

### Important Design Patterns

**Strategy Pattern**: `PerfTestStrategy` encapsulates test configuration

**Factory Pattern**: `PerfTest::create()` creates instance from strategy

**Callback Pattern**: Action/reset code passed as callbacks from frontend to backend

**Template Method**: `DataCollection` base class defines collection lifecycle

**Proxy Pattern**: `ApiCallerProxy` provides transparent IPC communication

## Code Organization

### Source Files by Function

**Test Orchestration**:
- `core/src/perf_test.cpp`: Main test execution logic
- `core/src/perf_test_strategy.cpp`: Strategy configuration parsing
- `core/src/frontend_api_handler.cpp`: Frontend API registration and dispatch

**Data Collection**:
- `collection/src/duration_collection.cpp`: Obtain code segment execution duration
- `collection/src/cpu_collection.cpp`: Obtain CPU metrics (usage, load) via `/proc/stat`
- `collection/src/memory_collection.cpp`: Obtain memory metrics (PSS, RSS) via `/proc/[pid]/status`
- `collection/src/app_start_time_collection.cpp`: Obtain application startup latency (response latency, completion latency) via HiSysEvent
- `collection/src/page_switch_time_collection.cpp`: Obtain page switch latency (completion latency) via HiSysEvent
- `collection/src/list_swipe_fps_collection.cpp`: Calculate list scrolling FPS by capturing and parsing HiTrace during code segment execution

**IPC Communication**:
- `connection/src/ipc_transactor.cpp`: IPC serialization/deserialization
- `connection/src/api_caller_client.cpp`: Client connection management
- `connection/src/api_caller_server.cpp`: Server request processing

**Frontend Bindings**:
- `napi/src/perftest_napi.cpp`: N-API module registration and export
- `napi/src/callback_code_napi.cpp`: ArkTS code segment callback handling (calls user-defined actionCode, resetCode and waits for completion)
- `ani/src/perftest_ani.cpp`: ANI native method binding
- `ani/src/callback_code_ani.cpp`: ArkTS code segment callback handling (calls user-defined actionCode, resetCode and waits for completion)
- `napi/src/perftest_exporter.js`: JS-side initialization and daemon startup
- `ani/ets/@ohos.test.PerfTest.ets`: ArkTS-side API definition

**Server Daemon**:
- `core/src/start_daemon.cpp`: Daemon entry point, provides command to start perftest-daemon

## Build System

This project uses **GN (Generate Ninja)** as the build system, integrated into the OpenHarmony build system.

### Build Commands

```bash
# Build all PerfTest components (core, IPC, client, server, ANI)
./build.sh --product-name <product> --build-target perftestkit

# Build single component
./build.sh --product-name <product> --build-target perftest_server     # Executable daemon (perftest)
./build.sh --product-name <product> --build-target perftest_client     # N-API shared library (libperftest.z.so)
./build.sh --product-name <product> --build-target perftest_ani        # ANI shared library (libperftest_ani.so)
./build.sh --product-name <product> --build-target perftest_abc        # ABC module (@ohos.test.PerfTest.abc)

# Build unit tests
./build.sh --product-name <product> --build-target perftest_unittest
```

**Note:** If the `ani/ets/@ohos.test.PerfTest.ets` file is modified, you need to delete `out/<product>/obj/test/testfwk/arkxtest/perftest/@ohos.test.PerfTest.abc` before rebuilding for changes to take effect.

### Build Artifacts Location

- Executable daemon (perftest): `out/<product>/testfwk/arkxtest/perftest`
- N-API shared library (libperftest.z.so): `out/<product>/testfwk/arkxtest/libperftest.z.so`
- ANI shared library (libperftest_ani.so): `out/<product>/testfwk/arkxtest/libperftest_ani.so`
- ABC module (@ohos.test.PerfTest.abc): `out/<product>/obj/test/testfwk/arkxtest/perftest/@ohos.test.PerfTest.abc`

- Unit tests: `out/<product>/tests/unittest/arkxtest/perftest/perftest_unittest.txt`

### Device Installation Location

- Server daemon: `/system/bin/perftest`
- N-API client: `/system/lib/module/test/libperftest.z.so`
- ANI library: `/system/lib/libperftest_ani.so`
- ABC module: `/system/framework/@ohos.test.PerfTest.abc`

### Test Commands

```bash
# Run unit tests on device
hdc file send perftest_unittest /data/local/tmp/
hdc shell chmod +x /data/local/tmp/perftest_unittest
hdc shell /data/local/tmp/perftest_unittest

# Run with specific test filter
hdc shell /data/local/tmp/perftest_unittest --gtest_filter=PerfTestTest.*
```

### Test Result Interpretation

Unit tests use the Google Test framework for output:
- `[  PASSED  ]`: Test passed
- `[  FAILED  ]`: Test failed
- `[  RUN     ]`: Test currently running
- Runtime statistics are located at the end of output

Example output:
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

## Common Development Tasks

### Adding New Performance Metrics

1. Add enum value to `PerfMetric` in `collection/include/data_collection.h`
2. Create new collector class inheriting from `DataCollection`
3. Implement `StartCollection()` and `StopCollectionAndGetResult()`
4. Register in frontend class definition of `FrontendApiHandler`
5. Add TypeScript enum in `ani/ets/@ohos.test.PerfTest.ets`

### Modifying IPC Protocol

1. Update `ApiCallInfo` and `ApiReplyInfo` structures in `connection/include/common_utils.h`
2. Modify serialization in `connection/src/api_caller_client.cpp`
3. Modify deserialization in `connection/src/api_caller_server.cpp`
4. Update `OnRemoteRequest()` in `connection/src/ipc_transactor.cpp`

### Adding Server-side Interface

1. Add interface handling logic in `core/include/perf_test.h` and `.cpp` files
2. Add static method to handle interface call in `core/src/frontend_api_handler.cpp`, register interface via `AddHandler()` method

### Unit Testing

Unit tests use the Google Test framework. Test files are located in `test/unittest/`:

- `data_collection_test.cpp`: Data collector tests
- `ipc_transactor_test.cpp`: IPC communication tests
- `frontend_api_handler_test.cpp`: API registration and dispatch tests
- `perf_test_test.cpp`: Test orchestration tests

## Important Notes

### ArkTS Callback Method Handling

The framework uses a complex callback mechanism to execute JS/ArkTS code from the C++ backend:
- `actionCode`: User-defined test code segment to execute
- `resetCode`: Optional environment reset code segment between iterations
- Callbacks managed via `CodeCallbackContext` with thread synchronization
- Supports both N-API and ANI callbacks

### Thread Safety

- Server daemon runs in a separate process
- IPC calls are synchronous but may block
- Callback execution requires careful thread synchronization
- Uses `ThreadLock` to wait for async callback completion

### Callback Thread Safety Implementation Details

The framework uses the following mechanisms to ensure safe cross-thread callback calls:

**CodeCallbackContext Synchronization Primitives**:
- Uses `std::condition_variable` and `std::mutex` for thread synchronization
- `ThreadLock` class encapsulates wait/notification logic to prevent deadlocks

**N-API Thread-safe Calls**:
```cpp
// Cross-thread safe N-API callback call
napi_call_threadsafe_function(napi_threadsafe_function func, void* data, napi_threadsafe_function_call_mode mode)
```
- `mode` parameter controls call behavior (blocking/non-blocking)
- Uses `napi_tsblocking` mode to wait for callback completion

**ANI Thread-safe Calls**:
```cpp
// Cross-thread safe ANI callback call
ani_call_threadsafe_function(ani_threadsafe_function func, void* data, ani_threadsafe_function_call_mode mode)
```
- Similar mechanism to N-API, adapted for ArkTS-Static scenarios

**Timeout Protection**:
- Default wait time: 30 seconds
- Configurable via `PerfTestStrategy::timeout`
- Returns error after timeout to avoid infinite waiting

**Thread Model**:
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

### Error Handling

All functions use `ApiCallErr` structure for error reporting:
- `errorCode`: Numeric error code
- `errorMessage`: String error description

### System Dependencies

- Requires HiSysEvent service to obtain application startup/page switch events
- Uses `/proc` filesystem for CPU/memory metrics
- Depends on testserver SA (5502) to call high-permission verified interfaces (PerfTest process cannot directly apply for ACL permissions)
- Requires developer mode to access processes
