# 执行计划

> 将 Approved Spec 拆成可独立执行、可验证、可审查的 Task。

## Plan 元数据

| 字段 | 内容 |
|------|------|
| Plan ID | PLAN-ISMOCKED-2026 |
| 关联 Feature | FEAT-ISMOCKED-2026 |
| 关联文档 | proposal.md / design.md / spec.md |
| 复杂度 | 标准 |
| 状态 | Draft |
| Owner | [待确认] |

## 输入状态

| 输入 | 路径 | 要求状态 |
|------|------|----------|
| Requirement | `proposal.md` | Approved（基线通过） |
| Design | `design.md` | Approved（设计通过） |
| Spec | `spec.md` | Approved |

## 受影响文件全量清单

| 仓 | 层（来自 design.md） | 文件路径 | 修改类型 | 说明 |
|----|---------------------|----------|----------|------|
| testfwk_arkxtest | Mock 模块（动态树） | `jsunit/src/module/mock/MockKit.js` | 修改 | 新增 isMocked 实例方法，遍历 recordMockedMethod + propertyValueMap |
| testfwk_arkxtest | API 声明 | `jsunit/index.d.ts` | 修改 | MockKit class 新增 isMocked 方法签名声明 |
| testfwk_arkxtest | Mock 模块（静态树） | `jsunit/src_static/module/mock/MockKit.ets` | 修改 | 新增 isMocked 实例方法，遍历 hashMap |
| testfwk_arkxtest | 测试 | 消费方测试应用（路径由实现时确定） | 新增 | XTS 验收测试用例，覆盖 7 条 AC |

**检查项：**
- [x] design.md 调用链每一层都有对应文件列出（Mock 模块 / API 声明 / 测试）
- [x] 每个文件修改类型和职责说明明确
- [x] 无映射行的层意味着设计到实现的翻译缺失 — 无缺失层

**生成文件声明：** 无生成文件（无 bridge/IDL/CAPI/cpptoc）。所有文件为手工编辑的 `.js`/`.ets`/`.d.ts` 源码。

## AC 到 Task 追溯

| AC | 来源 | Task | 验证方式 | 覆盖？ |
|----|------|------|----------|--------|
| AC-1 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-2 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-3 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-4 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-5 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-6 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-7 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-8 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |
| AC-9 | spec.md | TASK-1, TASK-2, TASK-3 | XTS 单元测试 | 是 |

## 首批实现边界

**首批必须实现：** TASK-1（动态树 isMocked + index.d.ts）、TASK-2（静态树 isMocked）——双树对等约束要求两侧同步
**可后置：** TASK-3（XTS 验收测试用例）——依赖 TASK-1 + TASK-2 完成
**不建议延后：** 无

## 阶段计划

| 阶段 | 目标 | 关键 Task | 结束门槛 | 最小验证 |
|------|------|-----------|----------|----------|
| Phase-1 | 双树实现 | TASK-1, TASK-2 | 两棵树 isMocked 方法均已新增，index.d.ts 声明已更新 | 源码审查：方法存在、签名一致、双树对等 |
| Phase-2 | 验收测试 | TASK-3 | 7 条 AC 全部有测试用例覆盖 | 消费方测试应用构建通过 + aa test 运行 7 条 AC 全 PASS |

## Task 粒度原则

- TASK-1 和 TASK-2 分别对应动态树和静态树实现，文件范围分离（`.js` vs `.ets`），验证闭环独立 → 拆分为两张 Task Card
- TASK-3 为验收测试，依赖前两个 Task 完成 → 独立 Task Card
- 每个 Task 对应一个可独立验收的最小能力闭环

## 禁止项

- [x] 没有 TBD / TODO / 占位符（Owner 字段为"[待确认]"属元数据非 Task 指令）
- [x] 没有"根据需要实现""酌情处理"等模糊指令
- [x] 没有跨 Task 隐式依赖（依赖显式声明在前置依赖列）
- [x] 没有要求 Agent 自行寻找未列出的上下文文件
- [x] 没有无验证方式的 AC
- [x] 没有"与 Task-N 类似"等引用（每个 Task 自包含）

