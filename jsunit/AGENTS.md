# JsUnit - AI Knowledge Base

## Overview

**JsUnit (Hypium)** is the JavaScript/ArkTS unit testing framework component of arkxtest, providing comprehensive test execution capabilities for OpenHarmony applications and system interfaces. It serves as the foundation for writing and executing unit tests with rich assertion libraries, mocking capabilities, and data-driven testing support.

## What This Package Is (and Is Not)

- **Pure npm package** — `package.json` has **no `scripts`** and **no `dependencies`**. There is no `bundle.json`, no GN build, no lint/typecheck/format tooling (`tsconfig.json`, `.eslintrc`, `.prettierrc` do not exist).
- **No standalone build/verification.** It is consumed by test applications via OHPM (`ohpm i @ohos/hypium`). Verification = a consuming app builds and runs.
- **Dual source trees** — every feature exists in both:
  - `src/` (`*.js`) — ArkTS-Dynamic (NAPI), entry `index.js`
  - `src_static/` (`*.ets`) — ArkTS-Static (ANI), entry `index.ets` / `src_static/hypium.ets`
  - The two MUST stay feature-parity. When editing one, check the other.

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
- `MockKit` - Function stubbing with `when()`/`afterReturn()`/`afterThrow()`, `mockObject()` for batch mocking, `mockPrivateFunc()` for private methods, `mockProperty()` for property mocking, `ignoreMock()` to restore
- `ArgumentMatchers` - 14 matchers: type matchers (`any*`/`not*`), fuzzy matchers (`matchRegexs`/`containSubstring`/`containValue`/`containSubArray` + negated forms)
- `VerificationMode` - Call count verification: `times`/`never`/`once`/`atLeast`/`atMost`
- Method call verification with matcher-based argument matching (`checkIsRightValue`)

**Config Module (`src/module/config/`)**
- `DataDriver` - Data-driven testing (suite/test-level parameters)
- `FilterService` - Test filtering and selection
- `ConfigService` - Test configuration management

**Report Module (`src/module/report/`)**
- `OhReport` - OpenHarmony-specific reporting
- `LogExpectError` - Error formatting
- `ReportExtend` - Report customization

**Failure Module (`src/module/failure/`)**
- `FailureCaptureService` - Auto screenshot + layout dump on test failure (enabled by `snapshotWhenFail`)

**Assert Module additions (`src/module/assert/`)**
- `assertMatchObj.js` - Partial object property matching with embedded matchers

### Multi-Language Support

| Environment | Binding Type | Location |
|-------------|--------------|----------|
| ArkTS-Dynamic | NAPI | `index.js` |
| ArkTS-Static | ANI | `index.ets`, `src_static/` |

Both environments provide the same unified API surface.

### Architecture map (dynamic ↔ static)

| Concept | Dynamic (`src/`) | Static (`src_static/`) |
|---------|------------------|------------------------|
| Entry | `index.js` — `Hypium`, `hypiumTest()`, workers | `index.ets`, `src_static/hypium.ets` |
| Type defs | `index.d.ts`, `index.ts` | interfaces inline in `.ets` |
| Core engine | `src/core.js` — singleton, service registry, event pub/sub | `src_static/core.ets` |
| Services | `src/service.js` (all four in one file) | `src_static/module/service/` (split per service) |
| Global API injection | `src/interface.js` — wraps core via `Reflect.has` | `src_static/interface.ets` |
| Assertions | `src/module/assert/` (`ExpectExtend.js`, `assertMatchObj.js`) | `src_static/module/assert/` |
| Mocking | `src/module/mock/` (`MockKit.js`, `ArgumentMatchers.js`, `VerificationMode.js`) | `src_static/module/mock/` |
| Config/filtering | `src/module/config/configService.js` | `src_static/module/config/configService.ets` |
| Failure capture | `src/module/failure/FailureCaptureService.js` | `src_static/module/...` |
| Test config enums | `src/Constant.js` — `TestType`/`Size`/`Level` (bitmask flags) | `src_static/Constant.ets` |

### Service & event architecture
- **Core** (`Core.getInstance()`) is the singleton hub. It holds `services` and `events` registries.
- Four core services: `SuiteService` (describe/hook orchestration), `SpecService` (it/case execution, skip, timeout), `ExpectService` (assertion engine + custom matchers), `ReportService` (logging/summary). Plus `ConfigService` for filtering.
- Events are a pub/sub bus keyed by `suite`/`spec`/`task`. `core.subscribeEvent('spec', reportService)` wires services to event sources. `core.fireEvents()` triggers them.
- Global test APIs (`describe`, `it`, `expect`, hooks) are injected via `core.addToGlobal()` → `globalThis`. The `interface.js` wrappers guard with `Reflect.has(core, name)` so they're safe before `core.init()`.

