# JsUnit - AI 知识库

## 概述

**JsUnit (Hypium)** 是 arkxtest 的 ArkTS 单元测试框架组件，为 OpenHarmony 应用和系统接口提供全面的测试执行能力。它是编写和执行单元测试的基础，提供丰富的断言库、Mock 能力和数据驱动测试支持。

## 本包的性质（是什么，不是什么）

- **纯 npm 包** — `package.json` **无 `scripts`** 且**无 `dependencies`**。没有 `bundle.json`，没有 GN 构建，没有 lint/typecheck/format 工具（`tsconfig.json`、`.eslintrc`、`.prettierrc` 不存在）。
- **无独立构建/验证**。它通过 OHPM 被测试应用消费（`ohpm i @ohos/hypium`）。验证方式 = 消费方应用能构建并运行。
- **双源码树** — 每个特性同时存在于：
  - `src/`（`*.js`）— ArkTS-Dynamic（NAPI），入口 `index.js`
  - `src_static/`（`*.ets`）— ArkTS-Static（ANI），入口 `index.ets` / `src_static/hypium.ets`
  - 两者必须保持特性对等。编辑一侧时，必须检查另一侧。

## 快速开始

### 仓库位置

**重要：** jsunit 位于 arkxtest 框架内，属于 OpenHarmony 仓库：

```
OpenHarmony/
├── build.sh              # 构建脚本（从此处执行）
├── test/xts/acts/        # XTS 测试套
└── test/testfwk/
    └── arkxtest/
        └── jsunit/       # ← 当前位置
            ├── src/
            ├── src_static/
            ├── index.js
            └── package.json
```

构建命令须从 OpenHarmony 根目录执行（见根 `../AGENTS.md` § 构建和验证）。

### 使用框架

jsunit 框架以 npm 包形式分发，直接集成到测试应用中：

```bash
# 通过 OHPM 安装
ohpm i @ohos/hypium
```

构建使用 jsunit 的测试应用，请参见下文"运行测试"章节。

## 安装和使用

### 安装

```bash
ohpm i @ohos/hypium
```

或添加到 `oh-package.json5`：
```json
"dependencies": {
    "@ohos/hypium": "1.0.25"
}
```

### 基本用法

