---
target_release:
  id: TBD
  status: proposed
---

# 需求文档

> 一份文档，从原始需求到基线结论。按阶段追加内容，不拆成多份独立文件。

## 一、原始需求

### 基本信息

| 字段 | 内容 |
|------|------|
| 需求ID | REQ-ISMOCKED-2026 |
| 需求名称 | 单元测试框架能力增强-新增 Mock 对象清理状态获取能力 |
| 来源 | 社区反馈/缺陷复盘（Mock 残留导致下游用例误判） |
| 提出人 | [待确认] |
| 目标发行版本 | TBD |
| 候选 Profile | none（testfwk_arkxtest 不匹配 arkweb/arkui/arkgraphic/arkdata） |
| 优先级 | P1 |
| 状态 | Baselined |

### 原始描述

**原始问题：** 开发者在测试用例 afterEach 清理阶段需确认 Mock 对象已还原，当前框架无任何接口可查询对象的 Mock 状态。Mock 残留（忘记 clear 或 ignoreMock）会导致下游用例误判，且难以定位根因——开发者无法在清理阶段编程式地检查 Mock 是否已被正确清理。

**痛点：**

| 用户类型 | 当前痛点 | 影响 |
|----------|----------|------|
| 应用开发者 | afterEach 中无法查询 Mock 是否已清理，只能盲目信任 clear 已生效 | Mock 残留导致后续用例在非预期 Mock 状态下执行，结果不可靠 |
| 应用开发者 | Mock 残留问题难以定位，需手动排查每个 Mock 调用 | 调试成本高，测试可信度下降 |

**期望结果：** 提供 `isMocked` 接口，使开发者可查询用户自定义类对象的 Mock 状态（某方法/属性是否仍处于 mock 状态），从而在清理阶段编程式地确认 Mock 已还原。MockKit 不支持 mock 系统模块 API，因此对系统方法调用 isMocked 始终返回 false。

### 背景证据

| 证据类型 | 链接/路径 | 说明 |
|----------|-----------|------|
| 源码现状 | `jsunit/src/module/mock/MockKit.js` | MockKit 有 `recordMockedMethod`(Map) 和 `propertyValueMap`(Map) 记录 mock 条目，`clear`/`ignoreMock` 清理时删除条目，但无查询接口 |
| 源码现状 | `jsunit/src_static/module/mock/MockKit.ets` | 静态树用 `hashMap`(Map<MapKey,string>) 统一记录方法和属性 mock，同样无查询接口 |
| API 声明 | `jsunit/index.d.ts` | MockKit 类声明无 `isMocked` 方法 |
| AGENTS 指南 | `jsunit/AGENTS.md` § Mock 能力 | 现有 Mock 工作流：mockFunc → when → verify → clear，无状态查询环节 |

### 初始范围

**可能包含：**
- MockKit 类新增 `isMocked(instance, name)` 实例方法，返回 boolean
- 动态树（`src/module/mock/MockKit.js`）实现：查询 `recordMockedMethod` + `propertyValueMap`
- 静态树（`src_static/module/mock/MockKit.ets`）实现：查询 `hashMap`
- `index.d.ts` 新增方法声明
- 验收测试用例

**明确不包含：**
- 系统模块 API 的 mock 状态查询（MockKit 不支持 mock 系统方法，始终返回 false）
- mockObject 返回的副本对象的 mock 状态查询（副本非原始对象，不在记录表中）
- Mock 调用次数/参数的查询（已有 `verify` + `VerificationMode` 覆盖）

### 初始假设

| 假设 | 类型 | 验证方式 | 状态 |
|------|------|----------|------|
| `isMocked` 是 MockKit 实例方法（非静态方法），与 mockFunc/mockProperty/clear 一致 | 技术 | 源码确认：现有 API 均为实例方法 | 已验证 |
| 动态树 mockFunc 调用后立即在 recordMockedMethod 记录条目 | 技术 | 源码确认：MockKit.js:341 `this.recordMockedMethod.set(info, originalMethod)` | 已验证 |
| 静态树 mockFunc 调用后不在 hashMap 记录条目，需 when() 才记录 | 技术 | 源码确认：MockKit.ets mockFunc→mockPrivateFunc→setMockerOfUserClass，hashMap 条目在 AfterWhen.generateHashKey 时创建 | 已验证 |
| 对系统方法调用 isMocked 返回 false（记录表中不存在条目） | 业务 | 需求方描述明确 | 已验证 |
| 新增方法不影响现有 API 行为，向后兼容 | 兼容性 | 纯新增方法，无同名方法冲突 | 已验证 |