## Task 列表

| Task ID | 目标 | 文件范围 | AC 映射 | 前置依赖 | 完成判据 | 验证命令 |
|---------|------|----------|---------|----------|----------|----------|
| TASK-1 | 动态树 isMocked 实现 + index.d.ts 声明 | `jsunit/src/module/mock/MockKit.js`, `jsunit/index.d.ts` | AC-1~AC-9 | 无 | MockKit.js 新增 isMocked 方法，index.d.ts 新增声明 | 源码审查 + 消费方构建 |
| TASK-2 | 静态树 isMocked 实现 | `jsunit/src_static/module/mock/MockKit.ets` | AC-1~AC-9 | TASK-1 | MockKit.ets 新增 isMocked 方法 | 源码审查 + 消费方构建 |
| TASK-3 | XTS 验收测试用例 | 消费方测试应用 | AC-1~AC-9 | TASK-1, TASK-2 | 7 条 AC 全部有测试用例，aa test 全 PASS | `hdc shell aa test ...` |

## Task 详情

### TASK-1: 动态树 isMocked 实现 + index.d.ts 声明

| 字段 | 内容 |
|------|------|
| 任务目标 | 在 MockKit.js 中新增 isMocked 实例方法，在 index.d.ts 中新增方法签名声明 |
| AC 映射 | AC-1, AC-2, AC-3, AC-4, AC-5, AC-6, AC-7, AC-8, AC-9（动态树侧） |
| 前置依赖 | 无 |
| 非目标 | 不修改静态树（MockKit.ets）；不修改现有 mockFunc/mockProperty/clear/ignoreMock 行为；不编写测试用例（TASK-3） |
| 完成判据 | MockKit.js 中 MockKit 类新增 isMocked 方法；index.d.ts 中 MockKit class 新增 isMocked 声明；方法签名与 spec.md 接口规格一致 |
| 停止条件 | 发现需修改 MockKit.js 中现有方法行为时停止并回传 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `jsunit/src/module/mock/MockKit.js` | 在 MockKit 类中（ignoreMock 方法之后、extend 方法之前）新增 isMocked 实例方法 |
| Modify | `jsunit/index.d.ts` | 在 MockKit class 声明中（mockProperty 之后）新增 isMocked 方法签名 |

**Spec Context**

AC-1: WHEN 对已 mock 方法的对象调用 isMocked(instance, "methodName") THEN 返回 true
AC-2: WHEN 清理后调用 isMocked THEN 返回 false
AC-3: WHEN 对系统模块 API 调用 isMocked THEN 返回 false
AC-4: WHEN 对未 mock 过的对象调用 isMocked THEN 返回 false
AC-5: WHEN 多次 mock 后调用 isMocked THEN 返回 true；全部清理后 THEN 返回 false
AC-6: WHEN 对已 mock 属性调用 isMocked THEN 返回 true
AC-7: WHEN 清理属性 mock 后调用 isMocked THEN 返回 false

规则 R-1~R-7 见 spec.md 规则定义表。

**Design Context**

ADR-1: 动态树 isMocked 同时查询 recordMockedMethod（方法 mock）和 propertyValueMap（属性 mock），任一命中返回 true。先查 recordMockedMethod，命中则短路返回 true；否则查 propertyValueMap。
ADR-3: 非法输入不报错，遍历自然返回 false。

动态树数据结构：
- `this.recordMockedMethod`: Map，key=`{obj, methodName}`，value=原始方法
- `this.propertyValueMap`: Map，key=`{obj, methodName}`，value=原始属性值