```arkts
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

## 知识路由

改动前按任务/路径/词汇先读对应章节；编辑前须声明 ① 任务属哪一行 ② 已读本文件对应章节 ③ 触发了哪些项目约束。

| 触发条件 | 主要文件（高频落点） | 先读本文件 |
| --- | --- | --- |
| 新增断言方法 | `src/module/assert/ExpectExtend.js`、`src_static/module/assert/` | § 断言库 |
| 新增 ArgumentMatchers | `src/module/mock/ArgumentMatchers.js`（`:130` matcheReturnKey） | § 参数匹配器（增强） |
| 新增 MockKit 机制 | `src/module/mock/MockKit.js`（`checkIsRightValue` `:192`） | § Mock 能力 + § Mock 工作流示例 |
| 新增过滤逻辑 | `src/module/config/configService.js`（`:121` setTestTag、`:182` enableFailureCapture、`:284` filterWithTestTag） | § 测试标签与过滤 |
| 自定义 TestRunner | `index.js`（Hypium 类 `:28`）、`src/testrunner/OpenHarmonyTestRunner.ts` | § TestRunner |
| 任务提到 `snapshotWhenFail` / 失败截图 / 扩展 Failure Capture | `src/module/failure/FailureCaptureService.js` | § 失败捕获 |
| 改 `src/`（.js 动态树）任一文件 | 对应 `src_static/`（.ets 静态树）文件 | § 架构映射（动态 ↔ 静态） + § 项目约束（双树对等） |
| 改 `src_static/`（.ets 静态树） | 对应 `src/`（.js 动态树）文件 | § 静态树类型说明 + § 代码风格 + § 项目约束（双树对等） |
| 任务提到 `assertMatchObj` / 部分对象匹配 | `src/module/assert/assertMatchObj.js` | § 对象属性匹配器断言 |
| 任务提到 `SkipError` / 条件跳过 | `src/service.js`（SkipError 类 `:82`、SpecService catch `:748`、判断 `:762`） | § 高级特性（跳过） |
| 任务提到 `DataDriver` / 数据驱动 / 参数化测试 | `src/module/config/DataDriver.js`、`src_static/module/config/DataDriver.ets` | § 高级特性（数据驱动测试） |
| 任务提到 `VerificationMode` / 调用次数验证 / `times`/`never`/`atLeast` | `src/module/mock/VerificationMode.js` | § Mock 能力（验证模式） |
| 任务提到 `registerAssert` / 自定义断言 | `index.js`（`Hypium.registerAssert` `:228`）、`src/module/assert/ExpectExtend.js` | § 关键 API + § 断言库 |

## 架构

### 核心组件

**入口点（`index.js`）**
- `Hypium` 类 - 主测试协调器
- `hypiumTest()` - 标准测试执行
- `hypiumInitWorkers()` - Worker 并行测试执行
- `hypiumWorkerTest()` - 单个 Worker 执行
- 通过 `registerAssert()` 注册自定义断言

**核心引擎（`src/core.js`）**
- 单例模式：`Core.getInstance()`
- 服务注册表，管理测试服务
- 事件驱动架构（发布/订阅）
- 全局 API 注入到测试作用域

**服务层（`src/service.js`）**

| 服务 | 职责 |
|------|------|
| `SuiteService` | 测试套件组织、嵌套套件、钩子、压力测试、随机执行 |
| `SpecService` | 单个测试用例管理、过滤、跳过、参数处理 |
| `ExpectService` | 断言引擎，25+ 方法，自定义匹配器，Promise 支持 |
| `ReportService` | 测试日志、实时报告、摘要生成、计时指标 |

### 模块组织

**断言模块（`src/module/assert/`）**
- 25+ 断言方法：`assertTrue`、`assertEqual`、`assertClose`、`assertContain`、`assertNaN` 等
- Promise 断言：`assertPromiseIsResolved`、`assertPromiseIsRejected`
- 复杂对象比较的深度相等工具
- 可扩展的自定义断言系统

**Mock 模块（`src/module/mock/`）**
- `MockKit` - 函数桩：`when()`/`afterReturn()`/`afterThrow()`，`mockObject()` 批量 Mock，`mockPrivateFunc()` 私有方法，`mockProperty()` 属性 Mock，`ignoreMock()` 恢复
- `ArgumentMatchers` - 14 种匹配器：类型匹配器（`any*`/`not*`），模糊匹配器（`matchRegexs`/`containSubstring`/`containValue`/`containSubArray` + 否定形式）
- `VerificationMode` - 调用次数验证：`times`/`never`/`once`/`atLeast`/`atMost`
- 基于匹配器的参数匹配方法调用验证（`checkIsRightValue`）

**配置模块（`src/module/config/`）**
- `DataDriver` - 数据驱动测试（套件/用例级参数）
- `FilterService` - 测试过滤和选择
- `ConfigService` - 测试配置管理

**报告模块（`src/module/report/`）**
- `OhReport` - OpenHarmony 特定报告
- `LogExpectError` - 错误格式化
- `ReportExtend` - 报告定制

**失败模块（`src/module/failure/`）**
- `FailureCaptureService` - 测试失败时自动截图 + 布局转储（通过 `snapshotWhenFail` 启用）

**断言模块扩展（`src/module/assert/`）**
- `assertMatchObj.js` - 带嵌入匹配器的部分对象属性匹配

### 多语言支持

| 环境 | 绑定类型 | 位置 |
|------|----------|------|
| ArkTS-Dynamic | NAPI | `index.js` |
| ArkTS-Static | ANI | `index.ets`、`src_static/` |

两种环境提供相同的统一 API 表面。

### 架构映射（动态 ↔ 静态）

| 概念 | 动态（`src/`） | 静态（`src_static/`） |
|------|----------------|----------------------|
| 入口 | `index.js` — `Hypium`、`hypiumTest()`、workers | `index.ets`、`src_static/hypium.ets` |
| 类型定义 | `index.d.ts`、`index.ts` | `.ets` 中的内联接口 |
| 核心引擎 | `src/core.js` — 单例、服务注册表、事件发布/订阅 | `src_static/core.ets` |
| 服务 | `src/service.js`（四个服务在一个文件） | `src_static/module/service/`（按服务拆分） |
| 全局 API 注入 | `src/interface.js` — 通过 `Reflect.has` 包装核心 | `src_static/interface.ets` |
| 断言 | `src/module/assert/`（`ExpectExtend.js`、`assertMatchObj.js`） | `src_static/module/assert/` |
| Mock | `src/module/mock/`（`MockKit.js`、`ArgumentMatchers.js`、`VerificationMode.js`） | `src_static/module/mock/` |
| 配置/过滤 | `src/module/config/configService.js` | `src_static/module/config/configService.ets` |
| 失败捕获 | `src/module/failure/FailureCaptureService.js` | `src_static/module/...` |
| 测试配置枚举 | `src/Constant.js` — `TestType`/`Size`/`Level`（位掩码标志） | `src_static/Constant.ets` |

### 高频/高风险改动路径

- `src/service.js`（SpecService/SuiteService 执行引擎，任何执行流程改动高风险）
- `src/module/config/configService.js`（过滤/失败捕获开关，影响运行时行为）
- `src/module/assert/ExpectExtend.js`（25+ 断言，新增须同步 `src_static/`）
- `index.js`（入口导出表面，影响所有消费方）
- `index.d.ts`（公共 API 类型声明，任何签名变更影响所有消费方编译）
- `src_static/hypium.ets`（ArkTS-Static 入口，静态树特性的聚合点）

### 服务与事件架构
- **Core**（`Core.getInstance()`）是单例枢纽。持有 `services` 和 `events` 注册表。
- 四个核心服务：`SuiteService`（describe/hook 编排）、`SpecService`（it/case 执行、跳过、超时）、`ExpectService`（断言引擎 + 自定义匹配器）、`ReportService`（日志/摘要）。另有 `ConfigService` 用于过滤。
- 事件是按 `suite`/`spec`/`task` 为键的发布/订阅总线。`core.subscribeEvent('spec', reportService)` 将服务连接到事件源。`core.fireEvents()` 触发事件。
- 全局测试 API（`describe`、`it`、`expect`、hooks）通过 `core.addToGlobal()` → `globalThis` 注入。`interface.js` 包装器通过 `Reflect.has(core, name)` 守卫，确保在 `core.init()` 之前调用是安全的。

## 测试执行流程

1. **初始化** - 核心服务设置、事件系统注册、全局 API 注入
2. **发现** - 通过 `describe()` 注册套件，通过 `it()` 注册测试用例
3. **执行** - 钩子执行（beforeAll、beforeEach 等），带超时处理的测试用例，异步支持
4. **报告** - 实时进度日志、最终摘要生成

## 代码风格

**`src/`（`.js`，ArkTS-Dynamic）：**
- **4 空格缩进**；字符串用单引号；日志用模板字符串（`` `${TAG}...` ``）。
- 仅 ES 模块 — `export default` / `export { ... }`。禁止 `module.exports`。
- 松散类型 — 参数很少有 JSDoc；函数使用 `function (a, b)` 不带类型。
- 类带 `static` 方法/属性；单例 `Core.getInstance()`。

**`src_static/`（`.ets`，ArkTS-Static）：**
- **2 空格缩进**；**每个 `.ets` 文件第 1 行必须是 `'use static';`**。
- 所有内容都有完整的 TS 风格类型注解：`let domain: int = 0x0000`、`static context: Map<string, AnyType>`。
- ArkTS-Static 标量类型：`int`、`byte`、`short`、`double`（不是普通 `number`）。用 `int[]` 而非 `number[]`。
- 严格静态语义：禁止 `any`（用 `AnyType`，来自 `module/types/common`），禁止 `arguments` 对象，对象形状需显式接口。

### 共享约定（两棵树）
- 每个文件都有 **Apache 2.0 许可证头**：`Copyright (c) <year> Huawei Device Co., Ltd.` 后跟许可证文本。动态文件写 `Copyright (c) 2021-...`，静态文件常写 `2025`。
- **导入**：分组 — 外部（`@ohos.*`）优先，然后本地项目模块。单导出模块用默认导入（`import Core from './core'`），多导出用命名导入。
- **`TAG` 常量** = `'[Hypium]'` — 所有 `console.info`/日志输出前缀 `` `${TAG}...` ``。
- **导出**：在文件末尾聚合为单个 `export { a, b, c };` 块（见 `index.js:248`）。
- **错误处理**：断言失败时抛出；Mock 匹配器对错误输入抛出 `Error('not a regex')` 等。正常代码中不用 try/catch 做流程控制。
- **命名**：PascalCase 类（`Hypium`、`Core`、`MockKit`、`ArgumentMatchers`）；camelCase 函数/变量（`hypiumTest`、`addService`）；UPPER_CASE 常量（`TAG`、`KEYSET`）；UPPER_CASE 静态类字段（`TestType.FUNCTION`）。
- **服务**：通过 `core.addService('name', service)` 以 `service.id` 为键注册；通过 `core.getDefaultService('name')` 获取。

## 特性

### 基本测试流程
- `describe()` - 定义测试套件
- `it()` - 定义测试用例
- 钩子：`beforeAll`、`afterAll`、`beforeEach`、`afterEach`
- 扩展钩子：`beforeEachIt`、`afterEachIt`、`beforeItSpecified`、`afterItSpecified`

### 关键 API（用于编写/修改测试辅助工具）
- **测试形状**：`export default function suiteFn() { describe('Suite', () => { it('case', 0, () => { expect(x).assertEqual(y) }) }) }`
- **钩子**：`beforeAll`、`afterAll`、`beforeEach`、`afterEach`，以及 `beforeEachIt`/`afterEachIt`（在嵌套套件周围运行）和 `beforeItSpecified(name, fn)`/`afterItSpecified(name, fn)`。
- **跳过**：`xdescribe`/`xit` 用于静态跳过；`throw new SkipError(msg)` 用于运行时条件跳过（标记 spec 为已忽略）。
- **Mock**：`new MockKit()` → `mocker.mockFunc(obj, obj.method)` → `when(stub)(args).afterReturn(val)` → `mocker.verify('method', [args]).times(n)` → `mocker.clear(obj)`。还有 `mockPrivateFunc`、`mockProperty`、`mockObject`、`ignoreMock`。
- **验证模式**（`VerificationMode.js`）：`times(n)`/`never()`/`once()`/`atLeast(n)`/`atMost(n)`。
- **自定义断言**：`Hypium.registerAssert(fn)` / `Hypium.unregisterAssert(fn)`。
- **修饰符**：`.not()`（取反）、`.message(msg)`（自定义失败消息）— 可在任何断言上链式调用。

### 断言库（25+ 方法）

**基础：** `assertTrue`、`assertFalse`、`assertEqual`、`assertFail`
**比较：** `assertClose`、`assertLarger`、`assertLess`、`assertLargerOrEqual`、`assertLessOrEqual`
**类型/空值：** `assertInstanceOf`、`assertNull`、`assertUndefined`、`assertNaN`
**集合：** `assertContain`
**Promise：** `assertPromiseIsPending`、`assertPromiseIsResolved`、`assertPromiseIsRejected`、`assertPromiseIsResolvedWith`、`assertPromiseIsRejectedWith`、`assertPromiseIsRejectedWithError`
**特殊：** `assertNegUnlimited`、`assertPosUnlimited`、`assertDeepEquals`（对象/数组值相等）、`assertThrowError`
**对象属性匹配器：** `assertMatchObj` — 带每个属性嵌入匹配器的部分对象匹配（见下文）
**修饰符：** `.not()`、`.message()`

### Mock 能力
- 函数桩：`when(mock.func()).afterReturn(value)`
- 抛出异常：`when(mock.func()).afterThrow(error)`
- 调用验证：`verify(mock.func()).times(count)`
- 验证模式：`times(n)`、`never()`、`once()`、`atLeast(n)`、`atMost(n)`（`VerificationMode.js`）
- 私有方法/属性 Mock：`mockPrivateFunc`、`mockProperty`
- 参数匹配器

**带匹配器验证**：`verify` 通过 `MockKit.checkIsRightValue` → `ArgumentMatchers.matcheReturnKey` 匹配记录的调用。匹配器可以作为参数传递给 `verify()` 替代精确值，实现灵活的调用验证（例如，`verify(mockFunc(ArgumentMatchers.anyString())).times(1)`）。

### 高级特性
- **数据驱动测试** - 使用不同数据集的参数化测试
- **压力测试** - 重复测试执行以进行性能验证
- **随机执行** - 测试顺序随机化以检测不稳定用例
- **并行执行** - 基于 Worker 的并行测试执行
- **Async/Await** - 原生 Promise 处理
- **测试过滤** - 按测试类型、大小、级别过滤
- **跳过测试** - `xdescribe`、`xit` 用于临时排除
- **自定义位置跳过** - 在测试体任意位置 `throw new SkipError(message)` 跳过当前 spec（在 `SpecService` 执行中捕获，标记为跳过并附原因）。`SkipError` 从框架导出。
- **超时配置** - 每测试或全局超时设置

### 测试配置

**测试类型（位掩码标志）：**
`FUNCTION`、`PERFORMANCE`、`POWER`、`RELIABILITY`、`SECURITY`、`GLOBAL`、`COMPATIBILITY`、`USER`、`STANDARD`、`SAFETY`、`RESILIENCE`

**测试大小：**
`SMALLTEST`、`MEDIUMTEST`、`LARGETEST`

**测试级别：**
`LEVEL0` 到 `LEVEL4`

### 静态树类型说明（ArkTS-Static）
- 禁止对象字面量映射（`{ 'k': v }`）— 用 `new Map<string, int>()` + `.set()` 然后 `export`（见 `Constant.ets:64`）。
- `JSON.stringifyJsonElement`、`jsonx.JsonElement`、`$_iterator()` 是 ArkTS-Static JSON API（不能用普通 `JSON.parse` 解析对象）。
- `readonly` 静态字段：`private static readonly MAX_PARSE_DEPTH = 20;`。

### 测试标签与过滤

**测试用例上的自定义标签**：`it(desc, filter, func, timeout, tag)` 接受可选的 `tag` 字符串参数（`SpecService.it`，`service.js:912`）。标签使用 `|` 作为单个用例上多个标签的分隔符（例如，`"smoke|regression"`）。

**运行时基于标签的过滤**：通过 `aa test` 命令传递 `-s tag <expr>`。表达式解析器（`configService.setTestTag`，`configService.js:121`）支持：
- 逗号 `,` = 或：`"smoke,regression"` 运行标记为 smoke 或 regression 的用例
- 加号 `+` = 与：`"smoke+p0"` 运行同时标记为 smoke 和 p0 的用例
- 组合：`"smoke+p0,regression"` = (smoke 与 p0) 或 regression

当标签过滤激活时，没有任何标签的用例会被跳过（`filterWithTestTag`，`configService.js:284`）。

**其他过滤维度**：`class`/`notClass`（按完全限定的套件/spec 名称）、`suite`、`itName`、`filter`（嵌套名称）、`testType`、`level`、`size`，以及 `runSkipped`（`all`/`skipped`）选择性运行跳过的测试。

### 参数匹配器（增强）

**类型匹配器：** `any()`、`anyString()`、`anyBoolean()`、`anyNumber()`、`anyObj()`、`anyFunction()`
**否定类型匹配器：** `notString()`、`notBoolean()`、`notNumber()`、`notObj()`、`notFunction()`

**模糊匹配器（部分/正则）：**

| 匹配器 | 描述 |
|--------|------|
| `matchRegexs(regex)` | 字符串必须匹配正则 |
| `notMatchRegexs(regex)` | 字符串必须不匹配正则 |
| `containSubstring(sub)` | 字符串必须包含子串 |
| `notContainSubstring(sub)` | 字符串必须不包含子串 |
| `containValue(val)` | 数组必须包含该值 |
| `notContainValue(val)` | 数组必须不包含该值 |
| `containSubArray(arr)` | 数组必须包含子数组的所有元素 |
| `notContainSubArray(arr)` | 数组必须不包含子数组的所有元素 |

所有匹配器在 `when()` 桩和 `verify()` 调用验证中都可用（`ArgumentMatchers.matcheReturnKey`，`ArgumentMatchers.js:130`）。

### 对象属性匹配器断言

**`expect(actual).assertMatchObj(expected)`**（`assertMatchObj.js`）：部分对象匹配 — 只检查 `expected` 中列出的属性，忽略 `actual` 中的额外属性。`expected` 中的每个属性值可以是：
- 具体值（精确相等检查）
- `ArgumentMatchers` 匹配器（例如，`ArgumentMatchers.anyString()` — 灵活的每属性匹配）
- 嵌套对象（递归匹配，最大深度 6）

示例：
```arkts
const user = { name: 'Alice', age: 30, email: 'a@b.com', role: 'admin' };
// 只验证 name 和 age；忽略 email/role
expect(user).assertMatchObj({ name: 'Alice', age: 30 });
// 对一个属性使用匹配器
expect(user).assertMatchObj({ name: ArgumentMatchers.anyString(), age: 30 });
```

### 失败捕获（失败时截图 + 布局转储）

当通过命令行参数 `-s snapshotWhenFail true` 启用时（`configService.enableFailureCapture`，`configService.js:181`），框架在测试用例失败时自动捕获诊断产物：

- **截图**：`Driver.screenCap()` 保存 PNG 到 `/data/storage/el2/base/<suite>_<spec>_<timestamp>.png`
- **布局转储**：`Driver.dumpLayout()` 保存 JSON 到 `/data/storage/el2/base/<suite>_<spec>_<timestamp>.json`

实现：`FailureCaptureService.captureOnFailure()`（`src/module/failure/FailureCaptureService.js`），在 `SpecService` catch 块中触发（`service.js:751`）。文件名经过清理（特殊字符替换为 `_`）。捕获错误会被记录但不影响测试结果。

### Mock 工作流示例
```arkts
const mocker = new MockKit();
const stub = mocker.mockFunc(obj, obj.method);
when(stub)('test').afterReturn('1');              // 桩返回值
when(stub)(ArgumentMatchers.anyString()).afterReturn('2'); // 基于匹配器的桩
expect(obj.method('test')).assertEqual('1');      // 断言
mocker.verify('method', ['test']).once();         // 调用次数验证
mocker.verify('method', [ArgumentMatchers.anyString()]).times(2);
mocker.clear(obj);                                // 恢复原始方法
```

## 运行测试

### 应用测试（ArkTS）

**重要：** 以下所有构建命令必须从 **OpenHarmony 源码根目录** 执行，不是从 jsunit 目录执行。构建输出位于 OpenHarmony 根目录的 `out/<product>/`。

**构建测试应用：**
```bash
# 先导航到 OpenHarmony 根目录
cd /path/to/openharmony

