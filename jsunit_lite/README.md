# jsunit_lite - 小型系统 JS 单元测试框架

## 简介

jsunit_lite 是面向 OpenHarmony 小型系统（small system）的 JS 单元测试框架。提供测试用例编写、执行和结果输出能力，用于小型设备 JS 接口自动化测试。

## 适用场景

- **系统类型**：小型系统（smartVision），如 HiSpark Taurus（ipcamera_hispark_taurus）
- **JS 引擎**：JerryScript（JERRY_ES2015=1）
- **内核类型**：Linux 内核 / LiteOS 内核
- **通信方式**：串口（serial，115200）+ 网口（telnet/NFS），无 hdc

## 目录结构

```
jsunit_lite/
├── index.js                          # 入口：Hypium 对象 + 导出 API
├── src/
│   ├── core.js                       # Core 单例引擎：服务注册 + 事件总线
│   ├── service.js                    # 4 个服务 + Suite/Spec + 断言实现
│   ├── interface.js                  # 全局 API 包装层
│   ├── event.js                      # 事件类：SpecEvent/SuiteEvent/TaskEvent
│   ├── Constant.js                   # 常量：TestType/Size/Level
│   └── module/
│       ├── config/configService.js   # 配置服务
│       ├── kit/SysTestKit.js         # 测试上下文查询
│       └── report/liteReport.js      # 结果输出（console.info 格式）
└── README.md                         # 本文档
```

## 引擎限制

小型系统 JerryScript 引擎能力受限，以下语法在编译后（babel 转换）不可用：

| 语法 | 原因 | 替代方案 |
|------|------|----------|
| `class extends` | babel 生成 `Reflect.construct`，引擎无 Reflect | 使用 `Object.create(Error.prototype)` |
| `async`/`await` | babel 生成 `regeneratorRuntime`，引擎不支持 | 使用同步执行或 `done` 回调 |
| `Reflect.has()` | 引擎无 Reflect 对象 | 使用 `typeof obj.prop === 'function'` |
| `globalThis` | ES2020，引擎不支持 | API 直接写入 Core 实例属性 |
| `Promise.then()` 微任务 | Promise 存在但微任务不触发 | 全部改为同步执行 |

以下语法**可用**（babel 编译后兼容）：

| 语法 | 说明 |
|------|------|
| `import`/`export` | webpack 统一转为 CommonJS |
| `let`/`const` | babel 转为 `var` |
| `class`（无 extends） | babel 转为 `classCallCheck`/`createClass`，ES5 兼容 |
| `for`/`while`/`switch` | 原生 ES5 |
| `Promise`（同步调用） | Promise 对象本身可用，但 `.then()` 回调不可靠 |

### app.js 体积限制

ACE Lite 引擎对单个 JS 文件有大小限制：

- **限制值**：48KB（`FILE_CONTENT_LENGTH_MAX = 1024 * 48`）
- **源码位置**：`foundation/arkui/ace_engine_lite/frameworks/src/core/base/js_fwk_common.h`
- **超限表现**：ACE 引擎从 JS parser 模式切换到字节码模式（查找 `app.bc`），debug 编译不生成 `.bc` 文件导致报错 `view model is undefined`
- **类型溢出**：源码中类型为 `uint16_t`，值 `1024 * 48 = 49152` 未溢出；若改为 `1024 * 256` 会溢出为 0，需同时改 `uint32_t`

如需解除限制，修改 `js_fwk_common.h` 中 `uint16_t` → `uint32_t`，值改 `1024 * 256`，需重新编译并烧录镜像。

## API 列表

### 测试入口

| API | 参数 | 说明 |
|-----|------|------|
| `Hypium.hypiumTest(delegator, args, testsuite)` | delegator: any, args: any, testsuite: Function | 初始化框架并执行测试 |

### 用例定义

| API | 参数 | 说明 |
|-----|------|------|
| `describe(name, fn)` | name: string, fn: Function | 定义测试套 |
| `it(name, filter, fn)` | name: string, filter: number, fn: Function | 定义测试用例（filter 参数当前不生效，保留用于 codecheck 合规） |
| `xdescribe(name, fn)` | name: string, fn: Function | 定义跳过的测试套 |
| `xit(name, filter, fn)` | name: string, filter: number, fn: Function | 定义跳过的测试用例 |
| `xdescribe.reason(reason)(name, fn)` | reason: string | 带原因跳过测试套 |
| `xit.reason(reason)(name, filter, fn)` | reason: string | 带原因跳过测试用例 |

### 生命周期钩子

| API | 参数 | 说明 |
|-----|------|------|
| `beforeAll(fn)` | fn: Function | 所有用例前执行一次 |
| `afterAll(fn)` | fn: Function | 所有用例后执行一次 |
| `beforeEach(fn)` | fn: Function | 每个用例前执行 |
| `afterEach(fn)` | fn: Function | 每个用例后执行 |
| `beforeEachIt(fn)` | fn: Function | 每个用例前执行（含嵌套套件传递） |
| `afterEachIt(fn)` | fn: Function | 每个用例后执行（含嵌套套件传递） |
| `beforeItSpecified(itDescs, fn)` | itDescs: string\|string[], fn: Function | 仅指定用例前执行 |
| `afterItSpecified(itDescs, fn)` | itDescs: string\|string[], fn: Function | 仅指定用例后执行 |

### 断言 API

通过 `expect(actualValue)` 返回断言对象，调用断言方法：