代码风格（src/ 动态树）：
- 4 空格缩进
- 单引号字符串
- ES 模块 `export`
- `function` 关键字（与 clear/ignoreMock 一致）
- 许可证头已有（Copyright (c) 2022-2024），文件已存在不需新增

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-API-LEVEL | Must: 新增 Public API 纯新增方法，向后兼容 |
| 双树对等 | Must: 此 Task 完成后须执行 TASK-2 同步静态树 |
| 不改变现有 API 行为 | Must Not: 不修改 mockFunc/mockProperty/clear/ignoreMock/verify 等现有方法 |

**Steps**

- [ ] 在 `jsunit/src/module/mock/MockKit.js` 的 MockKit 类中，`ignoreMock` 方法之后新增 `isMocked` 方法
- [ ] isMocked 实现：遍历 `this.recordMockedMethod`，匹配 `key.obj === obj && key.methodName === name`，命中返回 true；再遍历 `this.propertyValueMap`，同样匹配，命中返回 true；否则返回 false
- [ ] 在 `jsunit/index.d.ts` 的 MockKit class 声明中，`mockProperty` 之后新增 `isMocked(obj: Object, name: String): boolean`
- [ ] 确认 `index.js` 的 export 块已包含 MockKit（已导出，无需改动）
- [ ] 填写完成证据

**实现代码参考**（来自 design.md）：

```javascript
isMocked(obj, name) {
  let mockFuncResult = null;
  for (const [key, value] of this.mockFuncResultMap) {
    if (key.obj === obj && key.methodName === name) {
      mockFuncResult = value;
      break;
    }
  }
  if (mockFuncResult && this.stubs instanceof Map && this.stubs.has(mockFuncResult)) {
    return true;
  }
  let isPropertyMocked = false;
  this.propertyValueMap.forEach(function (value, key, map) {
    if (key.obj === obj && key.methodName === name) {
      if (obj[name] !== value) {
        isPropertyMocked = true;
      }
    }
  });
  return isPropertyMocked;
}
```

**Completion Evidence**

| 证据类型 | 命令/路径 | 结果 |
|----------|-----------|------|
| 源码审查 | MockKit.js 中 isMocked 方法存在 | PASS/FAIL |
| 签名一致 | index.d.ts 中 isMocked 声明与 spec.md 接口规格一致 | PASS/FAIL |
| 导出检查 | index.js export 块已含 MockKit（无需改动） | PASS |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | 在 MockKit.js 新增 isMocked 实例方法（遍历 recordMockedMethod + propertyValueMap），在 index.d.ts 新增方法签名声明 |
| 允许修改 | `jsunit/src/module/mock/MockKit.js`, `jsunit/index.d.ts` |
| 允许新建 | 无 |
| 只读参考 | `jsunit/src/module/mock/ExtendInterface.js`（了解 extend 机制）, `jsunit/AGENTS.md` § Mock 能力 / § 代码风格 |
| Spec 摘要 | AC-1~AC-7：isMocked 对已 mock 方法/属性返回 true，清理后返回 false，系统方法/未 mock 对象返回 false，多次 mock 后返回 true 全部清理后 false。规则 R-1~R-7 见 spec.md。 |
| Design 摘要 | ADR-1: 方法 mock 查 mockFuncResultMap（找 mock 函数）+ stubs（查 when() 是否调过）；属性 mock 比较 obj[name] 与 propertyValueMap 原始值。mockFunc 仅替换为包装函数但未改行为（需 when() 才算真正 mock）。clear() 删 mockFuncResultMap 条目但不删 stubs/recordMockedMethod/propertyValueMap。ADR-3: 非法输入不报错返回 false。代码风格：4 空格、单引号、function 关键字。 |
| 执行步骤 | 1. 在 MockKit.js ignoreMock 之后新增 isMocked 方法 2. 在 index.d.ts MockKit class 新增 isMocked 声明 3. 确认 index.js 导出无需改动 |
| 验证命令 | 源码审查 + 消费方构建 / 期望: PASS |
| 完成规则 | 不得修改允许范围外的文件；如需扩大范围，停止并修订 Plan；没有 fresh verification evidence，不得声明完成 |

### TASK-2: 静态树 isMocked 实现