# ArkTS-Dynamic
./test/xts/acts/build.sh product-name <product> system_size=standard suite=<testsuite_path>:<target>

# ArkTS-Static
./test/xts/acts/build.sh product-name <product> system_size=standard xts_suitetype=hap_static suite=<testsuite_path>:<target>
```

**安装 HAP：**
```bash
# 输出在相对于 OpenHarmony 根目录的 out/<product>/suites/haps/
hdc install out/rk3568/suites/haps/<target>.hap
```

**运行测试：**
```bash
# ArkTS-Dynamic
hdc shell aa test -p <bundleName> -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <className>

# ArkTS-Static
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <className>
```

### 运行单个套件/类/用例

```bash
# 运行 bundle 中的所有测试（ArkTS-Dynamic）
hdc shell aa test -b <bundleName> -s unittest OpenHarmonyTestRunner

# 运行单个套件/类 — 关键的"单测"命令
hdc shell aa test -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <SuiteName>

# 运行套件中的单个测试用例
hdc shell aa test -b <bundleName> -s unittest OpenHarmonyTestRunner -s class <SuiteName>#<caseName>

# ArkTS-Static 添加 -m entry
hdc shell aa test -b <bundleName> -m entry -s unittest OpenHarmonyTestRunner -s class <SuiteName>
```

**运行时过滤器**（额外的 `-s` 标志）：`class`/`notClass`（完全限定的 `Suite#case`）、`suite`、`itName`、`testType`、`level`、`size`、`runSkipped`（`all`|`skipped`）、`timeout`、`stress`、`random`、`dryRun`、`snapshotWhenFail`。