## Test Execution Flow

1. **Initialization** - Core service setup, event system registration, global API injection
2. **Discovery** - Suite registration via `describe()`, test case registration via `it()`
3. **Execution** - Hook execution (beforeAll, beforeEach, etc.), test cases with timeout handling, async support
4. **Reporting** - Real-time progress logging, final summary generation

## Code Style

**`src/` (`.js`, ArkTS-Dynamic):**
- **4-space indentation**; single quotes for strings; template literals for logs (`` `${TAG}...` ``).
- ES modules only — `export default` / `export { ... }`. Never `module.exports`.
- Loosely typed — parameters rarely have JSDoc; functions use `function (a, b)` without types.
- Classes with `static` methods/properties; singleton `Core.getInstance()`.

**`src_static/` (`.ets`, ArkTS-Static):**
- **2-space indentation**; **every `.ets` file starts with `'use static';`** on line 1.
- Full TS-style type annotations on everything: `let domain: int = 0x0000`, `static context: Map<string, AnyType>`.
- ArkTS-Static scalar types: `int`, `byte`, `short`, `double` (NOT plain `number`). Use `int[]` not `number[]`.
- Strict static semantics: no `any` (use `AnyType` from `module/types/common`), no `arguments` object, explicit interfaces for object shapes.

### Shared conventions (both trees)
- **Apache 2.0 license header** on every file: `Copyright (c) <year> Huawei Device Co., Ltd.` followed by the license text. Dynamic files say `Copyright (c) 2021-...`, static files often `2025`.
- **Imports**: grouped — external (`@ohos.*`) first, then local project modules. Default imports for single-export modules (`import Core from './core'`), named imports for multi-export.
- **`TAG` constant** = `'[Hypium]'` — prefix all `console.info`/log output with `` `${TAG}...` ``.
- **Exports**: aggregate at file end in a single `export { a, b, c };` block (see `index.js:248`).
- **Error handling**: assertions throw on failure; mock matchers throw `Error('not a regex')` etc. for bad input. No try/catch for control flow in normal code.
- **Naming**: PascalCase classes (`Hypium`, `Core`, `MockKit`, `ArgumentMatchers`); camelCase functions/vars (`hypiumTest`, `addService`); UPPER_CASE constants (`TAG`, `KEYSET`); UPPER_CASE static class fields (`TestType.FUNCTION`).
- **Services**: registered via `core.addService('name', service)` keyed by `service.id`; retrieved via `core.getDefaultService('name')`.

## Features

### Basic Test Process
- `describe()` - Define test suites
- `it()` - Define test cases
- Hooks: `beforeAll`, `afterAll`, `beforeEach`, `afterEach`
- Extended hooks: `beforeEachIt`, `afterEachIt`, `beforeItSpecified`, `afterItSpecified`

### Key APIs (for writing/modifying test helpers)
- **Test shape**: `export default function suiteFn() { describe('Suite', () => { it('case', 0, () => { expect(x).assertEqual(y) }) }) }`
- **Hooks**: `beforeAll`, `afterAll`, `beforeEach`, `afterEach`, plus `beforeEachIt`/`afterEachIt` (run around nested suites) and `beforeItSpecified(name, fn)`/`afterItSpecified(name, fn)`.
- **Skip**: `xdescribe`/`xit` for static skip; `throw new SkipError(msg)` for runtime conditional skip (marks spec as ignored).
- **Mock**: `new MockKit()` → `mocker.mockFunc(obj, obj.method)` → `when(stub)(args).afterReturn(val)` → `mocker.verify('method', [args]).times(n)` → `mocker.clear(obj)`. Also `mockPrivateFunc`, `mockProperty`, `mockObject`, `ignoreMock`.
- **Verification modes** (`VerificationMode.js`): `times(n)`/`never()`/`once()`/`atLeast(n)`/`atMost(n)`.
- **Custom assertions**: `Hypium.registerAssert(fn)` / `Hypium.unregisterAssert(fn)`.
- **Modifiers**: `.not()` (negate), `.message(msg)` (custom failure message) — chainable on any assertion.

### Assertion Library (25+ methods)

