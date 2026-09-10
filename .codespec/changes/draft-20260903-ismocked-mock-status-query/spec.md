# 特性规格

> 固化用户可见行为和验收标准。

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | Mock 对象清理状态获取能力（isMocked） |
| 特性编号 | FEAT-ISMOCKED-2026 |
| 所属 Epic | 无 |
| 优先级 | P1 |
| 目标版本 | 引用 proposal.target_release（TBD） |
| SIG 归属 | 无（jsunit 框架内部增强） |
| 状态 | Draft |
| 复杂度 | 标准 |

## 本次变更范围（Delta）

> lineage: new-on-legacy，在现有 MockKit 能力上新增。

| 类型 | 内容 | 说明 |
|------|------|------|
| ADDED | MockKit.isMocked(instance, name) 实例方法 | 新增 Mock 状态查询能力，开发者可在清理阶段编程式检查 Mock 是否已还原 |
| MODIFIED | 无 | 不改变现有 mockFunc/mockProperty/clear/ignoreMock 行为 |
| REMOVED | 无 | — |

## 输入文档

| 文档 | 路径 | 状态 |
|------|------|------|
| Requirement | `.codespec/changes/draft-20260903-ismocked-mock-status-query/proposal.md` | Approved（基线通过） |

## 用户故事

### US-1: 查询对象方法 Mock 状态

**作为** 应用开发者,
**我想要** 在 afterEach 清理阶段调用 `isMocked(instance, "methodName")` 查询对象方法是否仍处于 mock 状态,
**以便** 确认 Mock 已被正确清理，避免 Mock 残留导致下游用例误判。

**验收标准（AC, Acceptance Criteria）：**

| AC编号 | 验收标准 | 类型 |
|--------|----------|------|
| AC-1 | WHEN 对已 `mockFunc` 且通过 `when()` 设置返回值的方法调用 `isMocked(instance, "methodName")` THEN 返回 `true` | 正常 |
| AC-2 | WHEN 对已 mock 方法的对象调用 `clear(obj)` 或 `ignoreMock(obj, method)` 清理后再调用 `isMocked(instance, "methodName")` THEN 返回 `false` | 边界 |
| AC-3 | WHEN 对系统模块 API 调用 `isMocked(instance, "systemMethod")` THEN 返回 `false`（MockKit 不支持 mock 系统方法） | 边界 |
| AC-4 | WHEN 对未 mock 过的对象调用 `isMocked(instance, "anyName")` THEN 返回 `false` | 边界 |
| AC-5 | WHEN 对同一对象多次 mockFunc + when() 不同方法后调用 `isMocked` 查询任一已 mock 方法 THEN 返回 `true`；WHEN 全部清理后 THEN 返回 `false` | 正常 |
| AC-8 | WHEN 对已 `mockFunc` 但未调用 `when()` 设置返回值的方法调用 `isMocked(instance, "methodName")` THEN 返回 `false`（原方法行为未被实际修改） | 边界 |

### US-2: 查询对象属性 Mock 状态

**作为** 应用开发者,
**我想要** 调用 `isMocked(instance, "propertyName")` 查询对象属性是否仍处于 mock 状态,
**以便** 确认属性 Mock 已被正确清理。

**验收标准（AC, Acceptance Criteria）：**

| AC编号 | 验收标准 | 类型 |
|--------|----------|------|
| AC-6 | WHEN 对已 mock 属性的对象调用 `isMocked(instance, "propertyName")` THEN 返回 `true` | 正常 |
| AC-7 | WHEN 对已 mock 属性的对象调用 `clear(obj)` 或 `ignoreMock(obj, propertyName)` 清理后再调用 `isMocked(instance, "propertyName")` THEN 返回 `false` | 边界 |
| AC-9 | WHEN `name` 参数为空字符串 `""` 或 `null` 调用 `isMocked(instance, name)` THEN 返回 `false` | 边界 |

## 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-1 | R-1 | TBD | XTS 单元测试 | `[待生成]` |
| AC-2 | R-2 | TBD | XTS 单元测试 | `[待生成]` |
| AC-3 | R-3 | TBD | XTS 单元测试 | `[待生成]` |
| AC-4 | R-4 | TBD | XTS 单元测试 | `[待生成]` |
| AC-5 | R-5 | TBD | XTS 单元测试 | `[待生成]` |
| AC-6 | R-6 | TBD | XTS 单元测试 | `[待生成]` |
| AC-7 | R-7 | TBD | XTS 单元测试 | `[待生成]` |
| AC-8 | R-8 | TBD | XTS 单元测试 | `[待生成]` |
| AC-9 | R-9 | TBD | XTS 单元测试 | `[待生成]` |

## 规则定义

