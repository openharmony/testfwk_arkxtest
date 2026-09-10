# 架构设计

> 确认 MockKit 模块的架构约束、关键设计决策、双树实现方案。

## 设计元数据

| 字段 | 内容 |
|------|------|
| Design ID | DESIGN-ISMOCKED-2026 |
| 关联需求 | `.codespec/changes/draft-20260903-ismocked-mock-status-query/proposal.md` |
| 关联 Epic | 无 |
| 目标 Feature | FEAT-ISMOCKED-2026 |
| 复杂度 | 标准 |
| 目标版本 | 引用 proposal.target_release（TBD） |
| Owner | [待确认] |
| 状态 | Draft |

## 需求基线

| 项 | 补充说明 |
|----|----------|
| 双树对等约束 | `src/`(MockKit.js) 和 `src_static/`(MockKit.ets) 须同步新增 isMocked，行为尽量对等 |
| 静态树 mockFunc 记录时机差异 | proposal Q-2 已确认：isMocked 仅查询已记录条目，不要求 mockFunc 立即可查。静态树 mockFunc 后需 when() 才在 hashMap 创建条目，此 gap 已接受 |
| 无独立构建 | jsunit 无 scripts/dependencies，验证方式为消费方测试应用构建+运行 |

## 上下文和现状

### 涉及仓和模块

| 仓库 | 补充架构说明 |
|------|-------------|
| testfwk_arkxtest (jsunit) | 单仓单组件变更。MockKit 类在动态树(src/module/mock/MockKit.js)和静态树(src_static/module/mock/MockKit.ets)各有独立实现，数据结构不同但 API 表面一致。MockKit 已在 index.js/index.ets 导出，新增方法不需改导出表。 |

### 调用链层级分析

| 层 | 模块 | 职责 | 修改类型 |
|----|------|------|----------|
| 测试应用 | 消费方测试用例 | 调用 mocker.isMocked(instance, name) 查询 Mock 状态 | 新增调用 |
| 框架入口 | index.js / index.ets | 导出 MockKit 类（已导出，无需改动） | 无修改 |
| Mock 模块 | MockKit.js / MockKit.ets | 新增 isMocked 实例方法，查询内部记录表 | 新增方法 |
| API 声明 | index.d.ts | MockKit 类新增 isMocked 方法签名声明 | 新增声明 |

**检查项：**
- [x] 调用链每一层都已覆盖（测试应用 → 框架入口 → Mock 模块 → API 声明）
- [x] 每层职责边界清晰，无跨层违规调用（纯框架内部，无 IPC/SA/内核调用）
- [x] 每层修改类型明确

### 适用架构规则

| Rule ID | 适用原因 | 设计结论 | 验证方式 |
|---------|----------|----------|----------|
| OH-ARCH-API-LEVEL | 新增 Public API（MockKit.isMocked） | 纯新增方法，向后兼容；无权限要求；SysCap = SystemCapability.Test.UiTest（沿用现有） | API 评审/XTS |
| OH-ARCH-LAYERING | 框架内部调用 | 无跨层调用，测试应用直接调用框架 API | 代码评审 |

## 不涉及项承接

| 维度 | 设计结论 |
|------|----------|
| 兼容性（proposal 标记"涉及"） | 纯新增方法，不改变现有 mockFunc/mockProperty/clear/ignoreMock/verify 行为；index.d.ts 新增方法声明，与已有方法名不冲突，向后兼容 |
| API/SDK（proposal 标记"涉及"） | MockKit 类新增 `isMocked(obj: Object, name: String): boolean`；在 index.d.ts 的 MockKit class 中追加方法声明 |

## 关键设计决策

