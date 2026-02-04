# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

**JsUnit (Hypium)** is the JavaScript/ArkTS unit testing framework component of arkxtest, providing comprehensive test execution capabilities for OpenHarmony applications and system interfaces. It serves as the foundation for writing and executing unit tests with rich assertion libraries, mocking capabilities, and data-driven testing support.

## Quick Start

### Repository Context

**Important:** jsunit is located within the arkxtest framework, which is part of the OpenHarmony repository:

```
OpenHarmony/
├── build.sh              # Build script (run from here)
├── test/xts/acts/        # XTS test suites
└── test/testfwk/
    └── arkxtest/
        └── jsunit/       # ← You are here
            ├── src/
            ├── src_static/
            ├── index.js
            └── package.json
```

**All build commands must be run from the OpenHarmony root directory**, not from the jsunit directory.

### Using the Framework

The jsunit framework is distributed as an npm package and integrated directly into test applications:

```bash
# Install via OHPM
ohpm i @ohos/hypium
```

For building test applications that use jsunit, see the "Running Tests" section below.

## Installation and Usage

### Installation

```bash
ohpm i @ohos/hypium
```

Or add to `oh-package.json5`:
```json
"dependencies": {
    "@ohos/hypium": "1.0.25"
}
```

### Basic Usage

```javascript
import { describe, it, expect, beforeAll, afterAll } from '@ohos/hypium';

export default function abilityTest() {
  describe('ActsAbilityTest', function () {
    it('assertContain_success', 0, function () {
      let a = 'abc'
      let b = 'b'
      expect(a).assertContain(b)
    })
  })
}
```

## Architecture

### Core Components

**Entry Point (`index.js`)**
- `Hypium` class - Main test orchestrator
- `hypiumTest()` - Standard test execution
- `hypiumInitWorkers()` - Parallel test execution with workers
- `hypiumWorkerTest()` - Individual worker execution
- Custom assertion registration via `registerAssert()`

**Core Engine (`src/core.js`)**
- Singleton pattern: `Core.getInstance()`
- Service registry for managing test services
- Event-driven architecture (publish/subscribe)
- Global API injection into test scope

**Service Layer (`src/service.js`)**

| Service | Responsibility |
|---------|---------------|
| `SuiteService` | Test suite organization, nested suites, hooks, stress testing, random execution |
| `SpecService` | Individual test case management, filtering, skipping, parameter handling |
| `ExpectService` | Assertion engine with 25+ methods, custom matchers, promise support |
| `ReportService` | Test logging, real-time reporting, summary generation, timing metrics |

### Module Organization

**Assert Module (`src/module/assert/`)**
- 25+ assertion methods: `assertTrue`, `assertEqual`, `assertClose`, `assertContain`, `assertNaN`, etc.
- Promise assertions: `assertPromiseIsResolved`, `assertPromiseIsRejected`
- Deep equality utilities for complex object comparison
- Extensible custom assertion system

**Mock Module (`src/module/mock/`)**
- `MockKit` - Function stubbing with `when()`/`afterReturn()`/`afterThrow()`
- Property mocking capabilities
- Method call verification
- Argument matchers for flexible expectations

**Config Module (`src/module/config/`)**
- `DataDriver` - Data-driven testing (suite/test-level parameters)
- `FilterService` - Test filtering and selection
- `ConfigService` - Test configuration management

**Report Module (`src/module/report/`)**
- `OhReport` - OpenHarmony-specific reporting
- `LogExpectError` - Error formatting
- `ReportExtend` - Report customization

### Multi-Language Support

| Environment | Binding Type | Location |
|-------------|--------------|----------|
| ArkTS-Dynamic | NAPI | `index.js` |
| ArkTS-Static | ANI | `index.ets`, `src_static/` |

Both environments provide the same unified API surface.

## Test Execution Flow

1. **Initialization** - Core service setup, event system registration, global API injection
2. **Discovery** - Suite registration via `describe()`, test case registration via `it()`
3. **Execution** - Hook execution (beforeAll, beforeEach, etc.), test cases with timeout handling, async support
4. **Reporting** - Real-time progress logging, final summary generation

## Features

### Basic Test Process
- `describe()` - Define test suites
- `it()` - Define test cases
- Hooks: `beforeAll`, `afterAll`, `beforeEach`, `afterEach`
- Extended hooks: `beforeEachIt`, `afterEachIt`, `beforeItSpecified`, `afterItSpecified`

### Assertion Library (25+ methods)