### TestRunner

`aa test` 使用 `-s unittest <name>` 指定测试入口类。名称必须与 testrunner 文件名匹配，类名也应匹配。

**名称解析**：三处必须一致 — 文件名 = 导出的类名 = 命令参数。TestRunner 类必须实现 `TestRunner` 接口（`onPrepare()` 和 `onRun()`），在 `onRun()` 中调用 `Hypium.hypiumTest()` 启动测试。

#### 动态与静态项目差异

| 维度 | 动态项目 | 静态项目 |
|------|----------|----------|
| testrunner 路径 | `entry/src/ohosTest/ets/testrunner/` | `entry/src/main/src/testrunner/` |
| 文件类型 | `.ts` | `.ets` |
| 导出语法 | `export default class` | `export class`（命名导出） |
| 模块结构 | 双模块拆分（`main` + `ohosTest`） | 单嵌入模块（`main`） |
| module.json5 | `ohosTest/module.json5`（`name:"entry_test"`，`type:"feature"`） | `main/module.json5`（`name:"entry"`，`type:"entry"`） |
| 命令参数 | `-p <bundle>`（可选，自动检测 ohosTest 模块） | `-m entry`（必需，匹配模块名） |

**为什么静态项目需要 `-m entry`**：静态项目的 `module.json5` 有 `"name":"entry"`，所以命令必须显式指定；动态项目的 `ohosTest` 模块是自动检测的，所以 `-m` 可以省略。

