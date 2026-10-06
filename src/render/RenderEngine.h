/**
 * @file RenderEngine.h
 * @brief 基于 Qt RHI 的离屏渲染引擎
 * @details 负责 RHI 后端的初始化与平台选择（macOS 使用 Metal，Windows 使用
 *          OpenGLES2，其余平台回退到 Null 后端），加载编译好的 shader、维护顶点
 *          缓冲和采样器等常驻 GPU 资源，并在离屏帧中执行渲染后回读为 QImage。
 *          目前对外只提供高斯模糊的渲染接口，渲染管线在调用内部按 pass 构建。
 */
#pragma once

#include <QImage>
#include <QString>
#include <memory>

#include <rhi/qshader.h>

class QRhi;
class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QShader;
class QThread;
class QRhiCommandBuffer;
class QRhiRenderPassDescriptor;
class QRhiTexture;
class QRhiTextureRenderTarget;
class QOffscreenSurface;

class RenderEngine{
public:
    RenderEngine();
    ~RenderEngine();

    QImage renderBlur(const QImage &image,
        float radius, QString *errorMessage = nullptr);

    /*
    用户从编辑器编译出来的片元着色器。设为有效值后，后续所有
    RenderEngine 实例都改用它，批处理线程和界面预览因此共享同一份着色器。
    传入空 QShader 则回到内置的 blur 着色器。
    */
    static void setUserFragmentShader(const QShader &shader);
    static QShader userFragmentShader();

private:
    bool initialize(QString *errorMessage);
    bool loadShaders(QString *errorMessage);
    void releaseResources();

    QRhiGraphicsPipeline *createBlurPipeline(
        QRhiRenderPassDescriptor *renderPassDescriptor,
        QRhiShaderResourceBindings *shaderResourceBindings,
        QString *errorMessage);

    bool renderPass(
        QRhiCommandBuffer *commandBuffer,
        QRhiTextureRenderTarget *target,
        QRhiGraphicsPipeline *pipeline,
        QRhiShaderResourceBindings *shaderResourceBindings,
        const QSize &size,
        QString *errorMessage);

private:
    bool m_initialized = false;
    QThread *m_thread = nullptr;

    // 使用Qt抽象图形层Rendering Hardware Interface
    std::unique_ptr<QRhi> m_rhi;

    QShader *m_vertexShader = nullptr;
    QShader *m_blurShader = nullptr;

    QRhiBuffer *m_vertexBuffer = nullptr;
    QRhiSampler * m_sampler = nullptr;

    QOffscreenSurface *m_fallbackSurface = nullptr;
};
