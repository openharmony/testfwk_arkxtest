# Spec 检查报告

> 本报告由 ohos-test-spec-rule-checking 技能自动生成。

## 一、检查概要

| 项目 | 内容 |
|------|------|
| Spec 文件 | `C:\Users\zk\Desktop\testfwk_arkxtest\.codespec\changes\draft-20260903-ismocked-mock-status-query\spec.md` |
| Proposal 文件 | `C:\Users\zk\Desktop\testfwk_arkxtest\.codespec\changes\draft-20260903-ismocked-mock-status-query\proposal.md` |
| 检查时间 | 2026-09-08 |
| 检查工具 | ohos-test-spec-rule-checking v1.0（机械检查 + 语义检查） |
| 领域规则 | 未加载（spec 为测试框架内部增强，无特定领域归属；manifest profile: none） |
| 机械检查 | 已执行 |

### 结果统计

| 严重级别 | 规则总数 | 通过 ✅ | 不通过 ❌ | 无法判定 ⚠️ |
|----------|----------|---------|-----------|-------------|
| 🔴 Blocker | 26 | 25 | 0 | 1 |
| 🟡 Warning | 20 | 18 | 0 | 2 |
| **合计** | **46** | **43** | **0** | **3** |

> **结论：** ✅ 可进入测试设计——所有规则通过或豁免
>
> **结论判定算法（严格按以下步骤执行，不得跳步）：**
> 1. 从**最终合并结果**（机械检查 + 语义检查第 5 步完成后的结果）中统计：
>    - `blocker_fail` = 🔴 Blocker 级别中 status=fail 的条数 = **0**
>    - `warning_fail` = 🟡 Warning 级别中 status=fail 的条数 = **0**
> 2. 按以下优先级判定（只取第一个匹配的）：
>    - `blocker_fail > 0` → 「❌ 不可进入测试设计——须先修复 {blocker_fail} 项 🔴 Blocker 不通过项」
>    - `warning_fail > 0` → 「⚠️ 可进入测试设计——建议尽快修复 {warning_fail} 项 🟡 Warning 不通过项」
>    - 其余 → 「✅ 可进入测试设计——所有规则通过或豁免」 ✓
>
> **一致性约束（Issue #69）：**
> - `needs_review` 和 `skip` **不计入** fail。`needs_review` 须由语义检查（第 5 步）判定为 pass/fail 后再纳入统计。
> - 结论中的 `blocker_fail`/`warning_fail` 数字**必须与上方「结果统计」表的「不通过 ❌」列完全一致**。
> - 口头反馈给用户的结论**必须与报告 `{{OVERALL_VERDICT}}` 一致**，不得出现"口头说可进入、报告写不可进入"的矛盾。

---

## 二、通用规则检查结果

> 规则全集见 `references/general-rules.md`，共 46 条，覆盖 8 个维度。
> **"结果"列固定使用图标**：✅ 通过 / ❌ 不通过 / ⚠️ 无法判定（含 skip），与上方"结果统计"表一致，不得混用文字。
> **同根因命中归并**：同一根因被多条规则命中时，以最高严重级别规则为主条目呈现，其余在证据列标注"关联命中（同根因，见 SC-XX）"，修复建议只写一次。

### 🔴 Blocker · 必须修复

