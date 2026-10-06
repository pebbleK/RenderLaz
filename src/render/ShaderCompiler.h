/**
 * @file ShaderCompiler.h
 * @brief GLSL 源码的运行时编译
 * @details 把编辑器里的 GLSL 文本编译成 QShader，省去重新执行 qsb 构建步骤。
 *          一次 bake 会同时生成各后端所需的代码（SPIR-V、GLSL、MSL、HLSL），
 *          因此同一份源码在 macOS 的 Metal 和 Windows 的 OpenGLES2 上都能用。
 *          编译失败时把 glslang 的报错文本通过 errorMessage 返回给调用方。
 */
#pragma once

#include <QByteArray>
#include <QString>

#include <rhi/qshader.h>

class ShaderCompiler{
public:
    // 编译失败返回无效 QShader
    static QShader compile(
        const QByteArray &source,
        QShader::Stage stage,
        QString *errorMessage = nullptr
    );
};