### 初始分级判断

| 判断项 | 结果 | 依据 |
|--------|------|------|
| 复杂度 | 标准 | 单组件（jsunit Mock 模块），双树（src/ + src_static/），新增公共 API；不涉及多仓/SIG/安全性能关键路径 |
| 涉及仓数量 | 1 | arkxtest（jsunit 子组件） |
| 是否涉及 Public/System API | 是 | 新增 MockKit.isMocked 到 index.d.ts（纯新增方法，向后兼容） |
| 是否涉及安全/性能关键路径 | 否 | 查询操作，无安全/性能影响 |
| 是否跨 SIG | 否 | 仅 jsunit 组件 |

### 进入澄清条件

- [x] 原始问题和期望结果已记录
- [x] 需求来源和责任人已明确（来源：社区反馈/缺陷复盘；责任人：待确认）
- [x] 初始范围和不包含项已记录
- [x] 关键假设和待澄清问题已列出
- [x] 复杂度有判断（标准）

---

## 二、澄清记录

> 澄清是逐轮对话，不是一次性填表。标准级：按主题分组，每轮提出 3-5 个相关问题。

### 待澄清问题

| 编号 | 问题 | 为什么需要澄清 | 状态 |
|------|------|----------------|------|
| Q-1 | `isMocked` 的调用形式是 `mocker.isMocked(instance, name)`（实例方法）还是 `MockKit.isMocked(instance, name)`（静态方法）？ | 影响 API 签名和 index.d.ts 声明；现有 mockFunc/clear/ignoreMock 均为实例方法 | 已澄清：实例方法 |
| Q-2 | 静态树中 `mockFunc` 调用后不立即在 hashMap 记录（需 `when()` 才记录）。动态树 mockFunc 立即记录。`isMocked` 在两棵树的行为是否需要统一？ | 双树架构差异可能导致同一用法在两棵树下返回不同结果，违背双树对等约束 | 已澄清：仅查询已记录条目，不要求 mockFunc 立即可查 |
| Q-3 | `name` 参数同时接受方法名和属性名，是否需要区分？ | 影响 API 语义清晰度 | 已澄清：不区分，同时查 recordMockedMethod + propertyValueMap（动态树）/ hashMap（静态树），任一命中返回 true |
| Q-4 | 目标发行版本是哪个？ | target_release 需在基线时确认 | 已澄清：TBD（待定） |
| Q-5 | `isMocked` 对未传入任何 mock 操作的全新对象应返回 false——这是否符合期望？ | 确认边界行为 | 已澄清：符合，AC-4 已覆盖 |

### 讨论记录

| 日期 | 参与人 | 讨论主题 | 结论 | 后续动作 |
|------|--------|----------|------|----------|
| 2026-09-03 | AI Agent | 初始需求分析 + 源码探查 | 识别双树架构差异（Q-2）为关键设计点 | 需求方确认 |
| 2026-09-03 | AI Agent + 需求方 | 标准级澄清第 1 轮 | Q-1 实例方法；Q-2 仅查询已记录条目；Q-3 不区分方法/属性；Q-4 TBD；Q-5 符合 | 无 |

### 功能范围确认

| 问题 | 回答 | 确认人 | 状态 |
|------|------|--------|------|
| 核心功能包含哪些？ | isMocked(instance, name) 查询 mock 状态，返回 boolean | 需求方 | 已确认 |
| 明确不包含哪些？ | 系统方法查询、mockObject 副本查询、调用次数查询 | 需求方 | 已确认 |
| 是否有分期策略？ | 无 | 需求方 | 已确认 |

### 方案探索

> 标准及以上复杂度必填。