#### 如何自定义 TestRunner

1. 创建 Runner 类（动态：`export default class` / 静态：`export class`），实现 `onPrepare()` + `onRun()`。
2. 在 `onRun()` 中，获取 delegator 并调用 `Hypium.hypiumTest(delegator, args, testsuite)`。
3. 运行时，使用匹配的类名：`-s unittest MyTestRunner`。

参考模板：`src/testrunner/OpenHarmonyTestRunner.ts`（动态）、`src_static/testrunner/OpenHarmonyTestRunner.ets`（静态）。

## 验证（如何检查你的改动）

- **无本地 lint/构建。** jsunit 是纯 npm 包（`package.json` 无 `scripts`/`dependencies`），无 `tsconfig.json`/`.eslintrc`。改动验证方式是消费方测试应用能构建并运行。
- 确认 `src/` 和 `src_static/` 编辑保持特性对等 — 相同行为必须存在于两棵树中。

### 提交前自检清单（无设备时可执行）

虽然无独立构建/lint，提交前仍须完成以下检查：
1. **`index.d.ts` 签名一致性**：新增/修改的公共 API 是否在 `index.d.ts` 中声明了匹配签名。检查方法：对比 `src/` 中 `export` 的函数/类与 `index.d.ts` 中的声明。
2. **双树导出表面对齐**：`index.js` 和 `index.ets` 的导出列表是否一致——新增模块是否两侧都已导出。
3. **静态树语法合规**：`src_static/` 下新增/修改的 `.ets` 文件是否满足：第 1 行 `'use static';`、使用 `int`/`byte`/`short`/`double` 而非 `number`、使用 `AnyType` 而非 `any`、无对象字面量映射（用 `Map`）。
4. **许可证头**：新增文件是否包含 Apache 2.0 许可证头。
5. **TAG 常量**：新增文件中的日志是否使用 `${TAG}` 前缀（`TAG = '[Hypium]'`）。