| 字段 | 内容 |
|------|------|
| 任务目标 | 在 MockKit.ets 中新增 isMocked 实例方法，与动态树行为对等 |
| AC 映射 | AC-1, AC-2, AC-3, AC-4, AC-5, AC-6, AC-7（静态树侧） |
| 前置依赖 | TASK-1 完成（参照动态树 isMocked 行为） |
| 非目标 | 不修改动态树（MockKit.js）；不修改现有 mockFunc/mockProperty/clear/ignoreMock 行为；不编写测试用例（TASK-3） |
| 完成判据 | MockKit.ets 中 MockKit 类新增 isMocked 方法；方法签名与动态树对等；静态树语法合规（'use static' 首行、类型注解） |
| 停止条件 | 发现静态树语法限制导致无法对等实现时停止并回传，记录 gap 点 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `jsunit/src_static/module/mock/MockKit.ets` | 在 MockKit 类中（ignoreMock 方法之后）新增 isMocked 实例方法 |

**Spec Context**

AC-1~AC-7 同 TASK-1（静态树侧行为对等）。

已知 Gap（design.md 已知 Gap 表）：
- 静态树 mockFunc 后、when() 前调用 isMocked 返回 false（hashMap 尚无条目），动态树返回 true。此 gap 已在 proposal Q-2 确认接受。
- 静态树 mockProperty 后调用 isMocked 返回 true（mockProperty 内部创建 AfterWhen，hashMap 有条目），与动态树一致。

**Design Context**

ADR-2: 静态树 isMocked 查询 hashMap（Map<MapKey, string>），key=`{methodName, args, classObj}`，匹配 `instance === keyClassObj && name === keyMethodName`。

静态树数据结构：
- `this.hashMap`: Map<MapKey, string>，MapKey = `{methodName: string, args: Object[], classObj: object}`
- hashMap 统一记录方法 mock 和属性 mock

代码风格（src_static/ 静态树）：
- 2 空格缩进
- 首行 `'use static';`（文件已有）
- 完整类型注解：`instance: object`, `name: string`, 返回 `boolean`
- 禁止 `any`，用 `object`/`string`/`boolean`
- 许可证头已有（Copyright (c) 2025-2026），文件已存在不需新增
- hilog 日志可选（与现有方法风格一致）

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-API-LEVEL | Must: 新增 Public API，与动态树签名一致 |
| 双树对等 | Must: 行为尽量与动态树对等；已知 gap 记录在 design.md |
| 静态树语法 | Must: 'use static' 首行、2 空格缩进、完整类型注解 |
| 不改变现有 API 行为 | Must Not: 不修改现有 mockFunc/mockProperty/clear/ignoreMock 方法 |

**Steps**

- [ ] 在 `jsunit/src_static/module/mock/MockKit.ets` 的 MockKit 类中，`ignoreMock` 方法之后新增 `isMocked` 方法
- [ ] isMocked 实现：遍历 `this.hashMap`，匹配 `instance === key.classObj && name === key.methodName`，命中返回 true；遍历结束返回 false
- [ ] 确认方法签名 `isMocked(instance: object, name: string): boolean` 与动态树和 index.d.ts 一致
- [ ] 填写完成证据

**实现代码参考**（来自 design.md）：

```typescript
isMocked(instance: object, name: string): boolean {
  let hashMap = this.hashMap
  for (const hashEntries of hashMap.entries()) {
    if (!hashEntries) {
      continue
    }
    const key = hashEntries[0]
    const keyMethodName = key.methodName
    const keyClassObj = key.classObj
    if (instance === keyClassObj && name === keyMethodName) {
      return true
    }
  }
  return false
}
```

**Completion Evidence**