| 编号 | 方案概述 | 优势 | 风险/代价 | 选择结论 |
|------|----------|------|-----------|----------|
| A-1 | isMocked 查询现有记录表（动态树 recordMockedMethod+propertyValueMap，静态树 hashMap），不改变 mockFunc 的记录时机 | 改动最小，不触碰现有 mock 流程 | 静态树在 mockFunc 后、when() 前调用 isMocked 返回 false，与动态树不一致 | 推荐 |
| A-2 | isMocked 查询记录表，同时在静态树 mockFunc/mockPrivateFunc 阶段提前记录条目到 hashMap（或在单独的 mock 注册表） | 双树行为统一 | 改动静态树 mockFunc 流程，需确认不影响现有行为 | 放弃 |

**取舍理由：** 需求方明确选择"仅查询已记录条目，不要求 mockFunc 立即可查"（Q-2）。选 A-1：改动最小，不触碰现有 mock 流程，双树差异作为已知 gap 记录在 design.md 中。

### 上下文与知识源检索日志

| 编号 | 来源 | 查询/读取内容 | 关键发现 | 可信度 | 用于 | 命中/原因 |
|------|------|---------------|----------|--------|------|-----------|
| K-1 | 源码 `jsunit/src/module/mock/MockKit.js` | MockKit 类完整实现，recordMockedMethod/propertyValueMap/mockFuncResultMap 数据结构和记录时机 | mockFunc 在 :341 立即 set recordMockedMethod；mockProperty 在 :381 set propertyValueMap；clear 遍历两个 Map 恢复+删除；无 isMocked 方法 | 高 | 范围/API/设计 | 命中 |
| K-2 | 源码 `jsunit/src_static/module/mock/MockKit.ets` | 静态树 MockKit 完整实现，hashMap/propertyMap 数据结构 | hashMap 统一记录方法和属性 mock（key=MapKey{methodName,args,classObj}）；mockFunc→mockPrivateFunc→setMockerOfUserClass 不直接写 hashMap，条目在 AfterWhen.generateHashKey 时创建；clear 遍历 hashMap 恢复+删除 | 高 | 范围/API/设计 | 命中 |
| K-3 | `jsunit/index.d.ts` | MockKit 类公共 API 声明 | MockKit 类声明了 mockFunc/mockObject/verify/ignoreMock/clear/clearAll/mockPrivateFunc/mockProperty，无 isMocked；新增方法为纯新增，向后兼容 | 高 | API 兼容性 | 命中 |
| K-4 | `jsunit/AGENTS.md` | § Mock 能力、§ 架构映射（动态↔静态）、§ 项目约束（双树对等）、§ 关键 API | 双树对等约束；Mock 工作流示例；静态树语法规则（'use static'、int/double、AnyType、Map 代替对象字面量） | 高 | 设计约束 | 命中 |
| K-5 | `jsunit/index.js` / `jsunit/index.ets` | MockKit 导出方式 | MockKit 已在 index.js:260 和 index.ets 中导出，新增方法不需改导出表 | 高 | API 范围 | 命中 |
| K-6 | 根 `AGENTS.md` § 项目约束 | 公共 API .d.ts 兼容性规则 | "类中新增同名方法时，入参个数不同是兼容的"——isMocked 是全新方法名，无冲突，纯兼容新增 | 高 | 兼容性 | 命中 |

**上下文结论：**
- 高可信结论：isMocked 是纯新增 API，向后兼容；动态树实现可直接查询 recordMockedMethod + propertyValueMap；静态树实现可查询 hashMap
- 待确认结论：静态树 mockFunc 不立即记录 hashMap 条目的架构差异，是否导致双树行为不一致，需需求方确认接受范围
- 未使用来源及原因：未查询 DeepWiki（本仓为 testfwk_arkxtest，非 OpenHarmony 子系统仓，DeepWiki 无对应条目）；未查询多仓知识库（单仓变更）

### 子系统影响

| 问题 | 回答 | 确认人 | 状态 |
|------|------|--------|------|
| 涉及哪些子系统？ | jsunit（Hypium 单元测试框架），Mock 模块 | [需求方] | 已确认 |
| 是否需要新增子系统或部件？ | 否 | [需求方] | 已确认 |

### API 变更评估

