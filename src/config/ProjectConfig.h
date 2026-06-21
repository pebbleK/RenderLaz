#pragma once

#include "../effect/EffectPass.h"

#include <QList>
#include <QString>
#include <QStringList>

struct ProjectState{
    QStringList resourcePaths;
    QString currentImagePath; // 上次编辑图片
    QList<EffectType> effectTypes;
    QString outputDir;
};

class ProjectConfig{
public:
    static bool saveToFile(
        const QString &filePath,
        const ProjectState &state,
        QString *errorMessage = nullptr
    );

    static bool loadFromFile(
        const QString &filePath,
        ProjectState *state,
        QString *errorMessage = nullptr
    );

    // 转换工具函数
    static QString effectTypeToString(const EffectType type);
    static EffectType effectTypeFromString(const QString &value);
};