| 规则编号 | 规则名称 | 维度 | 结果 | 证据 / 缺失说明 |
|----------|----------|------|------|----------------|
| SC-OV-01 | 必填字段完整性 | 概述与完整性 | ✅ | 概述表必填字段（特性名称/编号/优先级/状态/复杂度等）均已填写。注：目标版本字段值为"引用 proposal.target_release（TBD）"，TBD 属 proposal 级待确认项，spec 正确引用而非自创占位 |
| SC-OV-02 | 内容完整性 | 概述与完整性 | ✅ | 所有章节（概述/Delta/用户故事/规则定义/验证映射/API 变更/接口规格/兼容性声明/架构约束/非功能性需求等）均有实质内容 |
| SC-OV-03 | 语言一致性 | 概述与完整性 | ✅ | 机械 fail 覆盖为 pass：L156 兼容性声明行"已有 API 行为变更: 否。纯新增方法，不改变 mockFunc/mockProperty/clear/ignoreMock/verify 等现有 API 行为"为中文句，其中 mockFunc/mockProperty/clear/ignoreMock/verify 为 Public API 方法名（行内代码），非整段英文规格描述 |
| SC-OV-05 | 兼容性声明必要性 | 概述与完整性 | ✅ | 变更/废弃 API 表为"无"，兼容性声明非必需但已自愿提供（3 项均声明"否"） |
| SC-AC-01 | 用户视角而非内部实现视角 | 用户故事与验收标准 | ✅ | 机械 fail 覆盖为 pass（两层判定均通过）：(a) AC 中 isMocked/mockFunc/when()/clear/ignoreMock 均为 Public API 方法名，非私有标识符（如 controllerMap_）；(b) AC 从应用开发者可观测视角编写，WHEN 描述调用 isMocked（含 mockFunc+when() 前置条件、空/null name 边界），THEN 描述可观测返回值（true/false） |
| SC-AC-03 | 典型用户场景覆盖 | 用户故事与验收标准 | ✅ | US-1/US-2 均含"作为应用开发者，我想要…，以便…"格式，角色+操作+预期结果完整 |
| SC-AC-04 | 新增与既有场景区分与内容不重复 | 用户故事与验收标准 | ✅ | 子项 a：Delta 表有 ADDED/MODIFIED/REMOVED 标记；子项 b：机械 Jaccard<0.8，人工复核 AC-1/AC-6（方法/属性 mock）为合理相似（同接口不同参数语义）；AC-8（mockFunc 未调 when()→false）与 AC-1（mockFunc+when()→true）互补非重复；AC-9（空/null name→false）为独立边界场景 |
| SC-AC-06 | 规格颗粒度适中 | 用户故事与验收标准 | ✅ | 每条 AC 聚焦单一行为（查询一种状态），THEN 简洁（返回 true/false），无 5+ 步调用链 |
| SC-AC-07 | 返回状态/结果格式与枚举明确性 | 用户故事与验收标准 | ✅ | 返回类型为 boolean，AC 明确断言 `返回 true` / `返回 false`，枚举取值集合完整；返回值描述已细化（true=方法 mockFunc+when() 或属性 mockProperty 活跃；false=未 mock/仅 mockFunc 未调 when()/name 为空/null/已清理） |
| SC-AC-08 | 命令覆盖完整性 | 用户故事与验收标准 | ⚠️ | 非 CLI spec（概述无子命令字段，AC 无 executable 模式），不适用 |
| SC-BR-01 | 禁止纯内部实现作为业务规则 | 业务规则 | ✅ | 规则 R-1~R-9 描述黑盒可验证行为（调用 isMocked 返回 true/false）；R-8 约束列含"mockFunc 仅替换为包装函数透传原方法"属实现机制描述，但用词为 Public API 名+通用概念，非内部私有标识符（如 controllerMap_）；R-9 约束"非法输入安全降级，不抛异常"为黑盒行为约束；详细实现已在"架构约束"章节 |
| SC-EX-01 | 超时阈值精确性 | 异常与边界规则 | ✅ | spec 不涉及超时/timeout/延迟场景，非功能需求性能项为 N/A |
| SC-EX-02 | 异常构造手段说明与跨表一致性 | 异常与边界规则 | ✅ | API 声明"错误码: 无"，返回 boolean 不抛异常；所有场景返回 true/false（含空/null name 安全降级），无错误路径，无跨表冲突 |
| SC-EC-01 | 错误码精确性与全局一致性 | 错误码定义 | ✅ | API 声明无错误码（"错误码: 无"/"错误码范围: 无"），spec 正文无孤立码 |
| SC-EC-02 | 错误码与返回值类型一致 | 错误码定义 | ✅ | 返回 boolean，无错误码，类型一致 |
| SC-API-01 | 开放级别标注 | 接口变更分析 | ✅ | API 变更分析表"开放范围"列标注为 Public |
| SC-API-02 | 对外 API 全集列举 | 接口变更分析 | ✅ | 仅 1 个 Public API（MockKit.isMocked），无"等""类似""参考"等省略词；变更/废弃 API 为"无" |
| SC-API-03 | 接口参数规格完整性 | 接口变更分析 | ✅ | instance: Object（必填，约束：对象类型或 function）；name: String（必填，约束：为空字符串或 null 时返回 false，不抛异常）；返回值 boolean 含含义说明 |
| SC-API-07 | 权限描述规范性 | 接口变更分析 | ✅ | spec 无权限相关描述（proposal 确认"否"），N/A |
| SC-VT-01 | 端到端验证方式 | 验证与测试设计 | ✅ | 验证方式为 XTS 测试（OpenHarmony 兼容性测试套），属外部黑盒验证而非内部单测；验证映射表 VM-1~VM-9 含验证重点描述；对测试框架 API 而言，XTS 用例即为用户视角端到端验证 |
| SC-VT-02 | 底层能力接口的测试路径 | 验证与测试设计 | ✅ | isMocked 有直接用户场景（afterEach 清理阶段查询 Mock 状态），非无用户场景的底层能力接口 |
| SC-VT-06 | 验证映射完整性 | 验证与测试设计 | ✅ | 机械 needs_review 覆盖为 pass：验证映射表 VM-1~VM-9 对应 AC-1~AC-9，验证方式列非空且非 TBD（"XTS 单元测试"），验证重点列均有具体描述 |
| SC-GN-01 | Proposal 前置依赖 | 整体规范 | ✅ | proposal.md 存在于 spec 同目录且非空（304 行，基线状态 Baselined） |
| SC-GN-02 | Spec 与 proposal 自洽 | 整体规范 | ✅ | proposal 基线 5 条 AC → spec 细化为 9 条 AC：方法/属性 mock 拆分为独立 AC（AC-1/AC-6），清理后行为拆分（AC-2/AC-7），新增 AC-8（mockFunc 未调 when()→false，对应 proposal Q-2/K-2），新增 AC-9（空/null name→false，属边界细化）。AC-8/AC-9 均未与 proposal 矛盾，属 spec 层面合法细化；proposal 成功标准（isMocked 返回 true/false，向后兼容）与 spec AC 和兼容性声明对应 |
| SC-GN-05 | 领域-内容错配检测 | 整体规范 | ✅ | 自动探测信号弱（arkruntime=1，arkts 命中为语言名"ArkTS-Dynamic/Static"误报）；manifest profile: none，proposal 确认"候选 Profile: none（不匹配 arkweb/arkui/arkgraphic/arkdata）"；spec 为测试框架内部增强，无特定领域归属，无错配可判 |
| SC-GN-06 | 关联引用完整性 | 整体规范 | ✅ | 机械 needs_review 覆盖为 pass：规则表 R-1~R-9 的"关联AC"列引用 AC-1~AC-9（均存在）；API 变更表"关联 AC"为 AC-1~AC-9（均存在）；行为场景表"关联 AC"列引用 AC-1~AC-9（均存在）；验收追溯表 AC-1~AC-9 对应 R-1~R-9（均存在）；无悬空引用 |