| 规则ID | 类型 | 触发条件 | 预期行为 | 边界/约束 | 关联AC |
|--------|------|----------|----------|-----------|--------|
| R-1 | 行为 | 对象 `instance` 的方法 `name` 已被 `mockFunc` mock 且通过 `when()` 设置了返回值（行为已实际修改），尚未被 `clear` 或 `ignoreMock` 清理 | `isMocked(instance, name)` 返回 `true` | name 为非空字符串；仅 `mockFunc` 未调 `when()` 不满足 | AC-1 |
| R-2 | 边界 | 对象 `instance` 的方法 `name` 曾被 mock 且 `when()` 已设置返回值，但已被 `clear(obj)` 或 `ignoreMock(obj, method)` 清理 | `isMocked(instance, name)` 返回 `false` | 清理后条目从记录表移除 | AC-2 |
| R-3 | 边界 | 对系统模块 API 调用 `isMocked`（MockKit 不支持 mock 系统方法，记录表无对应条目） | 返回 `false` | 系统方法始终未被 mock | AC-3 |
| R-4 | 边界 | 对象 `instance` 从未被任何 mock 操作处理过 | `isMocked(instance, name)` 返回 `false` | name 为任意非空字符串 | AC-4 |
| R-5 | 行为 | 对同一对象 `instance` 多次 mockFunc + when() 不同方法后，查询任一已 mock 方法 | 返回 `true`；全部清理后返回 `false` | 多次 mock 不互相覆盖 | AC-5 |
| R-6 | 行为 | 对象 `instance` 的属性 `name` 已被 `mockProperty` mock，且尚未被清理 | `isMocked(instance, name)` 返回 `true` | name 为非空字符串；mockProperty 直接改值，无需 when() | AC-6 |
| R-7 | 边界 | 对象 `instance` 的属性 `name` 曾被 mock，但已被 `clear(obj)` 或 `ignoreMock(obj, propertyName)` 清理 | `isMocked(instance, name)` 返回 `false` | 清理后条目从记录表移除 | AC-7 |
| R-8 | 边界 | 对象 `instance` 的方法 `name` 已被 `mockFunc` mock，但未调用 `when()` 设置返回值（原方法行为未被实际修改） | `isMocked(instance, name)` 返回 `false` | mockFunc 仅替换为包装函数透传原方法，when() 才实际修改返回值 | AC-8 |
| R-9 | 边界 | `name` 参数为空字符串 `""` 或 `null` | `isMocked(instance, name)` 返回 `false` | 非法输入安全降级，不抛异常 | AC-9 |

## 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-1 | R-1 / AC-1 | XTS 单元测试 | mockFunc + when() 后 isMocked 返回 true |
| VM-2 | R-2 / AC-2 | XTS 单元测试 | clear/ignoreMock 后 isMocked 返回 false |
| VM-3 | R-3 / AC-3 | XTS 单元测试 | 系统方法 isMocked 返回 false |
| VM-4 | R-4 / AC-4 | XTS 单元测试 | 未 mock 对象 isMocked 返回 false |
| VM-5 | R-5 / AC-5 | XTS 单元测试 | 多次 mockFunc+when() 后 true，全部清理后 false |
| VM-6 | R-6 / AC-6 | XTS 单元测试 | mock 属性后 isMocked 返回 true |
| VM-7 | R-7 / AC-7 | XTS 单元测试 | 清理属性 mock 后 isMocked 返回 false |
| VM-8 | R-8 / AC-8 | XTS 单元测试 | mockFunc 但未调 when() 时 isMocked 返回 false |
| VM-9 | R-9 / AC-9 | XTS 单元测试 | name 为空字符串或 null 时 isMocked 返回 false |

## API 变更分析

### 新增 API

| API 名称 | 开放范围 | 入参概要 | 返回值 | 错误码范围 | 功能描述 | 关联 AC |
|----------|----------|----------|--------|------------|----------|---------|
| `MockKit.isMocked` | Public | `instance: Object`, `name: String` | `boolean` | 无 | 查询对象的方法/属性是否仍处于 mock 状态（方法需 mockFunc + when() 才算 mock；属性 mockProperty 即算 mock；name 为空/null 返回 false） | AC-1~AC-9 |

### 变更/废弃 API

无。不改变现有 API 行为。

## 接口规格

### MockKit.isMocked

| 属性 | 值 |
|------|-----|
| 函数签名 | `isMocked(instance: Object, name: String): boolean` |
| 返回值 | `boolean` — `true` 表示对象的方法（已 mockFunc + when()）或属性（已 mockProperty）仍处于 mock 状态；`false` 表示未被 mock、仅 mockFunc 未调 when()、name 为空/null、或已被清理 |
| 开放范围 | Public |
| 错误码 | 无 |
| 关联 AC | AC-1~AC-9 |

**参数约束**