| 断言方法 | 参数 | 说明 |
|----------|------|------|
| `assertEqual(expected)` | expected: any | 严格相等 |
| `assertTrue()` | — | 判断为 true |
| `assertFalse()` | — | 判断为 false |
| `assertNull()` | — | 判断为 null |
| `assertUndefined()` | — | 判断为 undefined |
| `assertContain(sub)` | sub: string | 包含子串（非字符串类型先转 String） |
| `assertLarger(val)` | val: number | 大于 |
| `assertLess(val)` | val: number | 小于 |
| `assertLargerOrEqual(val)` | val: number | 大于等于 |
| `assertLessOrEqual(val)` | val: number | 小于等于 |
| `assertClose(val, precision)` | val: number, precision: number | 相对误差范围内相近 |
| `assertInstanceOf(type)` | type: string | 类型匹配（如 'Array'） |
| `assertFail()` | — | 直接失败 |
| `assertThrowError(expected)` | expected: string\|Function | 期望抛出异常 |
| `assertNaN()` | — | 判断为 NaN |
| `assertNegUnlimited()` | — | 判断为 -Infinity |
| `assertPosUnlimited()` | — | 判断为 Infinity |

### 断言修饰符

| 修饰符 | 说明 |
|--------|------|
| `.not()` | 取反断言结果 |
| `.message(msg)` | 自定义失败消息 |

```javascript
expect(1).not().assertEqual(2);  // 通过：1 不等于 2
expect(1).message('自定义失败消息').assertEqual(2);  // 失败时输出自定义消息
```

### SkipError

```javascript
import { SkipError } from 'jsunit_lite/index';

it('conditional_test', Level.LEVEL0, function() {
    if (!someCondition) {
        throw new SkipError('条件不满足，跳过');
    }
    // 正常测试逻辑
});
```

通过 SkipError 跳出的用例在结果中标记为 `[ignore]`。

### 常量

| 常量 | 说明 |
|------|------|
| `TestType.FUNCTION` 等 | 测试类型位掩码（FUNCTION/PERFORMANCE/POWER/...） |
| `Size.SMALLTEST` 等 | 测试规模位掩码（SMALLTEST/MEDIUMTEST/LARGETEST） |
| `Level.LEVEL0` ~ `LEVEL4` | 测试级别位掩码 |
| `DEFAULT` | 默认过滤值 0 |

常量用于 `it` 的 `filter` 参数（当前不生效，保留用于 codecheck 合规）：

```javascript
it('test_case', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1, function() {
    expect(1).assertEqual(1);
});
```

## 结果输出格式

测试结果通过 `console.info` 输出，与 xdevice 的 `JSUnitParserLite` parser 对齐：

```
[Console Info] [start] start run suites
[Console Info] [suite start] test_suite_name
[Console Info] [start] test_case_name
[Console Info] [pass] test_case_name
[Console Info] [fail] test_case_name
[Console Info] [ignore] test_case_name
[Console Info] [suite end]
[Console Info] [end] run suites end
```

| 标记 | 含义 |
|------|------|
| `[start] start run suites` | 测试任务开始 |
| `[suite start] xxx` | 测试套开始 |
| `[start] xxx` | 单条用例开始 |
| `[pass] xxx` | 用例通过 |
| `[fail] xxx` | 用例失败（后跟错误堆栈） |
| `[ignore] xxx` | 用例跳过 |
| `[suite end]` | 测试套结束 |
| `[end] run suites end` | 测试任务结束 |

## 代码示例

### 基本用例

```javascript
import { describe, it, expect, Level } from 'jsunit_lite/index';

export default function myTest() {
    describe('MyTestSuite', function() {
        it('assert_equal', Level.LEVEL0, function() {
            expect(1).assertEqual(1);
        });

        it('assert_contain', Level.LEVEL0, function() {
            expect('hello world').assertContain('world');
        });
    });
}
```

### 生命周期钩子

```javascript
import { describe, it, expect, beforeAll, afterAll, beforeEach, afterEach, Level } from 'jsunit_lite/index';

export default function lifecycleTest() {
    describe('LifecycleTest', function() {
        var counter;

        beforeAll(function() {
            counter = 0;
        });

        beforeEach(function() {
            counter++;
        });

        afterEach(function() {
            // 清理
        });

        afterAll(function() {
            // 最终清理
        });

        it('case_1', Level.LEVEL0, function() {
            expect(counter).assertEqual(1);
        });

        it('case_2', Level.LEVEL0, function() {
            expect(counter).assertEqual(2);
        });
    });
}
```

小型系统不支持 async/await 和 done 回调，所有测试用例必须使用同步函数。

### 测试入口（app.js）

```javascript
import Core from 'jsunit_lite/src/core';
import liteReport from 'jsunit_lite/src/module/report/liteReport';
import { describe, it, expect } from 'jsunit_lite/index';
import testsuite from '../test/List.test';

export default {
    data: {},
    onCreate: function() {
        console.info('[MyTest] onCreate start');
        var core = Core.getInstance();
        var reportInstance = new liteReport({ id: 'default' });
        core.addService('report', reportInstance);
        core.init();
        testsuite();
        core.execute();
    },
    onDestroy: function() {
        console.info('[MyTest] onDestroy');
    }
};
```

## 相关文档

- [XTS 用例开发指导（小型系统 JS）](../../README_zh.md) - 端到端开发编译执行流程
- [arkxtest 总览](../README_zh.md) - 自动化测试框架总览