### 🟡 Warning · 建议修改

| 规则编号 | 规则名称 | 维度 | 结果 | 证据 / 缺失说明 |
|----------|----------|------|------|----------------|
| SC-OV-04 | Delta 范围正确性 | 概述与完整性 | ✅ | Delta 表 ADDED 行内容为"MockKit.isMocked(instance, name) 实例方法"（API 名+参数名，非含类型的完整方法签名）+ 功能描述"新增 Mock 状态查询能力"；MODIFIED/REMOVED 标注为"无" |
| SC-AC-02 | WHEN/THEN 格式规范 | 用户故事与验收标准 | ✅ | 所有 AC 使用 WHEN/THEN 格式；THEN 描述可观测结果（返回 true/false），非模糊表述；Given 非必需（WHEN 已含前置条件） |
| SC-AC-05 | 跨平台规格覆盖 | 用户故事与验收标准 | ⚠️ | spec 和 proposal 均未提及跨平台需求（iOS/Android），"双树对等"为 OpenHarmony 内部 ArkTS-Dynamic/Static 双树实现，非跨平台，不适用 |
| SC-AC-09 | 命令场景构造完整性 | 用户故事与验收标准 | ⚠️ | 非 CLI spec，不适用 |
| SC-BR-02 | 黑盒表现描述完整性 | 业务规则 | ✅ | 每条规则（R-1~R-9）有关联 AC（AC-1~AC-9）和验证映射（VM-1~VM-9） |
| SC-BR-03 | 内部实现规格标记 | 业务规则 | ✅ | 内部实现（双树对等/记录表只读查询/方法 mock 需 when() 才算活跃/静态树 mockFunc 记录时序差异）集中在"架构约束"章节，与黑盒规格分离 |
| SC-BR-04 | 专业术语解释 | 业务规则 | ✅ | 术语为通用测试概念（Mock/clear/afterEach/verify/when），目标读者（OpenHarmony 应用开发者）可理解 |
| SC-EX-03 | 性能指标不作输入参数 | 异常与边界规则 | ✅ | 无性能指标出现在 WHEN 条件中 |
| SC-EX-04 | 字符串参数长度限制 | 异常与边界规则 | ✅ | name 参数约束已细化：为空字符串或 null 时返回 false，不抛异常（AC-9 覆盖）。**建议**：可补充最大长度限制以指导测试设计加超长测试点（不影响通过） |
| SC-EX-05 | 异常值/无效值覆盖完整性 | 异常与边界规则 | ✅ | 边界场景已覆盖：系统方法（AC-3）、未 mock 对象（AC-4）、多次 mock（AC-5）、mockFunc 未调 when()（AC-8）、空/null name（AC-9）。非字符串类型属类型违反（name 声明为 String），不在 API 契约范围 |
| SC-EC-03 | 错误码触发因素明确与唯一性 | 错误码定义 | ✅ | 机械 fail 覆盖为 pass：API 声明"错误码: 无"/"错误码范围: 无"，不存在错误码表行，机械检查将 API 变更表行误判为错误码表行；无错误码即无触发→码唯一性问题 |
| SC-API-04 | 接口定义规格完整性与跨章节一致性 | 接口变更分析 | ✅ | 子项 a：接口规格表含函数签名/返回值（含 mockFunc+when() 及空/null name 语义说明）/开放范围/错误码/关联 AC + 参数约束表（类型/必填/默认/约束，name 约束已含空/null 行为）；子项 b：AC 行为与行为场景表（L142-152）9 个场景一一对应，无矛盾 |
| SC-API-05 | 内部接口实现细节归位 design | 接口变更分析 | ✅ | spec 中无 InnerAPI，本规则不适用 |
| SC-API-06 | 调用方明确性 | 接口变更分析 | ✅ | 用户故事明确调用方为"应用开发者"（使用 Hypium 框架编写测试用例的开发者） |
| SC-API-08 | 预置环境明确性 | 接口变更分析 | ✅ | 非 CLI 工具类需求（测试框架 API），不适用 |
| SC-VT-03 | 外部手段说明 | 验证与测试设计 | ✅ | XTS 测试为外部黑盒手段（编写 XTS 用例 → 构建 HAP → aa test 运行），非仅内部单测 |
| SC-VT-04 | 测试入口覆盖 | 验证与测试设计 | ✅ | 验收追溯表每条 AC 有对应验证方式（XTS 单元测试）；验证映射表 VM-1~VM-9 含验证重点。注：证据列"[待生成]"和关联 Task "TBD"属实现阶段产出 |
| SC-VT-05 | 安全/防护要求可验证性 | 验证与测试设计 | ✅ | 非功能需求"安全: N/A"，无安全/防护要求需验证 |
| SC-GN-03 | 结论与证据一致性及跨章节自洽 | 整体规范 | ✅ | 子项 a：context-references 引用的代码结构（记录表/mockFunc+when() 记录时机差异）与 spec 架构约束一致；子项 b：接口行为在 AC 章节与行为场景表（9 个场景）一致；子项 c：无多异常表，无跨表冲突 |
| SC-GN-04 | 代码事实确认 | 整体规范 | ✅ | spec 引用的代码事实（双树对等 src/+src_static/、记录表 recordMockedMethod/propertyValueMap/hashMap、mockFunc+when() 记录时序差异、mockFunc 替换为包装函数透传原方法）与 proposal K-1/K-2 知识源检索日志一致；架构约束"方法 mock 需 when() 才算活跃"与 K-1（动态树 mockFunc 立即 set recordMockedMethod）+K-2（静态树 when() 才创建 hashMap 条目）对应 |