| 决策 ID | 问题 | 推荐方案 | 探索过的替代方案 | 取舍理由 | 影响 |
|---------|------|----------|-----------------|------|------|
| ADR-1 | 动态树 isMocked 如何判断方法 mock 是否活跃？ | 方法 mock：从 `mockFuncResultMap` 找到 mock 函数 `f`，再查 `stubs` 是否有 `f` 的条目（`when()` 调用后才有）。属性 mock：比较 `obj[name]` 与 `propertyValueMap` 存储的原始值 | 备选1：仅查 `recordMockedMethod` 条目是否存在 → 放弃：`clear()` 不删除条目，会误返回 true；备选2：比较 `obj[name]` 与原始方法 → 放弃：`mockFunc` 后 `obj[name]` 已是 mock 包装函数（≠原始方法），即使未调 `when()` 也会误返回 true | `mockFunc` 仅替换方法为包装函数但未改行为（透传原方法），`when()` 才实际修改返回值。`clear()` 删除 `mockFuncResultMap` 条目但不删 `stubs`；`recordMockedMethod`/`propertyValueMap` 条目也不删。因此方法须查 `mockFuncResultMap`+`stubs`，属性须比较当前值与原始值 | 动态树实现需查 3 个 Map + 比较值 |
| ADR-2 | 静态树 isMocked 查询哪个记录表？ | 查询 `hashMap`（Map<MapKey, string>），遍历 key 匹配 classObj + methodName | 备选1：维护独立的 mock 注册表 → 放弃：需改 mockFunc/mockProperty 流程，违反"不改变现有 Mock 流程"约束；备选2：查询 propertyMap → 放弃：propertyMap 以 hashKey 为键，不含 classObj+methodName 信息，无法直接匹配 | hashMap 统一记录方法和属性 mock，key 含 classObj 和 methodName，一次遍历即可覆盖全部 | 静态树 mockFunc 后未调 when() 时 hashMap 无条目，isMocked 返回 false（已接受 gap） |
| ADR-3 | isMocked 对非法输入（instance 非对象、name 非字符串）如何处理？ | 不做显式校验，直接遍历记录表。非对象/非字符串不会匹配任何记录表条目，自然返回 false | 备选1：抛异常 → 放弃：与现有 clear() 的校验风格不一致且增加调用方负担；备选2：加 if 校验返回 false → 放弃：冗余，遍历自然返回 false | 查询操作应为幂等只读，对非法输入安全降级返回 false 而非报错 | 调用方传入非法参数时得到 false 而非异常 |

## 设计骨架

### 骨架范围

| 骨架项 | 目标 | 不包含 | 验证方式 |
|--------|------|--------|----------|
| API 接口骨架 | isMocked 方法签名 + index.d.ts 声明 | 完整查询逻辑 | index.d.ts 签名检查 |
| 动态树骨架 | MockKit.js 新增 isMocked 方法 | — | 源码审查 |
| 静态树骨架 | MockKit.ets 新增 isMocked 方法 | — | 源码审查 |
| 测试骨架 | XTS 测试用例覆盖 7 条 AC | — | aa test 运行 |

### 骨架 Spec 拆分

| Task ID | 目标 | 受影响文件 | AC |
|---------|------|------------|-----|
| TASK-1 | 动态树 isMocked 实现 + index.d.ts 声明 | MockKit.js, index.d.ts | AC-1~AC-7（动态树侧） |
| TASK-2 | 静态树 isMocked 实现 | MockKit.ets | AC-1~AC-7（静态树侧） |
| TASK-3 | XTS 验收测试用例 | 待定测试套 | AC-1~AC-7 |

## 后续 Task 拆分

| Task ID | 目标 | 受影响文件 | 依赖 |
|---------|------|------------|------|
| TASK-1 | 动态树 isMocked 实现 + index.d.ts | `jsunit/src/module/mock/MockKit.js`, `jsunit/index.d.ts` | design.md + spec.md Approved |
| TASK-2 | 静态树 isMocked 实现 | `jsunit/src_static/module/mock/MockKit.ets` | TASK-1 完成（参照动态树行为） |
| TASK-3 | XTS 验收测试用例 | 消费方测试应用 | TASK-1 + TASK-2 完成 |

## API 签名、Kit 与权限

### 新增 API

| API 签名 | 类型 | Kit | d.ts 位置 | 权限要求 | SysCap |
|----------|------|-----|-----------|----------|--------|
| `isMocked(obj: Object, name: String): boolean` | Public | @ohos/hypium（MockKit 类） | `jsunit/index.d.ts`（MockKit class 内追加） | - | - |

### 变更/废弃 API

无。

## 构建系统影响

### BUILD.gn 变更

无。jsunit 无 BUILD.gn（`package.json` 仅声明包名与入口，无 scripts/dependencies）。

### bundle.json 变更

无。

---

## 双树实现方案

### 动态树（src/module/mock/MockKit.js）

在 MockKit 类中新增 `isMocked` 实例方法：

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

