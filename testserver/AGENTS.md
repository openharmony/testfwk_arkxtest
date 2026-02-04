# CLAUDE.md

This file provides guidance for Claude Code (claude.ai/code) when working in this code repository.

## Quick Start

### Repository Context

**Important:** testserver is located within the arkxtest framework, which is part of the OpenHarmony repository:

```
OpenHarmony/
├── build.sh              # Build script (run from here)
└── test/testfwk/
    └── arkxtest/
        └── testserver/    # ← You are here
            ├── src/       # Server implementation
            ├── init/      # Service initialization
            └── sa_profile/ # SA configuration
```

**All build commands must be run from the OpenHarmony root directory**, not from the testserver directory.

### Prerequisites
- OpenHarmony source code with build environment configured
- HDC tool for device communication
- Target device connected via HDC

## Project Overview

This is **testserver** - a system service belonging to the OpenHarmony testing framework (SA ID: 5502). It helps system built-in testing tools call high-permission restricted interfaces, including window management, clipboard operations, system event publishing, performance monitoring, and process control.

The service starts and stops on demand (starts when the first client connects) and automatically stops when all clients exit or developer mode is disabled.

## Architecture

### System SA Implementation

The service follows the OpenHarmony System Ability (SA) pattern:
- **Server** (`test_server_service`): Implements `TestServerInterfaceStub` and `SystemAbility`
- **Client** (`test_server_client`): Uses `ITestServerInterface` proxy to communicate via IPC

### IPC Interface Definition

Interfaces are defined using OpenHarmony IDL in `src/ITestServerInterface.idl`:
- `CreateSession`: Creates client caller instance, used to register client death callback on server
- `SetPasteData`: Sets clipboard content (conditional compilation: `ARKXTEST_PASTEBOARD_ENABLE`)
- `ChangeWindowMode/TerminateWindow/MinimizeWindow`: Manipulate windows via window manager service
- `PublishCommonEvent`: Publish system common events
- `FrequencyLock`: Lock CPU frequency via socperf client
- `SpDaemonProcess`: SmartPerf process control (start process, terminate process)
- `CollectProcessMemory/CollectProcessCpu`: Performance metrics (CPU, memory) collection via UCollectUtil
- `GetValueFromDataShare`: DataShare query (conditional: phone/tablet products only)

### Service Lifecycle

The service implements automatic lifecycle management:
1. **On-demand Start**: First client connection triggers `OnStart()`
2. **Caller Tracking**: `CreateSession()` increments caller count; death listener decrements count
3. **Auto Stop**: Service terminates when:
   - Caller count is zero and timeout expires (`CallerDetectTimer`)
   - Developer mode is disabled

### Conditional Compilation

Features are conditionally enabled based on product configuration:
- `ARKXTEST_PASTEBOARD_ENABLE`: Clipboard operations (requires `distributeddatamgr_pasteboard` component)
- `ARKXTEST_KNUCKLE_ACTION_ENABLE`: DataShare operations (phone/tablet only)

For feature dependencies, see the conditional compilation configuration in `src/BUILD.gn` file (search for macro definitions with `ARKXTEST_` prefix).

### Error Handling

All error codes are defined in `src/utils/test_server_error_code.h` with `TEST_SERVER_` prefix. Please use these error codes for consistency rather than raw integers.

### Common Error Codes

| Error Code | Name | Description |
|------------|------|-------------|
| 0 | TEST_SERVER_OK | Operation successful |
| -1 | TEST_SERVER_GET_INTERFACE_FAILED | Failed to get SA interface |
| 19000001 | TEST_SERVER_ADD_DEATH_RECIPIENT_FAILED | Failed to add death recipient |
| 19000002 | TEST_SERVER_CREATE_PASTE_DATA_FAILED | Failed to create pasteboard data |
| 19000003 | TEST_SERVER_SET_PASTE_DATA_FAILED | Failed to set pasteboard data |
| 19000004 | TEST_SERVER_PUBLISH_EVENT_FAILED | Failed to publish common event |
| 19000005 | TEST_SERVER_SPDAEMON_PROCESS_FAILED | SmartPerf process operation failed |
| 19000006 | TEST_SERVER_COLLECT_PROCESS_INFO_FAILED | Failed to collect process info |
| 19000007 | TEST_SERVER_OPERATE_WINDOW_FAILED | Window operation failed |
| 19000008 | TEST_SERVER_DATASHARE_FAILED | DataShare query failed |

## Main Dependencies

- `safwk:system_ability_fwk`: System ability framework
- `ipc:ipc_core`: Inter-process communication
- `samgr:samgr_proxy`: System ability manager
- `window_manager:session_manager_lite`: Window management
- `common_event_service:cesfwk_innerkits`: Event publishing
- `hiview:libucollection_utility`: Performance monitoring (CPU/memory collection)
- `soc_perf:socperf_client`: CPU frequency control
- `pasteboard:pasteboard_client` (optional): Clipboard operations
- `data_share:datashare_consumer` (optional): DataShare queries

## Configuration Files

- `init/testserver.cfg`: Service startup configuration (defines permissions, `uid/gid`, capabilities)
- `sa_profile/5502.json`: SA configuration file (defines `libpath`, auto-restart policy, on-demand stop conditions)

## Build System

This project uses **GN (Generate Ninja)** as the build system, integrated into the OpenHarmony build system.

