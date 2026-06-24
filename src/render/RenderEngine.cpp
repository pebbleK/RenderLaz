#include "RenderEngine.h"

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <rhi/qshader.h>

#include <QFile>
#include <QThread>
#include <QVector2D>
#include <QColor>

namespace{

    struct BlurUniforms{
        QVector2D texelSize;
        QVector2D direction;
        float radius = 0.0f;
        float padding[3] = {};
    };

    void setError(QString *errorMessage, const QString &message){
        if(errorMessage){
            *errorMessage = message;
        }
    }

}

RenderEngine::RenderEngine() = default;

RenderEngine::~RenderEngine(){
    releaseResources();
}

bool RenderEngine::initialize(QString *errorMessage){
    if(m_initialized){
        return true;
    }

    m_thread = QThread::currentThread();

#if defined(Q_OS_MACOS)
    QRhiMetalInitParams params;
    m_rhi.reset(QRhi::create(QRhi::Metal, &params));
#else
    QRhiNullInitParams params;
    m_rhi.reset(QRhi::create(QRhi::Null, &params));
#endif

    if(!m_rhi){
        setError(errorMessage, "无法创建 QRhi");
        return false;
    }

    if(!loadShaders(errorMessage)){
        return false;
    }

    static const float vertices[]{
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };

    m_vertexBuffer = m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices));
    if(!m_vertexBuffer->create()){
        setError(errorMessage, "无法创建 RHI vertex buffer");
        return false;
    }

    m_sampler = m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    if(!m_sampler->create()){
        setError(errorMessage, "无法创建 RHI sampler");
        return false;
    }

    QRhiCommandBuffer *cb = nullptr;
    if(m_rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess){
        setError(errorMessage, "RHI beginOffscreenFrame 失败");
        return false;
    }

    QRhiResourceUpdateBatch *updates = m_rhi->nextResourceUpdateBatch();
    updates->uploadStaticBuffer(m_vertexBuffer, vertices);

    cb->resourceUpdate(updates);
    m_rhi->endOffscreenFrame();

    m_initialized = true;
    return true;
}

bool RenderEngine::loadShaders(QString *errorMessage){
    auto loadShader = [errorMessage](const QString &path) -> QShader *{
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)){
            setError(errorMessage, 
                "无法读取 shader：" + path + "，原因：" + file.errorString());
            return nullptr;
        }

        QShader shader = QShader::fromSerialized(file.readAll());
        if(!shader.isValid()){
            setError(errorMessage, "shader 无效：" + path);
            return nullptr;
        }

        return new QShader(shader);
    };

    m_vertexShader = loadShader(":/shaders/fullscreen.vert.qsb");
    if(!m_vertexShader){
        return false;
    }

    m_blurShader = loadShader(":/shaders/blur.frag.qsb");
    if(!m_blurShader){
        return false;
    }

    return true;
}

QRhiGraphicsPipeline *RenderEngine::createBlurPipeline(
    QRhiRenderPassDescriptor *renderPassDescriptor,
    QRhiShaderResourceBindings *shaderResourceBindings,
    QString *errorMessage){

    auto pipeline = m_rhi->newGraphicsPipeline();

    pipeline->setShaderStages({
        {QRhiShaderStage::Vertex, *m_vertexShader},
        {QRhiShaderStage::Fragment, *m_blurShader}
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        {QRhiVertexInputBinding(4 * sizeof(float))}
    });

    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, 
            QRhiVertexInputAttribute::Float2, 0),

        QRhiVertexInputAttribute(0, 1, 
            QRhiVertexInputAttribute::Float2, 2 * sizeof(float))
    });

    pipeline->setVertexInputLayout(inputLayout); // VAO
    pipeline->setShaderResourceBindings(shaderResourceBindings); // shader uniform bindings
    pipeline->setRenderPassDescriptor(renderPassDescriptor); // RGBA format
    pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    pipeline->setCullMode(QRhiGraphicsPipeline::None);

    if(!pipeline->create()){
        delete pipeline;
        setError(errorMessage, "无法创建 RHI blur pipeline");
        return nullptr;
    }

    return pipeline;
}

bool RenderEngine::renderPass(QRhiCommandBuffer *commandBuffer,
    QRhiTextureRenderTarget *target,
    QRhiGraphicsPipeline *pipeline,
    QRhiShaderResourceBindings *shaderResourceBindings,
    const QSize &size,
    QString *errorMessage){

    if(!commandBuffer || !target || !pipeline || !shaderResourceBindings){
        setError(errorMessage, "RHI render pass 的参数无效");
        return false;
    }

    commandBuffer->beginPass(target, QColor(0, 0, 0, 0), QRhiDepthStencilClearValue(1.0f, 0));
    commandBuffer->setGraphicsPipeline(pipeline);
    commandBuffer->setViewport(QRhiViewport(
        0.0f, 
        0.0f, 
        static_cast<float>(size.width()), 
        static_cast<float>(size.height())
    ));

    const QRhiCommandBuffer::VertexInput vertexBindings(m_vertexBuffer, 0);
    commandBuffer->setVertexInput(0, 1, &vertexBindings);
    commandBuffer->setShaderResources(shaderResourceBindings);
    commandBuffer->draw(6);

    commandBuffer->endPass();

    return true;
}