**实现要点：**
- **方法 mock**：从 `mockFuncResultMap`（Map，key=`{obj, methodName}`，value=mock 函数 `f`）找到 mock 函数，再查 `stubs`（Map，key=mock 函数 `f`）是否有对应条目——有说明 `when()` 已调用，行为已实际修改
- **关键**：仅 `mockFunc` 而未调 `when()` 时，`stubs` 无条目 → 返回 false（mock 包装函数透传原方法，行为未修改）
- `clear()` 删除 `mockFuncResultMap` 条目 → 找不到 mock 函数 → false；`ignoreMock()` 同理
- **属性 mock**：`propertyValueMap`（Map，key=`{obj, methodName}`，value=原始属性值）有条目时，比较 `obj[name]` 与原始值，不一致说明仍被修改 → true
- `clear()` 恢复属性值但不删除 `propertyValueMap` 条目 → 比较结果一致 → false；`ignoreMock()` 删除条目 → 无匹配 → false
- `stubs instanceof Map` 防御 `reset()` 将 `stubs` 置为 `{}` 的情况
- 不修改任何记录表（只读查询）
- 代码风格：4 空格缩进、单引号、`function` 关键字（与现有 clear/ignoreMock 一致）

### 静态树（src_static/module/mock/MockKit.ets）

在 MockKit 类中新增 `isMocked` 实例方法：

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

**实现要点：**
- 遍历 `hashMap`（Map<MapKey, string>），key=`{methodName, args, classObj}`
- 匹配 `instance === keyClassObj && name === keyMethodName`
- 命中返回 true，遍历结束返回 false
- hashMap 统一记录方法 mock 和属性 mock，一次遍历覆盖全部
- 代码风格：2 空格缩进、`'use static'` 首行、完整类型注解、`object`/`string`/`boolean` 标量类型
- hilog 日志前缀（可选）：`hilog.info(domain, tag, '%{public}s', 'start run isMocked')`

### index.d.ts 声明

在 MockKit class 中追加：

```typescript
export class MockKit {
  constructor()
  mockFunc(obj: Object, func: Function): Function
  mockObject(obj: Object): Object
  verify(methodName: String, argsArray: Array<any>): VerificationMode
  ignoreMock(obj: Object, func: Function | String): void
  clear(obj: Object): void
  clearAll(): void
  mockPrivateFunc(originalObject: Object, method: String): Function
  mockProperty(obj: Object, propertyName: String, value: any): void
  isMocked(obj: Object, name: String): boolean  // ← 新增
}
```

**兼容性确认：** `isMocked` 是全新方法名，与现有方法无同名冲突。入参 `(Object, String)` 与现有方法参数不同，向后兼容。

### 导出表面

MockKit 已在 `index.js:260` 和 `index.ets` 中导出，新增实例方法不需改导出表。

## 已知 Gap（双树行为差异）

| 场景 | 动态树行为 | 静态树行为 | 原因 | 状态 |
|------|-----------|-----------|------|------|
| mockFunc 后、when() 前调用 isMocked | 返回 true（recordMockedMethod 立即记录） | 返回 false（hashMap 尚无条目） | 静态树 mockFunc 仅 setMockerOfUserClass，hashMap 条目在 AfterWhen.generateHashKey 时创建 | 已接受（proposal Q-2 确认） |
| mockProperty 后调用 isMocked | 返回 true（propertyValueMap 立即记录） | 返回 true（mockProperty 内部创建 AfterWhen，hashMap 有条目） | 静态树 mockProperty 直接调用 AfterWhen | 一致 |

## 风险和开放问题

| 项 | 类型 | 影响 | 处理方式 | Owner |
|----|------|------|----------|-------|
| 静态树 mockFunc+when() 前查询返回 false | 行为差异 | 低 | proposal Q-2 已确认接受；design.md 已知 Gap 表记录 | 需求方 |
| 静态树 ignoreMockProperty 有提前 return bug | 技术债务 | 低 | isMocked 为只读查询不受影响；如需修复 ignoreMockProperty 另行处理 | [待确认] |

## 设计审批

- [x] 需求基线已确认，设计覆盖 P0/P1 AC（AC-1~AC-7 全部引用）
- [x] 不涉及项已承接，N/A 和展开项都有结论
- [x] 涉及仓和模块职责清楚（单仓单组件）
- [x] 调用链层级分析完整，每层覆盖到位
- [x] 适用架构规则已识别并形成设计结论
- [x] 分层和子系统边界合规
- [x] API 变更有签名、权限、错误码和兼容性说明
- [x] BUILD.gn/bundle.json 影响明确（无影响）
- [x] 设计输出和后续 Task 拆分明确（TASK-1~TASK-3）
- [x] 关键设计决策有理由和影响说明（ADR-1~ADR-3）
- [x] 风险和开放问题有 Owner

**结论:** 通过
