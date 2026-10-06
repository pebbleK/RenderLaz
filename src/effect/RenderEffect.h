/**
 * @file RenderEffect.h
 * @brief GPU 渲染特效实现
 * @details 基于 Qt RHI 离屏渲染的特效，目前提供 BlurEffect：可分离高斯模糊，
 *          在 RenderEngine 中拆成横向、纵向两个渲染 pass 完成，再回读为 QImage。
 *          与 CPU 特效不同，该阶段无法中途取消，只能在进入渲染前检查。
 */
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