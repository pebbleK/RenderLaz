# RenderLaz

基于 Tauri 2 的 Shader 图像特效编辑器：导入图片 → 组织特效链 → WebGL2 渲染 → 预览或批处理导出 PNG。

## 技术栈

- Tauri 2（Rust 核心 + 系统 WebView）
- React 19 + TypeScript 6 + Vite 8
- WebGL2（GPU 模糊）
- SQLite（工程、设置、批处理历史）

目标平台：macOS 14+、Windows 11。

## 目录

```text
src/                       前端
├── app/                   布局与状态（App.tsx、state.ts、rail.ts）
├── ui/                    面板组件（TabBar + 各页签内容）
├── render/                WebGL2 渲染、内置模糊着色器、特效链执行
├── effects/               CPU 像素特效
├── lib/                   IPC 封装、图片工具、日志、ts-rs 生成的类型
src-tauri/                 Rust 后端
├── src/commands/          images / projects / settings / history
├── src/db.rs              SQLite 建表与降级
├── src/models.rs          前后端共享类型（ts-rs 导出）
specs/                     Spec Kit 的需求、计划与任务
```

## 界面结构

左栏分两个区，每区一条页签栏：

- **资源区**：资源 / 批处理
- **编辑区**：特效链 / 日志（批处理历史与实时日志同页）

特效下拉框默认「无」，选中即临时预览；参数就在同一栏里，点链中任意一项即可就地调整它的强度、对比度——模糊另有半径与迭代次数。

右侧是预览区，占满剩余宽度。着色器在线编辑能力已移除，模糊始终使用内置实现。

## 开发

前置：Node 20.19+、Rust 1.85+、macOS 需 Xcode Command Line Tools。

```bash
npm install
npm run tauri dev          # 开发态：Vite + 桌面窗口
```

Rust 不在 PATH 时先执行 `export PATH="$HOME/.cargo/bin:$PATH"`。VS Code 用户可直接运行内置任务：`dev` / `build` / `test` / `typecheck`。

## 构建与测试

```bash
npm run build                                        # 类型检查 + 前端打包
npm run test                                         # 前端单元测试（vitest）
cargo test --manifest-path src-tauri/Cargo.toml       # Rust 测试 + 重新生成前端类型
npm run tauri build                                  # 产出安装包
```

前端类型由 `src-tauri/src/models.rs` 生成到 `src/lib/bindings/`，改结构体后跑一次 `cargo test` 即可同步。

## 数据位置

- macOS：`~/Library/Application Support/com.renderlaz.app/renderlaz.db`
- Windows：`%APPDATA%\com.renderlaz.app\renderlaz.db`

设置与批处理历史都在这个库里。工程保存在 feature 003 中移除，应用不再产生 `.renderlaz` 或 JSON 工程文件。删除该文件即可回到全新状态。

## 验收

逐条验收场景见 [specs/001-tauri2-sqlite-rewrite/quickstart.md](specs/001-tauri2-sqlite-rewrite/quickstart.md)。