| 问题 | 回答 | 确认人 | 状态 |
|------|------|--------|------|
| 是否需要新增/修改 Public API？ | 是，新增 1 个方法：MockKit.isMocked(instance, name) | 需求方 | 已确认 |
| 是否需要新增 System API？ | 否 | [需求方] | 已确认 |
| 是否会废弃已有 API？ | 否 | [需求方] | 已确认 |
| 是否需要新增权限声明？ | 否 | [需求方] | 已确认 |

### 兼容性与非功能需求

| 类别 | 核心问题 | 结论 | 确认人 | 状态 |
|------|----------|------|--------|------|
| 兼容性 | 向前/向后兼容要求？破坏性变更？ | 纯新增方法，向后兼容；不改变现有 API 行为 | [需求方] | 已确认 |
| 性能 | 响应时间/内存/并发要求？ | 查询操作，O(n) 遍历 Map（n=mock 条目数），无性能影响 | [需求方] | 已确认 |
| 安全 | 权限/隐私/加密/审计要求？ | 无 | [需求方] | 已确认 |
| 可靠性 | 崩溃率/容错/恢复要求？ | 无 | [需求方] | 已确认 |

### 依赖与风险

| 依赖项 | 类型 | 说明 | 状态 |
|--------|------|------|------|
| 无外部依赖 | — | 纯框架内部改动 | 已确认 |

| 风险 | 类型 | 影响 | 缓解措施 | 状态 |
|------|------|------|----------|------|
| 双树架构差异导致行为不一致 | 技术 | 中 | 需求方接受差异（Q-2），选方案 A-1，design.md 记录 gap 点 | 已确认 |
| 静态树语法合规（'use static'、类型注解） | 技术 | 低 | 按 jsunit/AGENTS.md § 代码风格 规范 | 已确认 |

### AC 完整性

- [x] 每个用户故事有验收标准
- [x] AC 全部使用 WHEN/THEN 格式
- [x] 覆盖正常流程、异常流程、边界条件
- [x] AC 可测试、可度量

### 澄清结论

- [x] 功能范围已完全明确
- [x] 子系统影响已识别
- [x] API 变更已评估
- [x] 兼容性和非功能需求已确认
- [x] 依赖和风险已识别且有缓解方案
- [x] AC 完整可测试
- [x] 标准及以上复杂度已完成方案探索（至少 2 个方案 + 取舍理由）

**结论:** 通过

---

## 三、需求基线

> 澄清完成后固化。manifest.md 是事实源，此处为审批结论。

### 基线信息

| 字段 | 内容 |
|------|------|
| 基线版本 | v1.0 |
| 基线日期 | 2026-09-03 |
| Owner | [待确认] |
| 确认人 | 需求方 |
| 复杂度 | 标准 |
| Profile | none |
| 目标发行版本 | TBD（引用 proposal.target_release） |
| 版本状态 | proposed |

### 问题陈述

Hypium 单元测试框架的 MockKit 提供 mockFunc/mockProperty/clear/ignoreMock 等 Mock 能力，但缺少查询 Mock 状态的接口。开发者在 afterEach 清理阶段无法编程式地确认 Mock 是否已被正确清理，Mock 残留导致下游用例在非预期状态下执行且难以定位根因。需新增 isMocked 实例方法，查询对象的方法/属性是否仍处于 mock 状态。

### 目标和成功指标

| 目标 | 成功指标 | 验证方式 |
|------|----------|----------|
| 提供 Mock 状态查询能力 | isMocked 对已 mock 条目返回 true，清理后返回 false | XTS 测试用例验证 5 条 AC |
| 向后兼容 | 现有 API 行为不变，消费方测试应用构建通过 | 消费方构建 + aa test 运行 |