| 证据类型 | 命令/路径 | 结果 |
|----------|-----------|------|
| 源码审查 | MockKit.ets 中 isMocked 方法存在 | PASS/FAIL |
| 语法合规 | 'use static' 首行、2 空格、类型注解完整 | PASS/FAIL |
| 签名一致 | 与 index.d.ts 和动态树 MockKit.js 签名对等 | PASS/FAIL |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | 在 MockKit.ets 新增 isMocked 实例方法（遍历 hashMap 匹配 classObj + methodName），与动态树行为对等 |
| 允许修改 | `jsunit/src_static/module/mock/MockKit.ets` |
| 允许新建 | 无 |
| 只读参考 | `jsunit/src_static/module/mock/MockKit.ets`（现有 ignoreMock/clear 方法风格）, `jsunit/AGENTS.md` § 静态树类型说明 / § 代码风格 |
| Spec 摘要 | AC-1~AC-8 同 TASK-1（静态树侧）。已知 Gap：mockFunc 后 when() 前 isMocked 返回 false（与动态树行为一致，已接受）。 |
| Design 摘要 | ADR-2: 查 hashMap，匹配 classObj + methodName。hashMap 统一记录方法和属性 mock。代码风格：2 空格、'use static'、类型注解、hilog 可选。 |
| 执行步骤 | 1. 在 MockKit.ets ignoreMock 之后新增 isMocked 方法 2. 遍历 hashMap 匹配 classObj + methodName 3. 确认签名与 index.d.ts 一致 |
| 验证命令 | 源码审查 + 消费方构建 / 期望: PASS |
| 完成规则 | 不得修改允许范围外的文件；如需扩大范围，停止并修订 Plan；没有 fresh verification evidence，不得声明完成 |

### TASK-3: XTS 验收测试用例

| 字段 | 内容 |
|------|------|
| 任务目标 | 编写 XTS 测试用例覆盖 9 条 AC，在消费方测试应用中构建并运行 |
| AC 映射 | AC-1, AC-2, AC-3, AC-4, AC-5, AC-6, AC-7, AC-8, AC-9 |
| 前置依赖 | TASK-1 完成, TASK-2 完成 |
| 非目标 | 不修改框架源码（MockKit.js/MockKit.ets/index.d.ts） |
| 完成判据 | 9 条 AC 全部有对应测试用例；消费方测试应用构建通过；aa test 运行 9 条 AC 全 PASS |
| 停止条件 | 消费方测试应用构建失败时停止并回传构建错误 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Create | 消费方测试应用中的测试文件（路径由实现时确定） | 编写覆盖 9 条 AC 的测试用例 |

**Spec Context**

| AC | 测试场景 | 预期 |
|----|----------|------|
| AC-1 | mockFunc + when() 后 isMocked(instance, "methodName") | 返回 true |
| AC-2 | clear/ignoreMock 后 isMocked(instance, "methodName") | 返回 false |
| AC-3 | 系统模块 API isMocked | 返回 false |
| AC-4 | 未 mock 对象 isMocked | 返回 false |
| AC-5 | 多次 mockFunc+when() 后 isMocked 返回 true；全部清理后 false | true → false |
| AC-6 | mockProperty 后 isMocked(instance, "propertyName") | 返回 true |
| AC-7 | 清理属性 mock 后 isMocked | 返回 false |
| AC-8 | mockFunc 但未调 when() 时 isMocked(instance, "methodName") | 返回 false |
| AC-9 | name 为空字符串或 null 时 isMocked(instance, name) | 返回 false |

**Design Context**

测试用例需覆盖双树（动态树 + 静态树）。动态树测试在 ArkTS-Dynamic 消费方应用中运行，静态树测试在 ArkTS-Static 消费方应用中运行。

已知 Gap（静态树）：mockFunc 后 when() 前 isMocked 返回 false。测试用例 AC-1 在静态树侧应使用 mockFunc + when() 后调用 isMocked 的模式。

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| 双树对等 | Must: 动态树和静态树均需有测试用例 |
| 证据先于声明 | Must: aa test 运行输出作为完成证据，不得用"应当通过"替代 |

**Steps**

