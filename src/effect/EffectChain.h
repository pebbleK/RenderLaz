/**
 * @file EffectChain.h
 * @brief 特效链容器
 * @details 按顺序持有一组特效类型，apply 时依次串行执行，并把单个 pass 的进度
 *          折算为整条链的整体进度。支持添加、删除、清空以及名称与类型的查询，
 *          串行结构保证后一个特效总是接收前一个特效的输出结果。
 */
#pragma once

#include "EffectPass.h"

#include <QImage>
#include <QList>
#include <QStringList>

class EffectChain{
public:
    void addPass(EffectType type);
    void removePass(int index);
    void clear();

    bool isEmpty() const;
    int size() const;

    QList<EffectType> effectTypes() const;
    // 返回特效链的特效名字列表
    QStringList passNames() const;

    QImage apply(
        const QImage &image,
        const std::function<bool()> &shouldCancel = {},
        const std::function<void(int)> &onProgress = {}
    ) const;

private:
    QList<EffectType> m_effectTypes;
};