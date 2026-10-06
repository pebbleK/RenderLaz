/**
 * @file ProjectConfig.h
 * @brief 工程文件的保存与读取
 * @details 定义可持久化的工程状态 ProjectState，并提供与 JSON 文件互转的静态接口。
 *          工程内容包含资源路径列表、当前编辑的图片、特效链类型序列和输出目录，
 *          另附 EffectType 与字符串之间的转换工具，用于序列化与反序列化。
 */
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
