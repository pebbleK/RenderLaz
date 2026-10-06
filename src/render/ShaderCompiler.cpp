#include "ShaderCompiler.h"

#include <QtShaderTools/rhi/qshaderbaker.h>

namespace{

    /*
    需要生成的目标后端集合。
    SPIR-V 是中间表示，GLSL 覆盖 OpenGL / OpenGL ES，
    MSL 对应 macOS 的 Metal，HLSL 留给 Direct3D 后端。
    */
    QList<QShaderBaker::GeneratedShader> targetShaders(){
        return {
            {QShader::SpirvShader, QShaderVersion(100)},
            {QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs)},
            {QShader::GlslShader, QShaderVersion(120)},
            {QShader::GlslShader, QShaderVersion(330)},
            {QShader::HlslShader, QShaderVersion(50)},
            {QShader::MslShader, QShaderVersion(20)}
        };
    }

}

QShader ShaderCompiler::compile(
    const QByteArray &source,
    QShader::Stage stage,
    QString *errorMessage){

    if(source.trimmed().isEmpty()){
        if(errorMessage){
            *errorMessage = "着色器源码为空";
        }
        return QShader();
    }

    QShaderBaker baker;
    baker.setSourceString(source, stage, QStringLiteral("editor.glsl"));

    /*
    这一行必须有。不设置 shader variant 的话，bake() 内部遍历变体的循环
    一次都不执行，返回一个空 shader，而且 errorMessage() 同样是空的，
    排查时会误以为是最底层编译失败。
    */
    baker.setGeneratedShaderVariants({QShader::StandardShader});
    baker.setGeneratedShaders(targetShaders());

    const QShader shader = baker.bake();

    if(!shader.isValid()){
        if(errorMessage){
            const QString detail = baker.errorMessage();
            *errorMessage = detail.isEmpty()
                ? QStringLiteral("着色器编译失败，编译器未返回具体原因")
                : detail;
        }
        return QShader();
    }

    return shader;
}
