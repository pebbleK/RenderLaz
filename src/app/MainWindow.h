#pragma once
#include "../logger/Logger.h"
#include "../resource/ResourceManager.h"

#include <QMainWindow>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QResizeEvent;

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();

    void importImages();
    void showSelectedResource(QListWidgetItem *item);
    void updatePreviewPixmap();
    void appendLog(const QString &message);

    QListWidget *m_resourceList = nullptr;
    QLabel *m_previewLabel = nullptr;
    QPlainTextEdit *m_logTextEdit = nullptr;

    ResourceManager *m_resourceManager;
    Logger m_logger;

    QString m_currentImagePath;
    QImage m_currentImage;
};
