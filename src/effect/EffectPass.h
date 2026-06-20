#pragma once

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
    virtual ~EffectPass() = default;

    virtual QString name() const = 0;
    virtual QString effectTypeSuffix() const = 0;

    virtual QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const = 0;

protected:
    static bool isCanceled(const std::function<bool()> &shouldCancel);
    static void reportProgress(
        const std::function<void(int)> &onProgress,
        int row,
        int totalRows);
};

// 工厂函数
std::unique_ptr<EffectPass> createEffectPass(EffectType type);