**Basic:** `assertTrue`, `assertFalse`, `assertEqual`, `assertFail`
**Comparison:** `assertClose`, `assertLarger`, `assertLess`, `assertLargerOrEqual`, `assertLessOrEqual`
**Type/Null:** `assertInstanceOf`, `assertNull`, `assertUndefined`, `assertNaN`
**Collection:** `assertContain`
**Promise:** `assertPromiseIsResolved`, `assertPromiseIsRejected`, `assertPromiseIsResolvedWith`, `assertPromiseIsRejectedWith`
**Special:** `assertNegUnlimited`, `assertPosUnlimited`, `assertDeepEquals`
**Modifiers:** `.not()`, `.message()`

### Mock Capabilities
- Function stubbing: `when(mock.func()).afterReturn(value)`
- Throw exceptions: `when(mock.func()).afterThrow(error)`
- Call verification: `verify(mock.func()).times(count)`
- Property mocking
- Argument matchers

### Advanced Features
- **Data-Driven Testing** - Parameterized tests with different datasets
- **Stress Testing** - Repeat test execution for performance validation
- **Random Execution** - Test order randomization for flake detection
- **Parallel Execution** - Worker-based parallel test execution
- **Async/Await** - Native Promise handling
- **Test Filtering** - Filter by test type, size, level
- **Skip Tests** - `xdescribe`, `xit` for temporary exclusion
- **Timeout Configuration** - Per-test or global timeout settings

### Test Configuration

**Test Types (bitmask flags):**
`FUNCTION`, `PERFORMANCE`, `POWER`, `RELIABILITY`, `SECURITY`, `GLOBAL`, `COMPATIBILITY`, `USER`, `STANDARD`, `SAFETY`, `RESILIENCE`

**Test Sizes:**
`SMALLTEST`, `MEDIUMTEST`, `LARGETEST`

**Test Levels:**
`LEVEL0` through `LEVEL4`

## Running Tests

### Application Tests (ArkTS)

**Important:** All build commands below must be executed from the **OpenHarmony source code root directory**, not from the jsunit directory. Build outputs are located in `out/<product>/` in the OpenHarmony root.

**Build Test Application:**
```bash
# Navigate to OpenHarmony root first
cd /path/to/openharmony

# ArkTS-Dynamic
./test/xts/acts/build.sh product-name <product> system_size=standard suite=<testsuite_path>:<target>

# ArkTS-Static
./test/xts/acts/build.sh product-name <product> system_size=standard xts_suitetype=hap_static suite=<testsuite_path>:<target>
```

**Install HAP:**
```bash
# Output is in out/<product>/suites/haps/ relative to OpenHarmony root
hdc install out/rk3568/suites/haps/<target>.hap
```

**Run Tests:**
```bash
# ArkTS-Dynamic
hdc shell aa test -p <bundleName> -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <className>

# ArkTS-Static
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <className>
```

## Important Context

1. **Dual Environment Support**: All features must work in both ArkTS-Dynamic (NAPI) and ArkTS-Static (ANI) environments
2. **Event-Driven Architecture**: Loose coupling between components via publish/subscribe pattern
3. **Singleton Pattern**: Core uses singleton pattern for centralized state management
4. **Global API Injection**: Test APIs are injected into the global scope during test execution
5. **Service Registry**: All major functionality is organized as services that can be independently managed
6. **Promise-Based Async**: Native async/await support with Promise assertions
7. **Module Version**: API support starts from API 8, with incremental features in subsequent versions
8. **Testing Framework**: This is the test framework itself, not an application under test

## Key Files

| File | Purpose |
|------|---------|
| `index.js` | Main entry point (ArkTS-Dynamic) |
| `index.ets` | Main entry point (ArkTS-Static) |
| `index.ts` | TypeScript definitions |
| `src/core.js` | Core engine and singleton |
| `src/service.js` | Service layer implementations |
| `src/module/assert/` | Assertion implementations |
| `src/module/mock/` | Mock framework |
| `src/module/config/` | Configuration and data-driven testing |
| `src/module/report/` | Reporting functionality |
| `src_static/` | ArkTS-Static specific implementations |
| `package.json` | NPM package configuration |
| `README.md` | Detailed feature documentation |

## Related Documentation

- **Root Documentation**: `../CLAUDE.md` - Overall arkxtest architecture
- **Uitest**: `../uitest/CLAUDE.md` - UI automation testing framework
- **PerfTest**: `../perftest/CLAUDE.md` - Performance testing framework
- **TestServer**: `../testserver/CLAUDE.md` - System ability service
- **Dependencies**: `../deps.md` - Complete list of external dependencies
- **Chinese README**: `../README_zh.md` - Detailed feature documentation in Chinese
