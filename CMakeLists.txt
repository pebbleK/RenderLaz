cmake_minimum_required(VERSION 3.20)

project(RenderLaz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

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
    src/effect/NormalEffect.h
    src/effect/NormalEffect.cpp
    src/effect/EffectChain.h
    src/effect/EffectChain.cpp

)

target_link_libraries(RenderLaz PRIVATE Qt6::Widgets)
