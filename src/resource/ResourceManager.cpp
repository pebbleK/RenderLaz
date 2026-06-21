#include "ResourceManager.h"

#include <QFileInfo>
#include <QImageReader>

QList<ImageResource> ResourceManager::addImage(const QStringList &filePaths, QStringList *errorMessages){
    QList<ImageResource> addedResources;

    for(const QString &filePath : filePaths){
        const QFileInfo fileInfo(filePath);
        const QString absolutePath = fileInfo.absoluteFilePath();

        // duplicate resource
        if(m_importedPaths.contains(absolutePath)){
            if(errorMessages){
                errorMessages->append("跳过重复资源：" + absolutePath);
            }
            continue;
        }

        // error report
        QString errorMessage;
        if(!canReadImage(absolutePath, &errorMessage)){
            if(errorMessages){
                errorMessages->append(errorMessage);
            }
            continue;
        }

        ImageResource resource;
        resource.filePath = absolutePath;
        resource.fileName = fileInfo.fileName();

        m_importedPaths.insert(absolutePath);
        m_resources.append(resource);
        addedResources.append(resource);
    }

    return addedResources;
}

void ResourceManager::clear(){
    m_resources.clear();
    m_importedPaths.clear();
}

QImage ResourceManager::loadImage(const QString &filePath, QString *errorMessage) const{
    QImageReader reader(filePath);
    reader.setAutoTransform(true); // 它可以处理部分图片的 EXIF 方向信息，比如手机拍摄图片旋转问题。
    // 例如竖着拍了一张照片，照片文件里的像素数据可能实际还是“横着”的，通过 EXIF 信息就能读出正确的像素数据顺序。
    
    QImage image = reader.read();
    if(image.isNull()){
        if(errorMessage){
            *errorMessage = "无法加载图片：" + filePath + "，原因：" + reader.errorString();
        }
        return QImage();
    }

    return image;
}

const QList<ImageResource> &ResourceManager::resources() const{
    return m_resources;
}

bool ResourceManager::canReadImage(const QString &filePath, QString *errorMessage) const{
    QImageReader reader(filePath);
    reader.setAutoTransform(true);

    if(!reader.canRead()){
        if(errorMessage){
            *errorMessage = "无法读取图片：" + filePath + "，原因：" + reader.errorString();
        }
        return false;
    }

    return true;
}