**Basic:** `assertTrue`, `assertFalse`, `assertEqual`, `assertFail`
**Comparison:** `assertClose`, `assertLarger`, `assertLess`, `assertLargerOrEqual`, `assertLessOrEqual`
**Type/Null:** `assertInstanceOf`, `assertNull`, `assertUndefined`, `assertNaN`
**Collection:** `assertContain`
**Promise:** `assertPromiseIsPending`, `assertPromiseIsResolved`, `assertPromiseIsRejected`, `assertPromiseIsResolvedWith`, `assertPromiseIsRejectedWith`, `assertPromiseIsRejectedWithError`
**Special:** `assertNegUnlimited`, `assertPosUnlimited`, `assertDeepEquals` (value-equal for objects/arrays), `assertThrowError`
**Object property matcher:** `assertMatchObj` — partial object matching with embedded matchers per property (see below)
**Modifiers:** `.not()`, `.message()`

### Mock Capabilities
- Function stubbing: `when(mock.func()).afterReturn(value)`
- Throw exceptions: `when(mock.func()).afterThrow(error)`
- Call verification: `verify(mock.func()).times(count)`
- Verification modes: `times(n)`, `never()`, `once()`, `atLeast(n)`, `atMost(n)` (`VerificationMode.js`)
- Private method / property mocking: `mockPrivateFunc`, `mockProperty`
- Argument matchers

**Verify with matchers**: `verify` matches recorded calls via `MockKit.checkIsRightValue` → `ArgumentMatchers.matcheReturnKey`. Matchers can be passed as arguments to `verify()` instead of exact values, enabling flexible call verification (e.g., `verify(mockFunc(ArgumentMatchers.anyString())).times(1)`).

### Advanced Features
- **Data-Driven Testing** - Parameterized tests with different datasets
- **Stress Testing** - Repeat test execution for performance validation
- **Random Execution** - Test order randomization for flake detection
- **Parallel Execution** - Worker-based parallel test execution
- **Async/Await** - Native Promise handling
- **Test Filtering** - Filter by test type, size, level
- **Skip Tests** - `xdescribe`, `xit` for temporary exclusion
- **Skip at custom position** - `throw new SkipError(message)` at any point in test body to skip the current spec (caught in `SpecService` execution, marked as skipped with reason). `SkipError` is exported from the framework.
- **Timeout Configuration** - Per-test or global timeout settings

### Test Configuration

**Test Types (bitmask flags):**
`FUNCTION`, `PERFORMANCE`, `POWER`, `RELIABILITY`, `SECURITY`, `GLOBAL`, `COMPATIBILITY`, `USER`, `STANDARD`, `SAFETY`, `RESILIENCE`

**Test Sizes:**
`SMALLTEST`, `MEDIUMTEST`, `LARGETEST`

**Test Levels:**
`LEVEL0` through `LEVEL4`

### Static-tree type notes (ArkTS-Static)
- Object-literal maps (`{ 'k': v }`) are NOT allowed — use `new Map<string, int>()` + `.set()` then `export` (see `Constant.ets:64`).
- `JSON.stringifyJsonElement`, `jsonx.JsonElement`, `$_iterator()` are ArkTS-Static JSON APIs (no plain `JSON.parse` of objects).
- `readonly` static fields: `private static readonly MAX_PARSE_DEPTH = 20;`.

### Test Tags & Filtering

**Custom tags on test cases**: `it(desc, filter, func, timeout, tag)` accepts an optional `tag` string parameter (`SpecService.it`, `service.js:912`). Tags use `|` as separator for multiple tags on a single case (e.g., `"smoke|regression"`).

**Tag-based filtering at runtime**: Pass `-s tag <expr>` via `aa test` command. The expression parser (`configService.setTestTag`, `configService.js:121`) supports:
- Comma `,` = OR: `"smoke,regression"` runs cases tagged with smoke OR regression
- Plus `+` = AND: `"smoke+p0"` runs cases tagged with BOTH smoke AND p0
- Combined: `"smoke+p0,regression"` = (smoke AND p0) OR regression

Cases without any tag are skipped when a tag filter is active (`filterWithTestTag`, `configService.js:284`).

**Other filter dimensions**: `class`/`notClass` (by fully-qualified suite/spec name), `suite`, `itName`, `filter` (nest name), `testType`, `level`, `size`, plus `runSkipped` (`all`/`skipped`) to selectively run skipped tests.

### ArgumentMatchers (Enhanced)

**Type matchers:** `any()`, `anyString()`, `anyBoolean()`, `anyNumber()`, `anyObj()`, `anyFunction()`
**Negated type matchers:** `notString()`, `notBoolean()`, `notNumber()`, `notObj()`, `notFunction()`

**Fuzzy matchers (partial/regex):**

