/**
 * @file ResourceManager.h
 * @brief 图片资源管理
 * @details 维护已导入图片的列表，内部用集合按绝对路径去重。导入时先用
 *          QImageReader 校验格式是否可读，再建立资源条目；加载图片时开启自动
 *          变换，以处理手机照片的 EXIF 方向信息。
 */
#pragma once

#include <QImage>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

struct ImageResource{
    QString filePath;
    QString fileName;
};

class ResourceManager{
public:
    QList<ImageResource> addImage(const QStringList &filePaths, QStringList *errorMessages = nullptr);
    QImage loadImage(const QString &filePath, QString *errorMessage = nullptr) const;
    const QList<ImageResource> &resources() const;
    void clear();
private:
    bool canReadImage(const QString &filePath, QString *errorMessage = nullptr) const;

private:
    QList<ImageResource> m_resources;
    QSet<QString> m_importedPaths;
};