### Done 定义

模板见根 `../AGENTS.md` § Done 定义。jsunit 专属项：
- 构建证据：无独立构建；须附消费方测试应用构建通过的输出摘录
- 运行证据：`aa test` 运行输出摘录
- 约束确认：双树对等 / 无独立构建 / 公共 API 兼容性

### 无法 device-side 验证时的兜底

见根 `../AGENTS.md` § 无法 device-side 验证时的兜底。

### 消费方构建失败排查清单

当消费方测试应用构建失败时，按以下顺序排查：
1. **`index.d.ts` 是否同步**：新增/修改的 API 是否在 `index.d.ts` 中声明了签名
2. **双树对等**：`src/` 和 `src_static/` 是否成对改动，静态树是否有语法错误（`'use static'`、类型注解）
3. **导出表面**：`index.js` / `index.ets` 是否导出了新增的模块
4. **版本兼容**：是否修改了已有 API 签名（参数类型/个数变更），导致现有用例编译失败

## 重要上下文

1. **双环境支持**：所有特性必须在 ArkTS-Dynamic（NAPI）和 ArkTS-Static（ANI）环境中工作
2. **事件驱动架构**：通过发布/订阅模式实现组件间松耦合
3. **单例模式**：Core 使用单例模式进行集中式状态管理
4. **全局 API 注入**：测试 API 在测试执行期间注入到全局作用域
5. **服务注册表**：所有主要功能组织为可独立管理的服务
6. **基于 Promise 的异步**：原生 async/await 支持，带 Promise 断言
7. **模块版本**：API 支持从 API 8 开始，后续版本增量添加特性
8. **SkipError**：在测试体任意位置抛出 `SkipError` 跳过当前 spec 并附原因 — 支持基于运行时检查的条件跳过
9. **失败捕获**：需要 `@ohos.UiTest` Driver；通过 `-s snapshotWhenFail true` 启用；保存到 `/data/storage/el2/base/`
10. **测试框架**：这是测试框架本身，不是被测应用

