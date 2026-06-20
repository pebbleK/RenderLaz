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