- [ ] 在消费方测试应用中创建测试套件，定义用户自定义类（含方法和属性）
- [ ] AC-1: mockFunc + when() 后调用 isMocked(instance, "methodName")，断言返回 true
- [ ] AC-2: 调用 clear(obj) 后 isMocked(instance, "methodName")，断言返回 false；另用 ignoreMock 后断言 false
- [ ] AC-3: 对系统模块对象调用 isMocked，断言返回 false
- [ ] AC-4: 对从未 mock 的对象调用 isMocked，断言返回 false
- [ ] AC-5: 对同一对象 mockFunc+when() 多个方法，逐一 isMocked 断言 true；clear 后逐一断言 false
- [ ] AC-6: mockProperty 后 isMocked(instance, "propertyName")，断言返回 true
- [ ] AC-7: clear/ignoreMock 清理属性后 isMocked(instance, "propertyName")，断言返回 false
- [ ] AC-8: 仅 mockFunc 不调 when()，isMocked(instance, "methodName")，断言返回 false
- [ ] AC-9: name 为空字符串 "" 和 null 分别调用 isMocked(instance, name)，断言返回 false
- [ ] 构建消费方测试应用
- [ ] 运行 aa test，记录输出
- [ ] 填写完成证据

**Completion Evidence**

| 证据类型 | 命令/路径 | 结果 |
|----------|-----------|------|
| 构建 | 消费方测试应用构建通过 | PASS/FAIL |
| 测试 | `hdc shell aa test -b <bundle> -s unittest OpenHarmonyTestRunner -s class <SuiteName>` | 9 条 AC 全 PASS |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | 编写 XTS 测试用例覆盖 9 条 AC，在消费方测试应用中构建并运行 |
| 允许修改 | 消费方测试应用中的测试文件 |
| 允许新建 | 消费方测试应用中的测试文件 |
| 只读参考 | `jsunit/index.d.ts`（API 签名）, `jsunit/AGENTS.md` § Mock 工作流示例 / § 运行测试, spec.md AC-1~AC-9 |
| Spec 摘要 | AC-1: mockFunc+when() 后 true。AC-2: 清理后 false。AC-3: 系统方法 false。AC-4: 未 mock false。AC-5: 多次 mockFunc+when() true 全清理 false。AC-6: mock 属性 true。AC-7: 清理属性 false。AC-8: mockFunc 但未调 when() false。AC-9: name 为空/null false。 |
| Design 摘要 | 双树测试。动态树 mockFunc 需 when() 才在 stubs 有条目；静态树 mockFunc 需 when() 才在 hashMap 有条目。两树行为一致：mockFunc 未调 when() 时 isMocked 返回 false。name 为空/null 安全降级返回 false。 |
| 执行步骤 | 1. 创建测试套件和用户自定义类 2. 编写 9 条 AC 对应测试用例 3. 构建消费方应用 4. 运行 aa test 5. 记录输出 |
| 验证命令 | `hdc shell aa test -b <bundle> -s unittest OpenHarmonyTestRunner -s class <SuiteName>` / 期望: 9 条 AC 全 PASS |
| 完成规则 | 不得修改框架源码；没有 aa test 运行输出，不得声明完成 |

## Plan 自审清单

- [x] 每个 P0/P1 AC 至少映射到一个 Task（AC-1~AC-9 均映射到 TASK-1/2/3）
- [x] 每个 Task 文件范围明确
- [x] 每个 Task 明确前置依赖、非目标、完成判据和停止条件
- [x] 每个 Task 有验证命令
- [x] Task 粒度形成能力闭环（TASK-1 动态树实现 / TASK-2 静态树实现 / TASK-3 验收测试）
- [x] 没有 TBD/TODO/占位符
- [x] 没有要求 Agent 自行寻找未列出的上下文
- [x] 交接信息自包含（Handoff Summary 完整）
- [x] 每个 Task 验证在完成时立即执行并记录证据
- [x] 无超 3000 行阈值的 Task（每个 Task 上下文 < 500 行）