## 关键文件

| 文件 | 用途 |
|------|------|
| `index.js` | 主入口点（ArkTS-Dynamic） |
| `index.ets` | 主入口点（ArkTS-Static） |
| `index.d.ts`、`index.ts` | TypeScript 定义 |
| `src/core.js` | 核心引擎和单例 |
| `src/service.js` | 服务层实现（所有四个服务） |
| `src/interface.js` | 全局 API 注入（`describe`/`it`/`expect`/hooks） |
| `src/Constant.js` | `TestType`/`Size`/`Level` 位掩码枚举、`TAG`、`KEYSET` |
| `src/module/assert/ExpectExtend.js` | 25+ 断言实现 |
| `src/module/assert/assertMatchObj.js` | 部分对象属性匹配器断言 |
| `src/module/mock/MockKit.js` | 函数/对象桩，带匹配器验证 |
| `src/module/mock/ArgumentMatchers.js` | 14 种匹配器（类型、模糊正则/子串/数组） |
| `src/module/mock/VerificationMode.js` | 调用次数验证模式 |
| `src/module/config/configService.js` | 配置、基于标签的过滤、失败捕获标志 |
| `src/module/config/DataDriver.js` | 数据驱动测试 |
| `src/module/failure/FailureCaptureService.js` | 失败时自动截图 + 布局转储 |
| `src/module/report/OhReport.js` | OpenHarmony 特定报告 |
| `src/module/kit/SysTestKit.js` | 日志关键字检测、用例元数据访问 |
| `src_static/` | ArkTS-Static 特定实现（`src/` 的镜像） |
| `package.json` | NPM 包配置（无 scripts，无 dependencies） |
| `README.md` | 详细特性和 API 文档 |

