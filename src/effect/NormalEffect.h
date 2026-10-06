/**
 * @file NormalEffect.h
 * @brief CPU 图像特效实现
 * @details 基于 QImage 逐像素计算的特效集合：Null（直通）、Grayscale（灰度）、
 *          Invert（反色）、Sepia（棕褐色）。主要使用cpu进行像素计算，未调用图形API, 每个特效在逐行扫描时都会检查取消，标志并按行上报进度，便于大图处理时响应中断。
 */
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