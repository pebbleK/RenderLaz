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
