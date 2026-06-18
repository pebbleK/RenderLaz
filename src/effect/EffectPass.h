#pragma once

#include <QObject>
#include <QString>
#include <QImage>

enum class EffectType{
    Null,
    Grayscale,
    Invert,
    Sepia
};

class EffectPass{
public:
    explicit EffectPass(EffectType type = EffectType::Null);

    QString name() const;
    QString effectTypeSuffix() const;

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const;

private:
    QImage applyNull(
        const QImage &image,
        const std::function<bool()> &shouldCancel,
        const std::function<void(int)> &onProgress
    ) const;

    QImage applyGrayscale(
        const QImage &image,
        const std::function<bool()> &shouldCancel,
        const std::function<void(int)> &onProgress
    ) const;

    QImage applyInvert(
        const QImage &image,
        const std::function<bool()> &shouldCancel,
        const std::function<void(int)> &onProgress
    ) const;

    QImage applySepia(
        const QImage &image,
        const std::function<bool()> &shouldCancel,
        const std::function<void(int)> &onProgress
    ) const;
    
    static bool isCanceled(const std::function<bool()> &shouldCancel);
    static void reportProgress(
        const std::function<void(int)> &onProgress,
        int row,
        int totalRows);

private:
    EffectType m_type = EffectType::Null;
    
};