## 项目约束

根 `../AGENTS.md` § 项目约束 的跨组件通用约束同样适用。以下为 jsunit 专属约束：

jsunit 是纯 npm 包，无独立构建/lint。

**不变量**：
- **双树对等（尽量达成）**：`src/`（`.js`，ArkTS-Dynamic）和 `src_static/`（`.ets`，ArkTS-Static）改动应尽量保持特性对等。改一侧前应确认另一侧的对应实现。由于 ArkTS-Static 语法限制，某些特性可能无法完全对等，此时须明确说明 gap 点及原因。
- **无独立构建/验证**：`package.json` 无 `scripts`、无 `dependencies`，无 `tsconfig.json`/`.eslintrc`。改动验证方式是消费方测试应用能构建并运行（见 § 验证）。

**禁止**：
- 修改 `index.d.ts` 公共 API 签名而不确认向后兼容（规则见根 `../AGENTS.md` § 项目约束）。
- 禁止改变现有 API 的行为（见根 `../AGENTS.md` § 项目约束）。

**改前须确认（ask-before）**：
- 改 `src/service.js` 的 `SpecService` 执行流程（it/case 超时、skip 逻辑、hook 顺序）→ 影响所有消费方测试的执行语义，先确认行为变更的向后影响。
- 改 `src/module/config/configService.js` 的过滤逻辑（`setTestTag` `:121`、`filterWithTestTag` `:284`）→ 影响运行时用例选择，先确认过滤表达式语法不变。
- 改 `src/Constant.js` 的 `TestType`/`Size`/`Level` 位掩码枚举值 → 影响外部 `filter` 参数，先确认位值不变。
- 改 `index.js` 的 `Hypium` 类导出表面（`hypiumTest` `:48`/`hypiumInitWorkers` `:73`/`registerAssert` `:228`）→ 影响所有 TestRunner，先确认签名兼容。

## 备注
- 改动行为时保持 `src/` 和 `src_static/` 同步。
- API 支持从 API 8 开始；里程碑特性在 README 中标记版本（例如 `1.0.25`、`1.0.26`、`1.0.28`）。
- 完整的 arkxtest 架构和 C++ 构建详情见父级 `../AGENTS.md`。

## 相关文档

- **根文档**：`../AGENTS.md` - 整体 arkxtest 架构
- **Uitest**：`../uitest/AGENTS.md` - UI 自动化测试框架
- **PerfTest**：`../perftest/AGENTS.md` - 性能测试框架
- **TestServer**：`../testserver/AGENTS.md` - 系统能力服务
- **TestHelper**：`../testhelper/AGENTS.md` - CLI 工具
- **依赖**：`../deps.md` - 完整外部依赖列表
- **中文 README**：`../README_zh.md` - 详细特性文档