### 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-1 | 作为应用开发者，我想要在 afterEach 中查询 Mock 对象的方法是否仍处于 mock 状态，以便确认 Mock 已被正确清理 | P0 |
| US-2 | 作为应用开发者，我想要查询 Mock 对象的属性是否仍处于 mock 状态，以便确认属性 Mock 已被正确清理 | P0 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-1 | WHEN 对已 mock 方法的对象调用 isMocked(instance, "methodName") THEN 返回 true；WHEN 调用 clear/ignoreMock 清理后 THEN 返回 false | 正常/边界 | US-1 |
| AC-2 | WHEN 对已 mock 属性的对象调用 isMocked(instance, "propertyName") THEN 返回 true；WHEN 清理后 THEN 返回 false | 正常/边界 | US-2 |
| AC-3 | WHEN 对系统模块 API 调用 isMocked THEN 返回 false（MockKit 不支持 mock 系统方法，记录表无条目） | 边界 | US-1 |
| AC-4 | WHEN 对未 mock 过的对象调用 isMocked THEN 返回 false | 边界 | US-1 |
| AC-5 | WHEN 对同一对象多次 mock 后调用 isMocked THEN 返回 true；WHEN 全部清理后 THEN 返回 false | 正常 | US-1 |

### 范围边界

**包含：** MockKit 类新增 isMocked(instance, name) 实例方法；动态树（src/）和静态树（src_static/）实现；index.d.ts 声明；XTS 验收测试
**不包含：** 系统模块 API mock 状态查询；mockObject 副本查询；调用次数/参数查询

### 影响范围

| 子系统 | 仓库 | 模块/路径 | 当前职责 | 影响类型 | Owner |
|--------|------|-----------|----------|----------|-------|
| jsunit | testfwk_arkxtest | jsunit/src/module/mock/MockKit.js | 动态树 MockKit 实现 | 新增方法 | [待确认] |
| jsunit | testfwk_arkxtest | jsunit/src_static/module/mock/MockKit.ets | 静态树 MockKit 实现 | 新增方法 | [待确认] |
| jsunit | testfwk_arkxtest | jsunit/index.d.ts | 公共 API 类型声明 | 新增方法声明 | [待确认] |

### API 变更项清单

| API 名称 | 变更类型 | 开放范围 | 概要说明 |
|----------|----------|----------|----------|
| MockKit.isMocked(instance: Object, name: String) | 新增 | Public | 查询对象的方法/属性是否仍处于 mock 状态，返回 boolean |

### 不涉及项确认

| 维度 | 涉及？ | 依据 | 若涉及，进入哪个下游文档 |
|------|--------|------|--------------------------|
| 性能 | 否 | 查询操作，O(n) 遍历，无性能影响 | N/A |
| 安全与权限 | 否 | 无权限/隐私/加密需求 | N/A |
| 兼容性 | 是 | 纯新增 API，需确认向后兼容 | spec.md |
| API/SDK | 是 | 新增 MockKit.isMocked 方法到 index.d.ts | design.md / spec.md |
| IPC/跨进程 | 否 | 框架内部，无 IPC | N/A |
| 构建与部件 | 否 | 无构建系统改动 | N/A |
| 国际化/无障碍 | 否 | 无 UI/国际化 | N/A |
| 数据迁移 | 否 | 无数据持久化 | N/A |

### 变更控制

| 变更类型 | 触发条件 | 处理规则 |
|----------|----------|----------|
| 范围新增 | 新增用户故事或仓/模块 | 重新评估复杂度和设计影响 |
| AC 变更 | 修改可观察行为或错误码 | 重新审批基线和 Spec |
| API 变更 | 新增/修改 Public/System API | 触发设计审批 |
| 非功能指标变更 | 性能/安全/兼容性阈值变化 | 重新确认测试计划 |
| 目标版本变更 | 交付版本调整 | 更新 proposal.target_release |

### 进入设计/Spec 条件

- [x] 所有 P0/P1 用户故事有 AC
- [x] 每条 AC 可测试、可度量
- [x] 范围内/外已确认
- [x] `proposal.target_release` 已确认或明确 TBD
- [x] `manifest.profile` 已确认或明确 none
- [x] 涉及仓、模块、SIG 已识别
- [x] 不涉及项已标记 N/A
- [x] 变更控制规则已确认
- [x] 标准及以上复杂度的澄清问题已逐项关闭，且讨论记录包含需求方/Owner/SIG 明确确认
- [x] 上下文与知识源检索日志已填写；未查询关键来源的原因已记录
- [x] 目标仓 Agent 指南已检查并记录关键约束（jsunit/AGENTS.md § Mock 能力 / § 架构映射 / § 项目约束 双树对等）

**基线结论:** 通过
