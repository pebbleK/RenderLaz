/**
 * @file MainWindow.h
 * @brief 应用主窗口
 * @details 负责界面搭建与交互协调：左侧资源列表和特效链，右侧预览区与参数面板、
 *          Shader 编辑、批处理任务、系统日志四个页签。串联资源导入、特效链编辑、
 *          批处理导出、工程保存与打开等流程，并把子模块（ResourceManager、
 *          TaskManager、Logger）的信号转发到对应控件。
 */
#pragma once
#include "../logger/Logger.h"
#include "../resource/ResourceManager.h"
#include "../task/TaskManager.h"
#include "../effect/EffectPass.h"
#include "../effect/EffectChain.h"
#include "../config/ProjectConfig.h"

#include <QMainWindow>
#include <QString>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QResizeEvent;
class QProgressBar;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QPushButton;
class QDoubleSpinBox;
class QSlider;
class QFormLayout;

namespace structPlace {
    enum class Status {
    Pending,
    Running,
    Finished,
    Failed,
    Canceled
    };

    QString statusToString(Status status);
}

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();

    // 输入图片
    void importImages();
    void showSelectedResource(QListWidgetItem *item);
    void updatePreviewPixmap();
    void appendLog(const QString &message);
    void refreshEffectPreview();

    // 线程处理
    void startBatchProcess();
    void cancelBatchProcess();

    // 更新任务状态
    void handleBatchStarted(int totalCount);
    void handleTaskStarted(int row, const QString &inputPath);
    void handleTaskProgressChanged(int row, int progress);
    void handleTaskFinished(int row, const QString &outputPath);
    void handleTaskFailed(int row, const QString &errorMessage);
    void handleBatchFinished();
    void handleBatchCanceled();

    // 特效处理
    QStringList importedImagePaths() const;
    EffectType selectedEffectType() const;
    void addSelectedEffectPass();
    void removeSelectedEffectPass();
    void updateEffectParameterPanel();

    // 保存配置/工程
    void saveProject();
    void openProject();

    ProjectState currentProjectState() const;
    void applyProjectState(const ProjectState &state);
    void refreshEffectList();

private:
    QListWidget *m_resourceList = nullptr;
    QLabel *m_previewLabel = nullptr;
    QPlainTextEdit *m_logTextEdit = nullptr;

    ResourceManager m_resourceManager;
    Logger m_logger;

    QString m_currentImagePath;
    QImage m_currentImage;
    QImage m_previewImage;

    QLineEdit *m_outputPathEdit = nullptr;

    QProgressBar *m_taskProgressBar = nullptr;
    QTableWidget *m_taskTable = nullptr; // 状态表格栏

    TaskManager m_taskManager;

    // 特效控件
    QComboBox *m_effectCombo = nullptr; // 下拉框控件
    QDoubleSpinBox *m_intensitySpin = nullptr;
    QSlider *m_radiusSlider = nullptr;
    QListWidget *m_effectList = nullptr;
    QPushButton *m_addPassButton = nullptr;
    QPushButton *m_removePassButton = nullptr;
    QFormLayout *m_parameterLayout = nullptr;
    EffectChain m_effectChain;
};