---

## 三、领域特有规则检查结果（扩展）

未加载领域规则。spec 为 testfwk_arkxtest 测试框架内部增强（jsunit Mock 模块），manifest profile: none，proposal 确认候选 Profile 为 none（不匹配 arkweb/arkui/arkgraphic/arkdata 等领域）。SC-GN-05 自动探测信号弱（arkruntime=1，"arkts"为语言名误报），无领域错配可判。

---

## 四、Top 3 最严重问题

本 spec 检查结果为 **0 项不通过**（0 Blocker + 0 Warning），无最严重问题需修复。

以下为 **改进建议**（非阻断，不影响进入测试设计）：

1. **SC-GN-02（设计细化提示）**：spec AC-8 将 proposal Q-2 的静态树行为（mockFunc 未调 when()→false）统一到双树，架构约束声明"与动态树行为一致"。动态树 K-1 表明 mockFunc 立即 set recordMockedMethod，若 isMocked 仅查记录表存在性会返回 true——实现时须确保 isMocked 检查的是"活跃 mock 状态"（when() 已调用）而非仅记录条目存在性。建议在 design.md 中明确动态树 isMocked 的查询逻辑。
2. **SC-EX-04（建议）**：`name` 参数已定义空字符串/null 边界行为（AC-9），仍建议补充最大长度限制以指导测试设计加超长测试点。
3. **SC-VT-04（建议）**：验收追溯表"关联 Task"列为 TBD、"证据"列为 [待生成]——属实现阶段产出，建议在 execution-plan 落地后回填。

---

*报告生成完毕。如需重新检查或调整规则，请修改对应 spec 文件后再次执行 ohos-test-spec-rule-checking。*
