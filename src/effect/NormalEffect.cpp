#include "NormalEffect.h"

QImage NullEffect::apply(
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
        onProgress(100);
    }

    return image;
}

QImage GrayscaleEffect::apply(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress
) const{
    if(image.isNull()){
        return QImage();
    }

    QImage result = image.convertToFormat(QImage::Format_ARGB32);

    if(onProgress){
        onProgress(0);
    }

    for(int y = 0; y < result.height(); ++y){
        if(isCanceled(shouldCancel)){
            return QImage();
        }

        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for(int x = 0; x < result.width(); ++x){
            const QColor color(line[x]);
            const int gray = qGray(color.rgb());
            line[x] = qRgba(gray, gray, gray, color.alpha());
        }

            reportProgress(onProgress, y, result.height());
        }

        if(onProgress){
            onProgress(100);
        }

        return result;
}

QImage InvertEffect::apply(
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

QImage SepiaEffect::apply(
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