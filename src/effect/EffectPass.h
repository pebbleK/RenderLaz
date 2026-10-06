/**
 * @file EffectPass.h
 * @brief 特效抽象接口与工厂函数
 * @details 定义所有特效的统一基类 EffectPass、支持的特效类型枚举 EffectType，
 *          以及按类型创建特效实例的工厂函数 createEffectPass。apply 约定每个
 *          特效都要支持取消检查（shouldCancel）和进度上报（onProgress），
 *          以便批处理任务能够中断并反馈界面。
 */
#pragma once

#include <QString>
#include <QImage>

enum class EffectType{
    Null,
    Grayscale,
    Invert,
    Sepia,
    Blur
};

//离屏渲染基类
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