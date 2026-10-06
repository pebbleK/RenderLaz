cmake_minimum_required(VERSION 3.20)

project(RenderLaz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets ShaderTools ShaderToolsPrivate GuiPrivate)

qt_standard_project_setup()

qt_add_executable(RenderLaz
    src/main.cpp

    src/app/MainWindow.h
    src/app/MainWindow.cpp

    src/resource/ResourceManager.h
    src/resource/ResourceManager.cpp

    src/logger/Logger.h
    src/logger/Logger.cpp

    src/task/TaskManager.h
    src/task/TaskManager.cpp

    src/effect/EffectPass.h
    src/effect/EffectPass.cpp
    src/effect/EffectChain.h
    src/effect/EffectChain.cpp
    src/effect/NormalEffect.h
    src/effect/NormalEffect.cpp
    src/effect/RenderEffect.h
    src/effect/RenderEffect.cpp

    src/config/ProjectConfig.h
    src/config/ProjectConfig.cpp

    src/render/RenderEngine.h
    src/render/RenderEngine.cpp

    src/render/ShaderCompiler.h
    src/render/ShaderCompiler.cpp

)

target_link_libraries(RenderLaz PRIVATE
    Qt6::Widgets
    Qt6::GuiPrivate
    # ShaderCompiler 用到 QShaderBaker，它声明在 QtShaderTools 的 rhi 子目录下，
    # 只链 Qt6::ShaderTools 拿不到该头文件的搜索路径。
    Qt6::ShaderToolsPrivate
)

qt_add_shaders(RenderLaz "renderlaz_shaders"
    PREFIX "/shaders"
    BASE "src/render/shaders"
    FILES
        src/render/shaders/fullscreen.vert
        src/render/shaders/blur.frag
)

# 默认着色器的源码文本，供编辑器载入作为初始内容，与上面烘焙出的 .qsb 同源
qt_add_resources(RenderLaz "renderlaz_shader_sources"
    PREFIX "/shadersrc"
    BASE "src/render/shaders"
    FILES
        src/render/shaders/fullscreen.vert
        src/render/shaders/blur.frag
)
