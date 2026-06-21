#pragma once

#include <QImage>
#include <QString>

class RenderEngine{
public:
    RenderEngine();
    ~RenderEngine();

    QImage renderBlur(const QImage &image, 
        float radius, QString *errorMessage = nullptr);

private:
    bool initialize(QString *errorMessage);

private:
    bool m_initialize = false;
};
