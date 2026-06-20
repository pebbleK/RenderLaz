#pragma once

#include "EffectPass.h"

#include <QString>
#include <QImage>
#include <functional>
#include <memory>

class NullEffect final : public EffectPass{
public:
    QString name() const override{
        return "Null";
    }

    QString effectTypeSuffix() const override{
        return "null";
    }

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const override;
};

class GrayscaleEffect final : public EffectPass{
public:
    QString name() const override{
        return "Grayscale";
    }

    QString effectTypeSuffix() const override{
        return "grayscale";
    }

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const override;
};

class InvertEffect final : public EffectPass{
public:
    QString name() const override{
        return "Invert";
    }

    QString effectTypeSuffix() const override{
        return "invert";
    }

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const override;
};

class SepiaEffect final : public EffectPass{
public:
    QString name() const override{
        return "Sepia";
    }

    QString effectTypeSuffix() const override{
        return "sepia";
    }

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const override;
};