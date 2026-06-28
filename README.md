# RenderLaz

RenderLaz 是一个基于 Qt/C++ 的 Shader 图像特效编辑器。项目用于导入图片、组织特效链，并通过 GPU 渲染生成预览或导出结果。

## 已实现功能

- 图片资源导入与预览。
- 基础特效链管理。
- 普通 CPU 图像特效，包括灰度、反色、棕褐色等。
- 基于 Shader 的 Blur 特效。
- 支持 windows 和 macOS 双平台 GPU 特效渲染。
- 批量处理任务框架。
- 工程配置保存与加载。
- 应用日志输出。

## 支持平台

- Windows: 已支持 OpenGL 渲染后端。
- macOS: 已支持 Metal 渲染后端。

## 技术栈

- C++17
- Qt 6 Widgets
- Qt RHI
- Qt ShaderTools
- CMake

## 本机 CMake 配置

仓库里的 `CMakePresets.json` 只保存跨平台共享配置，不保存 Qt、编译器、Ninja/MinGW 等本机绝对路径。

首次拉取后，复制 `CMakeUserPresets.json.example` 为 `CMakeUserPresets.json`，再按本机安装路径修改其中的 `CMAKE_PREFIX_PATH`、编译器路径或生成器。

## 构建并运行

各平台运行脚本：脚本会按当前系统自动选择对应 preset。

```powershell
.\build-run.ps1          # Windows
```

```bash
./build-run.sh           # macOS / Linux
```

或显式指定 preset：

```powershell
.\build-run.ps1 windows-local
```

```bash
./build-run.sh macos-local
```

## 当前状态

项目目前处于早期开发阶段，核心界面、资源管理、特效链、批处理和基础渲染流程已经接入。后续重点是完善实时预览、扩展更多 Shader 特效，并继续优化跨平台渲染后端。