| 参数 | 类型 | 必填 | 默认值 | 约束条件 |
|------|------|------|--------|---------|
| instance | Object | 是 | — | 必须为对象类型（object 或 function）；非对象/非函数时行为由实现定义 |
| name | String | 是 | — | 方法名或属性名；为空字符串 `""` 或 `null` 时返回 `false`，不抛异常 |

**行为场景**

| # | 触发条件 | 预期行为 | 关联 AC |
|---|----------|----------|---------|
| 1 | instance 的方法 name 已被 mockFunc + when() mock，未被清理 | 返回 `true` | AC-1 |
| 2 | instance 的方法 name 已被 mock，已被 clear/ignoreMock 清理 | 返回 `false` | AC-2 |
| 3 | instance 为系统模块对象，name 为系统方法 | 返回 `false` | AC-3 |
| 4 | instance 从未被 mock | 返回 `false` | AC-4 |
| 5 | instance 的多个方法已被 mockFunc + when() mock，查询其中任一 | 返回 `true`；全部清理后返回 `false` | AC-5 |
| 6 | instance 的属性 name 已被 mockProperty mock，未被清理 | 返回 `true` | AC-6 |
| 7 | instance 的属性 name 已被 mock，已被 clear/ignoreMock 清理 | 返回 `false` | AC-7 |
| 8 | instance 的方法 name 已被 mockFunc 但未调 when()（行为未修改） | 返回 `false` | AC-8 |
| 9 | name 为空字符串 `""` 或 `null` | 返回 `false` | AC-9 |

## 兼容性声明

- **已有 API 行为变更:** 否。纯新增方法，不改变 mockFunc/mockProperty/clear/ignoreMock/verify 等现有 API 行为。
- **配置文件格式变更:** 否。
- **数据存储格式变更:** 否。
- **最低支持版本:** API 8（与 Hypium 框架一致）
- **API 版本号策略:** 新增方法无需 @since 标注策略变更；MockKit 类新增方法，入参个数与已有方法不同，向后兼容。

## 架构约束

| 关键约束 | 约束说明 | 影响 AC |
|----------|----------|---------|
| 双树对等 | ArkTS-Dynamic（`src/`）和 ArkTS-Static（`src_static/`）须同步实现 isMocked | AC-1~AC-9 |
| 不改变现有 Mock 流程 | isMocked 为只读查询，不修改 mock 记录表 | AC-1~AC-9 |
| 方法 mock 需 when() 才算活跃 | mockFunc 仅替换为包装函数透传原方法，when() 才实际修改返回值；isMocked 对仅 mockFunc 未调 when() 的方法返回 false | AC-1, AC-5, AC-8 |
| 静态树 mockFunc 记录时机差异 | 静态树中 mockFunc 调用后需 `when()` 才在记录表创建条目；isMocked 仅查询已记录条目，mockFunc 后、when() 前查询返回 false（与动态树行为一致） | AC-1, AC-5, AC-8 |

## 非功能性需求

> proposal 不涉及项确认：性能/安全/可靠性均为 N/A。

| 类型 | 指标/阈值 | 验证方式 | 证据 |
|------|-----------|----------|------|
| 性能 | N/A | — | — |
| 功耗 | N/A | — | — |
| 内存 | N/A | — | — |
| 安全 | N/A | — | — |
| 可靠性 | N/A | — | — |
| 可测试性 | isMocked 返回值为 boolean，可直接用于 expect 断言 | XTS 单元测试 | `[待生成]` |

## 多设备适配声明

无差异。jsunit 为测试框架内部组件，不涉及设备形态差异。

## 全局特性影响

| 特性 | 适用？ | 结论 | 关联场景 |
|------|--------|------|----------|
| 无障碍 | 否 | N/A | — |
| 大字体 | 否 | N/A | — |
| 深色模式 | 否 | N/A | — |
| 多窗口/分屏 | 否 | N/A | — |
| 多用户 | 否 | N/A | — |
| 版本升级 | 否 | N/A | — |
| 生态兼容 | 是 | 向后兼容，纯新增 API | — |

## Spec 自审清单

- [x] 无"待定""TBD""TODO"等占位符（target_release 引用 proposal，属引用非占位）
- [x] 所有 AC 使用 WHEN/THEN 格式，可独立测试
- [x] 范围边界明确（做什么/不做什么清晰）
- [x] 无语义模糊表述
- [x] AC 与规则表交叉一致（9 个 AC 对应 9 条规则，每条规则至少关联一个 AC）
- [x] 规则表每条通过 5 项质量检查（可复现/可观测/边界值/关联AC/无冲突）

## context-references

```yaml
context-queries:
  - repo: "testfwk_arkxtest"
    query: "MockKit 内部记录表数据结构（recordMockedMethod/propertyValueMap 动态树，hashMap 静态树）及 mockFunc/mockProperty 的记录时机差异"
```

**关键文档：** `jsunit/AGENTS.md` § Mock 能力 / § 架构映射（动态↔静态）
