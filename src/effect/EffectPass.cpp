#include "EffectPass.h"

#include <QColor>
#include <algorithm>

EffectPass::EffectPass(EffectType type)
: m_type(type)
{}

QString EffectPass::name() const{
    switch(m_type){
    case EffectType::Null:
        return "Null";
    case EffectType::Grayscale:
        return "Grayscale";
    case EffectType::Invert:
        return "Invert";
    case EffectType::Sepia:
        return "Sepia";
    }

    return "Unknown";
}

QString EffectPass::effectTypeSuffix() const{
    switch(m_type){
    case EffectType::Null:
        return "Null";
    case EffectType::Grayscale:
        return "grayscale";
    case EffectType::Invert:
        return "invert";
    case EffectType::Sepia:
        return "sepia";
    }

    return "effect";
}

QImage EffectPass::apply(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress
    ) const{
        if(image.isNull()){
            return QImage();
        }

        switch(m_type){
        case EffectType::Null:
            return applyNull(image, shouldCancel, onProgress);
        case EffectType::Grayscale:
            return applyGrayscale(image, shouldCancel, onProgress);
        case EffectType::Invert:
            return applyInvert(image, shouldCancel, onProgress);
        case EffectType::Sepia:
            return applySepia(image, shouldCancel, onProgress);
        }

        return image;
}

QImage EffectPass::applyNull(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress) const{
    if(isCanceled(shouldCancel)){
        return QImage();
    }

    if(onProgress){
        onProgress(100);
    }

    return image;
    }

QImage EffectPass::applyGrayscale(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress) const{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);

    if(onProgress){
        onProgress(0);
    }

    for(int y = 0; y < result.height(); ++y){
        if(isCanceled(shouldCancel)){
            return QImage();
        }

        // rbg按权重转换为灰度值
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for(int x = 0; x < result.width(); ++x){
            const QColor color(line[x]);
            const int gray = qGray(color.rgb());
            line[x] = qRgba(gray, gray, gray, color.alpha());
        }

        // 逐行的进度
        reportProgress(onProgress, y, result.height());
    }

    if(onProgress){
        onProgress(100);
    }

    return result;
}

QImage EffectPass::applyInvert(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress) const{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);

    if(onProgress){
        onProgress(0);
    }

    for(int y = 0; y < result.height(); ++y){
        if(isCanceled(shouldCancel)){
            return QImage();
        }

        // rbg反转色域
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for(int x = 0; x < result.width(); ++x){
            const QColor color(line[x]);
            line[x] = qRgba(255 - color.red(), 255 - color.green(), 
            255 - color.blue(), color.alpha());
        }

        // 逐行的进度
        reportProgress(onProgress, y, result.height());
    }

    if(onProgress){
        onProgress(100);
    }

    return result;
}

QImage EffectPass::applySepia(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress) const{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);

    if(onProgress){
        onProgress(0);
    }

    for(int y = 0; y < result.height(); ++y){
        if(isCanceled(shouldCancel)){
            return QImage();
        }

        // rgb转换棕褐色
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for(int x = 0; x < result.width(); ++x){
            const QColor color(line[x]);

            const int red = color.red();
            const int green = color.green();
            const int blue = color.blue();

            const int newRed = std::clamp(
                static_cast<int>(0.393 * red + 0.769 * green + 0.189 * blue),
                0, 255);

            const int newGreen = std::clamp(
                static_cast<int>(0.349 * red + 0.686 * green + 0.168 * blue),
                0,
                255
            );

            const int newBlue = std::clamp(
                static_cast<int>(0.272 * red + 0.534 * green + 0.131 * blue),
                0,
                255
            );

            line[x] = qRgba(newRed, newGreen, newBlue, color.alpha());
        }

        // 逐行进度
        reportProgress(onProgress, y, result.height());
    }

    if(onProgress){
        onProgress(100);
    }

    return result;
}

bool EffectPass::isCanceled(const std::function<bool()> &shouldCancel){
    return shouldCancel && shouldCancel(); // 检查绑定 || 检查返回值
}
    
void EffectPass::reportProgress(
    const std::function<void(int)> &onProgress,
    int row,
    int totalRows){

    if(!onProgress || totalRows <= 0){
        return;
    }

    // 每20次任务报告一次，最后一次运行报告
    if(row % 20 == 0 || row == totalRows - 1){
        const int progress = (row + 1) * 100 / totalRows;
        onProgress(progress);
    }
}