/*
    QRhiTextureRenderTarget -> 画布
    QRhiRenderPassDescriptor -> 画布参数
    QRhiShaderResourceBindings -> uniformBuffer
    QRhiGraphicsPipeline -> 画法
    QRhiCommandBuffer -> rendering order
*/
QImage RenderEngine::renderBlur(const QImage &image,
    float radius, QString *errorMessage){

    if(image.isNull()){
        setError(errorMessage, "输入图片为空");
        return QImage();
    }

    if(!initialize(errorMessage)){
        return QImage();
    }

    const QImage sourceImage = image.convertToFormat(QImage::Format_RGBA8888);
    const QSize size = sourceImage.size();

    // 分为横向blur和纵像blur两个PASS，因此创建了两个Pipeline

    QRhiTexture *sourceTexture = nullptr;
    QRhiTexture *horizontalTexture = nullptr;
    QRhiTexture *resultTexture = nullptr;

    QRhiTextureRenderTarget *horizontalTarget = nullptr;
    QRhiTextureRenderTarget *resultTarget = nullptr;

    QRhiRenderPassDescriptor *horizontalPassDescriptor = nullptr;
    QRhiRenderPassDescriptor *resultPassDescriptor = nullptr;

    QRhiBuffer *horizontalUniformBuffer = nullptr;
    QRhiBuffer *verticalUniformBuffer = nullptr;

    QRhiShaderResourceBindings *horizontalBindings = nullptr;
    QRhiShaderResourceBindings *verticalBindings = nullptr;

    QRhiGraphicsPipeline *horizontalPipeline = nullptr;
    QRhiGraphicsPipeline *verticalPipeline = nullptr;

    auto cleanup = [&](){
        delete verticalPipeline;
        delete horizontalPipeline;

        delete verticalBindings;
        delete horizontalBindings;

        delete verticalUniformBuffer;
        delete horizontalUniformBuffer;

        delete resultPassDescriptor;
        delete horizontalPassDescriptor;

        delete resultTarget;
        delete horizontalTarget;

        delete resultTexture;
        delete horizontalTexture;
        delete sourceTexture;
    };

    /* 
    frags: Null - 纹理用于读取
    RenderTarget - 纹理为可作为渲染目标写入
    UsedAsTransferSource - 纹理会读回CPU
    */
    sourceTexture = m_rhi->newTexture(
        QRhiTexture::RGBA8,
        size,
        1);

    horizontalTexture = m_rhi->newTexture(
        QRhiTexture::RGBA8,
        size,
        1,
        QRhiTexture::RenderTarget);

    resultTexture = m_rhi->newTexture(
        QRhiTexture::RGBA8,
        size,
        1,
        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);

    if(!sourceTexture->create() || !horizontalTexture->create() || !resultTexture->create()){
        cleanup();
        setError(errorMessage, "无法创建 RHI texture");
        return QImage();
    }

    horizontalTarget = m_rhi->newTextureRenderTarget(
        QRhiTextureRenderTargetDescription(
        QRhiColorAttachment(horizontalTexture)));

    horizontalPassDescriptor = horizontalTarget->newCompatibleRenderPassDescriptor();
    horizontalTarget->setRenderPassDescriptor(horizontalPassDescriptor);
    if(!horizontalTarget->create()){
        cleanup();
        setError(errorMessage, "无法创建横向 blur render target");
        return QImage();
    }

    resultTarget = m_rhi->newTextureRenderTarget(
        QRhiTextureRenderTargetDescription(
            QRhiColorAttachment(resultTexture)));

    resultPassDescriptor = resultTarget->newCompatibleRenderPassDescriptor();
    resultTarget->setRenderPassDescriptor(resultPassDescriptor);
    if(!resultTarget->create()){
        cleanup();
        setError(errorMessage, "无法创建纵向 blur render target");
        return QImage();
    }

    const int uniformBufferSize = m_rhi->ubufAligned(sizeof(BlurUniforms));
    horizontalUniformBuffer = m_rhi->newBuffer(
        QRhiBuffer::Static,
        QRhiBuffer::UniformBuffer,
        uniformBufferSize);

    verticalUniformBuffer = m_rhi->newBuffer(
        QRhiBuffer::Static,
        QRhiBuffer::UniformBuffer,
        uniformBufferSize);

    if(!horizontalUniformBuffer->create() || !verticalUniformBuffer->create()){
        cleanup();
        setError(errorMessage, "无法创建 blur uniform buffer");
        return QImage();
    }

    horizontalBindings = m_rhi->newShaderResourceBindings();
    horizontalBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::FragmentStage,
            horizontalUniformBuffer,
            0,
            sizeof(BlurUniforms)),

        QRhiShaderResourceBinding::sampledTexture(
            1,
            QRhiShaderResourceBinding::FragmentStage,
            sourceTexture,
            m_sampler)
    });

    if(!horizontalBindings->create()){
        cleanup();
        setError(errorMessage, "无法创建横向 blur shader bindings");
        return QImage();
    }

    verticalBindings = m_rhi->newShaderResourceBindings();
    verticalBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::FragmentStage,
            verticalUniformBuffer,
            0,
            sizeof(BlurUniforms)),

        QRhiShaderResourceBinding::sampledTexture(
            1,
            QRhiShaderResourceBinding::FragmentStage,
            horizontalTexture,
            m_sampler)
    });

    if(!verticalBindings->create()){
        cleanup();
        setError(errorMessage, "无法创建纵向 blur shader bindings");
        return QImage();
    }

    horizontalPipeline = createBlurPipeline(
        horizontalPassDescriptor,
        horizontalBindings,
        errorMessage);

    if(!horizontalPipeline){
        cleanup();
        return QImage();
    }

    verticalPipeline = createBlurPipeline(
        resultPassDescriptor,
        verticalBindings,
        errorMessage);

    if(!verticalPipeline){
        cleanup();
        return QImage();
    }

    // 设置uniform struct数据
    BlurUniforms horizontalUniforms;
    horizontalUniforms.texelSize = QVector2D(
        1.0f / static_cast<float>(size.width()),
        1.0f / static_cast<float>(size.height()));
    horizontalUniforms.direction = QVector2D(1.0f, 0.0f);
    horizontalUniforms.radius = radius;

    BlurUniforms verticalUniforms;
    verticalUniforms.texelSize = horizontalUniforms.texelSize;
    verticalUniforms.direction = QVector2D(0.0f, 1.0f);
    verticalUniforms.radius = radius;

    QRhiCommandBuffer *commandBuffer = nullptr;
    if(m_rhi->beginOffscreenFrame(&commandBuffer) != QRhi::FrameOpSuccess){
        cleanup();
        setError(errorMessage, "RHI beginOffscreenFrame 失败");
        return QImage();
    }

    // cpu端上传数据
    QRhiResourceUpdateBatch *updates = m_rhi->nextResourceUpdateBatch();
    updates->uploadTexture(sourceTexture, sourceImage);
    updates->uploadStaticBuffer(
        horizontalUniformBuffer,
        0,
        sizeof(BlurUniforms),
        &horizontalUniforms);
    updates->uploadStaticBuffer(
        verticalUniformBuffer,
        0,
        sizeof(BlurUniforms),
        &verticalUniforms);

    commandBuffer->resourceUpdate(updates);

    // PASS 1
    if(!renderPass(
        commandBuffer,
        horizontalTarget,
        horizontalPipeline,
        horizontalBindings,
        size,
        errorMessage)){
        m_rhi->endOffscreenFrame();
        cleanup();
        return QImage();
    }

    // PASS 2
    if(!renderPass(
        commandBuffer,
        resultTarget,
        verticalPipeline,
        verticalBindings,
        size,
        errorMessage)){
        m_rhi->endOffscreenFrame();
        cleanup();
        return QImage();
    }

    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch *readbackUpdates = m_rhi->nextResourceUpdateBatch();
    readbackUpdates->readBackTexture(
        QRhiReadbackDescription(resultTexture),
        &readback);

    commandBuffer->resourceUpdate(readbackUpdates);

    m_rhi->endOffscreenFrame();
    m_rhi->finish();

    const QSize resultSize = readback.pixelSize.isValid()
        ? readback.pixelSize
        : size;

    if(readback.data.isEmpty()){
        cleanup();
        setError(errorMessage, "RHI readback 结果为空");
        return QImage();
    }

    QImage result(
        reinterpret_cast<const uchar *>(readback.data.constData()),
        resultSize.width(),
        resultSize.height(),
        QImage::Format_RGBA8888);

    QImage copiedResult = result.copy().convertToFormat(QImage::Format_ARGB32);
    //上下颠倒
    // QImage copiedResult = result.copy().convertToFormat(QImage::Format_ARGB32);

    cleanup();

    return copiedResult;
}

void RenderEngine::releaseResources(){
    delete m_sampler;
    m_sampler = nullptr;

    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;

    delete m_vertexShader;
    m_vertexShader = nullptr;

    delete m_blurShader;
    m_blurShader = nullptr;

    m_rhi.reset();

    m_initialized = false;
    m_thread = nullptr;
}

