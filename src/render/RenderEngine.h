#pragma once

#include <QImage>
#include <QString>
#include <memory>

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