**Important:** All build commands must be executed from the **OpenHarmony source code root directory**, not from the testserver directory. Build artifacts are located in `out/<product>/testfwk/arkxtest/` relative to the OpenHarmony root.

### Build Commands

```bash
# Navigate to OpenHarmony root first
cd /path/to/openharmony

# Build server and client libraries
./build.sh --product-name <product> --build-target test_server_service
./build.sh --product-name <product> --build-target test_server_client

# Build unit tests
./build.sh --product-name <product> --build-target testserver_unittest

# Build Fuzz tests
./build.sh --product-name <product> --build-target testserver_fuzztest
```

### Build Artifacts Location

- Server library: `out/<product>/testfwk/arkxtest/libtest_server_service.z.so`
- Client library: `out/<product>/testfwk/arkxtest/libtest_server_client.z.so`
- testserver.cfg: `out/<product>/obj/test/testfwk/arkxtest/testserver/init/testserver.cfg`
- 5502.json: `out/<product>/obj/test/testfwk/arkxtest/testserver/sa_profile/profiles/5502.json`

- Unit tests: `out/<product>/tests/unittest/arkxtest/testserver/testserver_unittest`

- Fuzz tests: `out/<product>/tests/fuzztest/arkxtest/testserver/*FuzzTest`

### Device Installation Location

- Server library: `/system/lib/libtest_server_service.z.so`
- Client library: `/system/lib/libtest_server_client.z.so`
- Service process startup config (testserver.cfg): `/system/etc/init/testserver.cfg`
- SA config file (5502.json): `/system/profile/testserver.json`

## Device Deployment

### Deploy Service Configuration

Build artifacts location: `out/<product>/testfwk/arkxtest/` (relative to OpenHarmony root)

```bash
# From OpenHarmony root
# Mount system partition
hdc target mount
hdc shell mount -o rw,remount /

# Push SA configuration file
hdc file send out/<product>/obj/test/testfwk/arkxtest/testserver/sa_profile/profiles/5502.json /system/profile/testserver.json

# Push service startup config
hdc file send out/<product>/obj/test/testfwk/arkxtest/testserver/init/testserver.cfg /system/etc/init/testserver.cfg

# Push server and client libraries
hdc file send out/<product>/testfwk/arkxtest/libtest_server_service.z.so /system/lib/libtest_server_service.z.so
hdc file send out/<product>/testfwk/arkxtest/libtest_server_client.z.so /system/lib/libtest_server_client.z.so
```

### Verify Service Status

```bash
# Check if SA 5502 is registered
hdc shell smgr list | grep 5502

# Check service logs
hdc shell hilog | grep TestServer
```

### Test Commands

```bash
# Run unit tests
hdc file send testserver_unittest /data/local/tmp/
hdc shell chmod +x /data/local/tmp/testserver_unittest
hdc shell /data/local/tmp/testserver_unittest

# Run with specific test filter
hdc shell /data/local/tmp/testserver_unittest --gtest_filter=ServiceTest.*
```

## Adding New IPC Methods

1. Add method signature in `src/ITestServerInterface.idl`
2. Implement in `src/service/test_server_service.h` and `.cpp` files
3. Add wrapper client interface in `src/client/test_server_client.h` and `.cpp` files
4. Update related unit test cases in `test/unittest/`
5. Rebuild to regenerate IPC Proxy/Stub code

**Note:** IPC Proxy/Stub code is automatically generated during compilation and archived in the `out/<product>/gen/test/testfwk/arkxtest/testserver/src` directory. Manual editing is not allowed.

## Troubleshooting

### Service Not Starting

**SA not registered**: Verify 5502.json is in `/system/profile/testserver.json`
```bash
cat /system/profile/testserver.json
```

**Permission denied**: Check testserver.cfg has correct uid/gid
```bash
cat /system/etc/init/testserver.cfg
```

**Missing dependencies**: Verify all required OpenHarmony components are installed

**Check service logs**
```bash
hdc shell hilog | grep C03110
```

**Verify developer mode**
```bash
hdc shell param get const.security.developermode.state
```
Should return `true`. If not, enable developer mode in settings.

### Client Connection Issues

**Access denied**: Verify ACL permissions in SA profile
```bash
cat /system/profile/testserver.json
```

**Version mismatch**: Ensure client and server are built from same source

### Build Issues

**GN not found**: Ensure OpenHarmony build environment is properly configured

**build.sh not found**: You must run commands from OpenHarmony root directory
```bash
# First navigate to OpenHarmony root
cd /path/to/openharmony
# Then run build commands
./build.sh --product-name <product> --build-target test_server_service
```

### Common Issues

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| Returns TEST_SERVER_GET_INTERFACE_FAILED (-1) | SA not started or registration failed | Check if developer mode is enabled, view hilog logs |
| Returns 19000002/19000003 (pasteboard related) | pasteboard service unavailable | Check `ARKXTEST_PASTEBOARD_ENABLE` conditional compilation config |
| Returns 19000007 (window operation failed) | window manager service exception | Confirm window_manager service is running normally |
| Returns 19000008 (DataShare failed) | DataShare service unavailable | Check if product configuration supports DataShare |
| Service not found by samgr | SA config file missing or incorrect | Verify 5502.json exists in /system/profile/testserver.json |
| Permission denied errors | testserver.cfg has wrong uid/gid | Check init/testserver.cfg configuration |