| Matcher | Description |
|---------|-------------|
| `matchRegexs(regex)` | String must match the regex |
| `notMatchRegexs(regex)` | String must NOT match the regex |
| `containSubstring(sub)` | String must contain substring |
| `notContainSubstring(sub)` | String must NOT contain substring |
| `containValue(val)` | Array must contain the value |
| `notContainValue(val)` | Array must NOT contain the value |
| `containSubArray(arr)` | Array must contain all elements of sub-array |
| `notContainSubArray(arr)` | Array must NOT contain all elements of sub-array |

All matchers work in both `when()` stubbing and `verify()` call verification (`ArgumentMatchers.matcheReturnKey`, `ArgumentMatchers.js:130`).

### Object Property Matcher Assertion

**`expect(actual).assertMatchObj(expected)`** (`assertMatchObj.js`): partial object matching — only checks properties listed in `expected`, ignores extra properties in `actual`. Each property value in `expected` can be:
- A concrete value (exact equality check)
- An `ArgumentMatchers` matcher (e.g., `ArgumentMatchers.anyString()` — flexible per-property matching)
- A nested object (recursively matched, max depth 6)

Example:
```javascript
const user = { name: 'Alice', age: 30, email: 'a@b.com', role: 'admin' };
// Only verify name and age; ignore email/role
expect(user).assertMatchObj({ name: 'Alice', age: 30 });
// With matcher on one property
expect(user).assertMatchObj({ name: ArgumentMatchers.anyString(), age: 30 });
```

### Failure Capture (Screenshot + Layout Dump on Fail)

When enabled via command-line parameter `-s snapshotWhenFail true` (`configService.enableFailureCapture`, `configService.js:181`), the framework automatically captures diagnostic artifacts when a test case fails:

- **Screenshot**: `Driver.screenCap()` saves a PNG to `/data/storage/el2/base/<suite>_<spec>_<timestamp>.png`
- **Layout dump**: `Driver.dumpLayout()` saves a JSON to `/data/storage/el2/base/<suite>_<spec>_<timestamp>.json`

Implementation: `FailureCaptureService.captureOnFailure()` (`src/module/failure/FailureCaptureService.js`), triggered in `SpecService` catch block (`service.js:751`). File names are sanitized (special chars replaced with `_`). Capture errors are logged but do not affect test results.

### Mock workflow example
```javascript
const mocker = new MockKit();
const stub = mocker.mockFunc(obj, obj.method);
when(stub)('test').afterReturn('1');              // stub return value
when(stub)(ArgumentMatchers.anyString()).afterReturn('2'); // matcher-based stub
expect(obj.method('test')).assertEqual('1');      // assertion
mocker.verify('method', ['test']).once();         // call-count verification
mocker.verify('method', [ArgumentMatchers.anyString()]).times(2);
mocker.clear(obj);                                // restore originals
```

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

### Running a single suite / class / case

```bash
# Run ALL tests in a bundle (ArkTS-Dynamic)
hdc shell aa test -b <bundleName> -s unittest OpenHarmonyTestRunner

# Run a SINGLE suite/class — the key "single test" command
hdc shell aa test -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <SuiteName>

# Run a SINGLE test case within a suite
hdc shell aa test -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <SuiteName>#<caseName>

# ArkTS-Static adds -m entry
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <SuiteName>
```

**Runtime filters** (extra `-s` flags): `class`/`notClass` (fully-qualified `Suite#case`), `suite`, `itName`, `testType`, `level`, `size`, `runSkipped` (`all`|`skipped`), `timeout`, `stress`, `random`, `dryRun`, `snapshotWhenFail`.

### TestRunner

`aa test` uses `-s unittest <name>` to specify the test entry class. The name must match the testrunner file name, and the class name should match too.

**Name resolution**: three places must agree — file name = exported class name = command argument. The TestRunner class must implement the `TestRunner` interface (`onPrepare()` and `onRun()`), calling `Hypium.hypiumTest()` inside `onRun()` to start tests.

#### Dynamic vs Static project differences

| Dimension | Dynamic project | Static project |
|-----------|-----------------|----------------|
| testrunner path | `entry/src/ohosTest/ets/testrunner/` | `entry/src/main/src/testrunner/` |
| File type | `.ts` | `.ets` |
| Export syntax | `export default class` | `export class` (named export) |
| Module structure | Dual-module split (`main` + `ohosTest`) | Single embedded module (`main`) |
| module.json5 | `ohosTest/module.json5` (`name:"entry_test"`, `type:"feature"`) | `main/module.json5` (`name:"entry"`, `type:"entry"`) |
| Command arg | `-p <bundle>` (optional, auto-detects ohosTest module) | `-m entry` (required, matches module name) |

