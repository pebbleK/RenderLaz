<div align="center">
  <img src="src-tauri/icons/icon.png" width="96" alt="RenderLaz logo">

  # RenderLaz

  *懒人式体验特效渲染*

  ![Platform](https://img.shields.io/badge/platform-macOS%2014%2B%20%7C%20Windows%2011-4f8cff?style=flat-square)
  ![Tauri](https://img.shields.io/badge/Tauri-2-24c8db?style=flat-square&logo=tauri&logoColor=white)
  ![React](https://img.shields.io/badge/React-19-61dafb?style=flat-square&logo=react&logoColor=white)
  ![WebGL2](https://img.shields.io/badge/WebGL-2-ff6a00?style=flat-square)
  ![SQLite](https://img.shields.io/badge/SQLite-3-003b57?style=flat-square&logo=sqlite&logoColor=white)

  [功能](#功能) • [快速开始](#快速开始) • [构建与测试](#构建与测试) • [架构](#架构) • [数据位置](#数据位置) • [故障排查](#故障排查)
</div>

RenderLaz 是一个基于 Tauri 2 的桌面图像特效编辑器。导入图片、把特效串成一条链、在右侧实时预览，确认后导出 PNG 或整批处理。全程本地运行，不上传任何图片。

## 功能

- **串行特效链** — 前一项的输出就是后一项的输入，增删任意一项后预览立即按剩余顺序重算。
- **选特效即预览** — 下拉框里换一种特效，预览区马上显示「当前链 + 这个特效」的结果，不会污染真实特效链；确认满意再点「添加」。
- **逐项调参** — 点链中任意一项就能就地改它的参数，不需要删了重加。
- **批处理导出** — 按当前链逐张导出 PNG，显示整体进度，可中途取消；单张失败不中断整批。
- **本地持久化** — 输出目录、窗口尺寸、当前会话与批处理历史存在 SQLite，重启后原样恢复。
- **日志面板** — 批处理历史与实时运行日志同页，带时间与级别。

### 特效

| 特效 | 实现 | 可调参数 |
| --- | --- | --- |
| 直通 `null` | CPU | 强度、对比度 |
| 灰度 `grayscale` | CPU（BT.601 权重） | 强度、对比度 |
| 反色 `invert` | CPU | 强度、对比度 |
| 棕褐 `sepia` | CPU（经典矩阵） | 强度、对比度 |
| 模糊 `blur` | WebGL2（可分离两遍高斯） | 强度、对比度、半径、迭代次数 |

每一步算出自身结果后，都会按「强度」与前一步画面混合、再按「对比度」调整，所以任何特效都能当半透明叠加用。

### 界面

左侧是固定宽度的功能栏，分成上下两个区，每区一条页签栏；右侧预览占满剩余宽度。

- **资源区** — `资源`（导入与选择图片）/ `批处理`
- **编辑区** — `特效链`（下拉框 + 参数区 + 链列表）/ `日志`

预览会按最长边 2048 降采样以保证响应速度，导出与批处理始终使用原始分辨率。

## 快速开始

前置依赖：

| 依赖 | 版本要求 |
| --- | --- |
| Node.js | 20.19+ |
| Rust | 1.85+ |
| macOS | 14+，需 Xcode Command Line Tools |
| Windows | 11，需 MSVC Build Tools 与 WebView2 Runtime |

```bash
git clone git@github.com:pebbleK/RenderLaz.git
cd RenderLaz
npm install
npm run tauri dev          # 开发态：Vite + 桌面窗口
```

首次运行会编译 Rust 依赖，需要几分钟。

> [!NOTE]
> `cargo` 不在 PATH 时先执行 `export PATH="$HOME/.cargo/bin:$PATH"`。VS Code 用户可直接运行内置任务 `dev` / `build` / `test` / `typecheck`（见 `.vscode/tasks.json`）。

## 构建与测试

```bash
npm run build                                        # tsc --noEmit + vite build
npm run test                                         # 前端单元测试（vitest，37 项）
cargo test --manifest-path src-tauri/Cargo.toml      # Rust 测试（21 项）并重新生成前端类型
npm run tauri build                                  # 产出安装包
```

> [!IMPORTANT]
> 前端类型由 `src-tauri/src/models.rs` 通过 `ts-rs` 生成到 `src/lib/bindings/`，**不要手改**。改完结构体跑一次 `cargo test` 即可同步。

仓库目前没有 E2E 测试，界面行为需要人工验收，逐条场景见 [quickstart](specs/001-tauri2-sqlite-rewrite/quickstart.md)。

## 架构

```text
src/                        前端（React 19 + TypeScript 6）
├── app/                    App 外壳、reducer 状态、页签常量、临时预览
├── ui/                     左栏各面板 + 预览画布
├── render/                 WebGL2 上下文、内置模糊着色器、特效链执行
├── effects/                CPU 像素特效、参数区间与默认值
└── lib/                    IPC 封装、图片编解码、批处理循环、日志、ts-rs 生成的类型

src-tauri/src/              Rust 后端
├── commands/               images / settings / history / system
├── db.rs                   SQLite 建表、迁移、损坏降级
├── models.rs               前后端共享类型（唯一来源）
└── error.rs                统一 ApiError

specs/                      Spec Kit 的需求、计划与任务
```

几条贯穿全局的约束：

- **渲染只在 webview 内完成。** 预览与批处理都调用 `src/render/pipeline.ts` 的 `runChain`，Rust 侧不重复实现特效——这是两个入口结果一致的原因。
- **IPC 契约先行。** 前端只通过 `src/lib/ipc.ts` 调命令，不在组件里直接 `invoke`；命令一律返回 `Result<T, ApiError>`，失败时前端拿到 `{ code, message, context }` 而不是字符串。
- **写盘只走 `write_png`。** Rust 侧先写同目录临时文件，`fsync` 后 `rename`，批处理取消时才不会留下半个 PNG。
- **特效链串行执行。** `runChain` 按顺序逐项计算；模糊的多次迭代就是同一遍模糊重复执行。

## 数据位置

| 平台 | 路径 |
| --- | --- |
| macOS | `~/Library/Application Support/com.renderlaz.app/renderlaz.db` |
| Windows | `%APPDATA%\com.renderlaz.app\renderlaz.db` |

设置、会话与批处理历史都在这个库里，没有 `.renderlaz` 或工程 JSON 文件。删除该文件即可回到全新状态。

> [!WARNING]
> SQLite 以 WAL 模式运行，复制数据库文件前请先退出应用，否则可能拿到不完整的副本。

## 故障排查

| 现象 | 原因与处理 |
| --- | --- |
| 预览区提示「当前系统不支持 WebGL2」 | webview 未提供 WebGL2 上下文。模糊特效需要 GPU，应用不会静默降级为 CPU 实现。 |
| 顶部出现「数据库已损坏…」提示 | 损坏的库已被改名备份为 `renderlaz.db.corrupt-*`，并新建了空库；这是预期的降级行为。 |
| 顶部提示「数据库不可用…只读模式运行」 | 数据库打不开，应用已改用内存库启动，本次改动不会保存。检查数据目录的权限或占用情况。 |
| 批处理点取消后仍处理了一会儿 | 取消在每张图片处理完后生效，当前这张不会半途落盘。 |
| 预览与导出分辨率不同 | 预览按最长边 2048 降采样，导出始终保持原始尺寸。 |

## 规格与验收

功能演进记录在 `specs/` 下，每份都包含 spec（用户故事与验收标准）、plan 与 tasks：

| 目录 | 内容 |
| --- | --- |
| [`001-tauri2-sqlite-rewrite`](specs/001-tauri2-sqlite-rewrite/spec.md) | 从 Qt/C++ 重写为 Tauri 2 + SQLite + WebGL2 |
| [`002-streamline-ui`](specs/002-streamline-ui/spec.md) | 左栏双区页签布局，移除着色器在线编辑 |
| [`003-effect-preview-drop-projects`](specs/003-effect-preview-drop-projects/spec.md) | 选特效即时预览，移除工程保存 |
| [`004-effect-params`](specs/004-effect-params/spec.md) | `EffectStep` 增加结构化参数，管线加入混合与对比度 |
| [`005-inline-params`](specs/005-inline-params/spec.md) | 参数内联到特效链页签 |

项目遵循 [constitution](.specify/memory/constitution.md) 中的五条原则；非平凡改动的流程是 `/speckit-specify` → `/speckit-plan` → `/speckit-tasks` → `/speckit-implement`，完成后仓库必须保持可构建、可启动。

## 约定

更完整的约定与已知坑见 [AGENTS.md](AGENTS.md)，要点：

- 注释与界面文案统一用中文。
- 特效参数的区间与默认值只在 `src/effects/params.ts` 定义，不在组件里另写一份。
- 左栏页签常量集中在 `src/app/rail.ts`，新增入口时同时改常量、`TabBar` 渲染与 `App.tsx` 分支。
- 特效参数区编辑对象二选一：下拉框草稿（未入链）或链中选中项，两者不会同时生效。
