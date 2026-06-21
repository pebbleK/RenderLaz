#pragma once

#include "EffectPass.h"

#include <QString>

class BlurEffect final : public EffectPass{
public:
    QString name() const override{
        return "Blur";
    }

    QString effectTypeSuffix() const override{
        return "blur";
    }

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const override;
};