**Why `-m entry` is required for static**: the static project's `module.json5` has `"name":"entry"`, so the command must specify it explicitly; the dynamic project's `ohosTest` module is auto-detected, so `-m` can be omitted.

#### How to customize TestRunner

1. Create a Runner class (dynamic: `export default class` / static: `export class`), implementing `onPrepare()` + `onRun()`.
2. In `onRun()`, get the delegator and call `Hypium.hypiumTest(delegator, args, testsuite)`.
3. At runtime, use the matching class name: `-s unittest MyTestRunner`.

Reference templates: `src/testrunner/OpenHarmonyTestRunner.ts` (dynamic), `src_static/testrunner/OpenHarmonyTestRunner.ets` (static).

## Verification (how to check your change)
- **No local lint/build.** To verify a change, a consuming test app must build and run.
- For `.ets` static-tree edits, validate ArkTS-Static syntax with the ArkTS-Sta Playground skill (isolated snippet compile) before relying on a full app build.
- Confirm `src/` and `src_static/` edits stay feature-parity — the same behavior must exist in both trees.

## Important Context

1. **Dual Environment Support**: All features must work in both ArkTS-Dynamic (NAPI) and ArkTS-Static (ANI) environments
2. **Event-Driven Architecture**: Loose coupling between components via publish/subscribe pattern
3. **Singleton Pattern**: Core uses singleton pattern for centralized state management
4. **Global API Injection**: Test APIs are injected into the global scope during test execution
5. **Service Registry**: All major functionality is organized as services that can be independently managed
6. **Promise-Based Async**: Native async/await support with Promise assertions
7. **Module Version**: API support starts from API 8, with incremental features in subsequent versions
8. **SkipError**: Throwing `SkipError` at any position in test body skips the current spec with reason — enables conditional skip based on runtime checks
9. **Failure Capture**: Requires `@ohos.UiTest` Driver; enabled by `-s snapshotWhenFail true`; saves to `/data/storage/el2/base/`
10. **Testing Framework**: This is the test framework itself, not an application under test

## Key Files

| File | Purpose |
|------|---------|
| `index.js` | Main entry point (ArkTS-Dynamic) |
| `index.ets` | Main entry point (ArkTS-Static) |
| `index.d.ts`, `index.ts` | TypeScript definitions |
| `src/core.js` | Core engine and singleton |
| `src/service.js` | Service layer implementations (all four services) |
| `src/interface.js` | Global API injection (`describe`/`it`/`expect`/hooks) |
| `src/Constant.js` | `TestType`/`Size`/`Level` bitmask enums, `TAG`, `KEYSET` |
| `src/module/assert/ExpectExtend.js` | 25+ assertion implementations |
| `src/module/assert/assertMatchObj.js` | Partial object property matcher assertion |
| `src/module/mock/MockKit.js` | Function/object stubbing, verify with matchers |
| `src/module/mock/ArgumentMatchers.js` | 14 matchers (type, fuzzy regex/substring/array) |
| `src/module/mock/VerificationMode.js` | Call count verification modes |
| `src/module/config/configService.js` | Configuration, tag-based filtering, failure capture flag |
| `src/module/config/DataDriver.js` | Data-driven testing |
| `src/module/failure/FailureCaptureService.js` | Auto screenshot + layout dump on failure |
| `src/module/report/OhReport.js` | OpenHarmony-specific reporting |
| `src/module/kit/SysTestKit.js` | Log keyword detection, case metadata access |
| `src_static/` | ArkTS-Static specific implementations (mirror of `src/`) |
| `package.json` | NPM package config (no scripts, no dependencies) |
| `README.md` | Detailed feature & API documentation |

## Notes
- Keep `src/` and `src_static/` in sync when changing behavior.
- API support starts at API 8; milestone features tagged with version in README (e.g. `1.0.25`, `1.0.26`, `1.0.28`).
- See parent `../AGENTS.md` for the full arkxtest architecture and C++ build details.

## Related Documentation

- **Root Documentation**: `../CLAUDE.md` - Overall arkxtest architecture
- **Uitest**: `../uitest/CLAUDE.md` - UI automation testing framework
- **PerfTest**: `../perftest/CLAUDE.md` - Performance testing framework
- **TestServer**: `../testserver/CLAUDE.md` - System ability service
- **Dependencies**: `../deps.md` - Complete list of external dependencies
- **Chinese README**: `../README_zh.md` - Detailed feature documentation in Chinese
