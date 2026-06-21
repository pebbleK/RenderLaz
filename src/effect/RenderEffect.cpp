#include "RenderEffect.h"
#include "../render/RenderEngine.h"

QImage BlurEffect::apply(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress
    ) const{
    if(image.isNull()){
        return QImage();
    }

    if(isCanceled(shouldCancel)){
        return QImage();
    }

    if(onProgress){
        onProgress(0);
    }

    RenderEngine renderer;
    QString errorMessage;
    QImage result = renderer.renderBlur(image, 4.0f, &errorMessage);

    if(onProgress){
        onProgress(100);
    }

    return result;
}