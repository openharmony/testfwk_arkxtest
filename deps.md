# arkxtest 对外依赖清单

## 基础系统服务

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `samgr` | 系统能力管理器 | 系统服务注册与发现 |
| `safwk` | 系统能力框架 | 基础系统服务框架 |
| `init` | 初始化服务 | 进程和系统初始化 |

## IPC通信

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `ipc` | 进程间通信 | 客户端-服务端通信（uitest/perftest） |
| `eventhandler` | 事件处理 | 异步事件分发和处理 |

## 窗口与UI相关

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `window_manager` | 窗口管理器 | 窗口操作、获取窗口属性 |
| `accessibility` | 无障碍服务 | UI组件树访问（uitest核心） |
| `input` | 输入系统 | 注入触摸、按键、鼠标事件 |
| `graphic_2d` | 2D图形库 | 图形绘制和处理 |

## 能力与生命周期

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `ability_base` | Ability基础类 | Ability生命周期管理 |
| `ability_runtime` | Ability运行时 | Ability启动与管理 |
| `access_token` | 访问令牌 | 权限验证和安全控制 |

## 图像与媒体

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `image_framework` | 图像框架 | 屏幕截图、图像处理 |
| `libjpeg-turbo` | JPEG编解码 | JPEG格式图片处理 |
| `libpng` | PNG编解码 | PNG格式图片处理 |

## 性能与数据采集

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `soc_perf` | SoC性能 | 性能数据采集（perftest） |
| `hiview` | 系统视图 | 系统事件和日志 |
| `hisysevent` | 系统事件 | 系统事件监听和上报 |
| `hidumper` | 系统转储器 | 系统状态信息导出 |

## 数据处理与存储

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `json` | JSON解析 | 配置和数据序列化 |
| `data_share` | 数据共享 | 跨进程数据共享 |
| `udmf` | 统一数据格式 | 统一数据管理框架 |
| `pasteboard` | 剪贴板 | 文本输入（inputText中文支持） |

## 运行时与语言支持

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `napi` | Node.js API | JavaScript/ArkTS原生绑定 |
| `runtime_core` | 运行时核心 | ArkTS运行时环境 |
| `libuv` | 异步I/O库 | 异步操作支持 |

## 日志与公共事件

| 依赖组件 | 功能说明 | 用途 |
|---------|---------|------|
| `hilog` | 系统日志 | 日志输出和调试 |
| `common_event_service` | 公共事件服务 | 系统事件订阅和发布 |
| `c_utils` | C工具库 | 通用工具函数 |

## 依赖使用统计

### 按框架分类

**uitest 主要依赖：**
- `accessibility` - UI组件树访问
- `window_manager` - 窗口管理
- `input` - 输入注入
- `image_framework` - 截图
- `ipc` - 客户端-服务端通信

**perftest 主要依赖：**
- `soc_perf` - 性能数据采集
- `ability_runtime` - 应用启动时间测试
- `hiview` / `hisysevent` - 系统事件监听

**jsunit (Hypium) 主要依赖：**
- `napi` - JavaScript绑定
- `runtime_core` - ArkTS运行时
- `hilog` - 日志输出

**testserver 主要依赖：**
- `ipc` - IPC通信中心
- `samgr` - 服务注册
- `eventhandler` - 事件分发

## 资源占用

- **ROM**: 500KB
- **RAM**: 100KB

## 系统能力依赖

arkxtest 需要以下系统能力支持：
- `SystemCapability.Test.UiTest` - UI测试能力
- `SystemCapability.Test.PerfTest` - 性能测试能力

## 适配系统类型

- **standard** - 标准系统类型

## 完整依赖列表（按字母序）

1. `ability_base`
2. `ability_runtime`
3. `access_token`
4. `accessibility`
5. `common_event_service`
6. `c_utils`
7. `data_share`
8. `eventhandler`
9. `graphic_2d`
10. `hidumper`
11. `hilog`
12. `hisysevent`
13. `hiview`
14. `image_framework`
15. `init`
16. `input`
17. `ipc`
18. `json`
19. `libjpeg-turbo`
20. `libpng`
21. `libuv`
22. `napi`
23. `pasteboard`
24. `runtime_core`
25. `safwk`
26. `samgr`
27. `soc_perf`
28. `udmf`
29. `window_manager`

**总计：29个外部依赖组件**

## 来源

以上依赖信息来自 `bundle.json` 文件的 `component.deps.components` 字段。
