#include "../app/MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QPixmap>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){
    setWindowTitle("RenderLaz - 实时Shader特效编辑器");
    resize(1280, 820);

    connect(&m_logger, &Logger::messageLogged, this, &MainWindow::appendLog);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();

    m_logger.info("RenderLaz 启动，等待导入资源");
}

void MainWindow::setupMenuBar(){
    QMenu *fileMenu = menuBar()->addMenu("文件");
    QAction *importAction = fileMenu->addAction("导入图片/序列帧");
    connect(importAction, &QAction::triggered, this, &MainWindow::importImages);
    fileMenu->addAction("打开工程");
    fileMenu->addAction("保存工程");
    fileMenu->addSeparator();
    fileMenu->addAction("导出结果");
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close);

    QMenu *effectMenu = menuBar()->addMenu("特效");
    effectMenu->addAction("添加Pass");
    effectMenu->addAction("删除Pass");
    effectMenu->addAction("保存预设");
    effectMenu->addAction("加载预设");

    QMenu *renderMenu = menuBar()->addMenu("渲染");
    renderMenu->addAction("重新编译Shader");
    renderMenu->addAction("开始预览");
    renderMenu->addAction("暂停预览");

    QMenu *taskMenu = menuBar()->addMenu("任务");
    taskMenu->addAction("开始批处理");
    taskMenu->addAction("取消任务");
    taskMenu->addAction("清空任务");

    QMenu *helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction("关于");
}

void MainWindow::setupToolBar(){
    QToolBar *toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);

    QAction *importAction = toolBar->addAction("导入资源");
    connect(importAction, &QAction::triggered, this, &MainWindow::importImages);
    toolBar->addAction("保存工程");
    toolBar->addSeparator();
    toolBar->addAction("编译Shader");
    toolBar->addAction("开始预览");
    toolBar->addAction("停止预览");
    toolBar->addSeparator();
    toolBar->addAction("批量导出");
}

void MainWindow::setupCentralWidget(){
    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 左边局部布局
    auto *leftWidget = new QWidget(mainSplitter);
    auto *leftLayout = new QVBoxLayout(leftWidget);

    auto *projectGroup = new QGroupBox("工程资源", leftWidget);
    auto *projectLayout = new QVBoxLayout(projectGroup);

    m_resourceList = new QListWidget(projectGroup);
    m_resourceList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_resourceList, &QListWidget::itemClicked, this, &MainWindow::showSelectedResource);

    auto *importButton = new QPushButton("导入资源", projectGroup);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importImages);
    projectLayout->addWidget(m_resourceList);
    projectLayout->addWidget(importButton);

    auto *effectGroup = new QGroupBox("特效链", leftWidget);
    auto *effectLayout = new QVBoxLayout(effectGroup);
    auto *effectList = new QListWidget(effectGroup);
    effectList->addItems({"Pass 01 - Color Adjust", "Pass 02 - Gaussian Blur", "Pass 03 - Bloom"});
    auto *addPassButton = new QPushButton("添加Pass", effectGroup);
    effectLayout->addWidget(effectList);
    effectLayout->addWidget(addPassButton);

    leftLayout->addWidget(projectGroup);
    leftLayout->addWidget(effectGroup);

    // 右边局部布局
    auto *rightWidget = new QWidget(mainSplitter);
    auto *rightLayout = new QVBoxLayout(rightWidget);

    m_previewLabel = new QLabel("OpenGL Preview\n等待接入渲染窗口", rightWidget);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(420);
    m_previewLabel->setStyleSheet("background:#111827; color:#e5e7eb; font-size:22px; border:1px solid #374151;");
    m_previewLabel->setScaledContents(false);

    auto *bottomTabs = new QTabWidget(rightWidget);

    auto *parameterWidget = new QWidget(bottomTabs);
    auto *parameterLayout = new QFormLayout(parameterWidget);

    auto *effectCombo = new QComboBox(parameterWidget);
    effectCombo->addItems({"Color Adjust", "Gaussian Blur", "Bloom", "Edge Detect", "Glitch"});

    auto *intensitySpin = new QDoubleSpinBox(parameterWidget);
    intensitySpin->setRange(0.0, 10.0);
    intensitySpin->setSingleStep(0.1);
    intensitySpin->setValue(1.0);

    auto *radiusSlider = new QSlider(Qt::Horizontal, parameterWidget);
    radiusSlider->setRange(0, 100);
    radiusSlider->setValue(25);

    auto *outputPathEdit = new QLineEdit("./output", parameterWidget);
    auto *progressBar = new QProgressBar(parameterWidget);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    parameterLayout->addRow("当前特效", effectCombo);
    parameterLayout->addRow("强度", intensitySpin);
    parameterLayout->addRow("半径", radiusSlider);
    parameterLayout->addRow("输出目录", outputPathEdit);
    parameterLayout->addRow("任务进度", progressBar);

    auto *shaderEditor = new QPlainTextEdit(bottomTabs);
    shaderEditor->setPlainText("// Fragment shader placeholder\n// 后续在这里接入GLSL编辑与编译逻辑\n");

    auto *taskTable = new QTableWidget(0, 5, bottomTabs);
    taskTable->setHorizontalHeaderLabels({"资源", "特效预设", "状态", "进度", "输出路径"});
    taskTable->horizontalHeader()->setStretchLastSection(true);

    m_logTextEdit = new QPlainTextEdit(bottomTabs);
    m_logTextEdit->setReadOnly(true);

    bottomTabs->addTab(parameterWidget, "参数面板");
    bottomTabs->addTab(shaderEditor, "Shader编辑");
    bottomTabs->addTab(taskTable, "批处理任务");
    bottomTabs->addTab(m_logTextEdit, "系统日志");

    rightLayout->addWidget(m_previewLabel);
    rightLayout->addWidget(bottomTabs);

    // 总布局
    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);

    setCentralWidget(mainSplitter);
}

void MainWindow::setupStatusBar(){
    statusBar()->showMessage("未导入资源 | 渲染器未初始化 | 无批处理任务");
}

void MainWindow::importImages(){
    // 弹出对话框，选择图片文件，返回文件的绝对路径
    const QStringList filePaths = QFileDialog::getOpenFileNames(
        this, "导入图片", QString(), "*Images(*.png *.jpg *.jpeg *.bmp *.webp)"
    );

    if(filePaths.isEmpty()){
        return;
    }

    QStringList errorMessages;

    // 获取到的路径
    const QList<ImageResource> addedResources = m_resourceManager.addImage(filePaths, &errorMessages);

    for(const QString &errorMessage : errorMessages){
        m_logger.warning(errorMessage);
    }

    for(const ImageResource &resource : addedResources){
        // 创建一个列表项，显示文件名并自动添加到QListWidget中
        auto *item = new QListWidgetItem(resource.fileName, m_resourceList);
        item->setData(Qt::UserRole, resource.filePath);
        item->setToolTip(resource.filePath); // 鼠标悬停时显示完整路径作为提示
    }

    if(addedResources.isEmpty()){
        m_logger.warning("没有导入的图片资源");
        return;
    }

    m_logger.info(QString("导入完成：%1 张图片").arg(addedResources.size()));
    statusBar()->showMessage(QString("已导入 %1 张照片").arg(m_resourceManager.resources().size()));

    if(m_currentImage.isNull() && m_resourceList->count() > 0){
        m_resourceList->setCurrentRow(0);
        // 流程提交给下一个函数，默认显示第一张图
        showSelectedResource(m_resourceList->item(0));
    }
}

void MainWindow::showSelectedResource(QListWidgetItem *item){
    if(!item){
        return;
    }

    const QString filePath = item->data(Qt::UserRole).toString();

    QString errorMessage;
    QImage image = m_resourceManager.loadImage(filePath, &errorMessage);

    if(image.isNull()){
        m_logger.error(errorMessage);
        statusBar()->showMessage("图片加载失败");
        return;
    }

    m_currentImagePath = filePath;
    m_currentImage = image;

    updatePreviewPixmap();

    // QFileInfo这个类可以获取文件的各类属性，传入文件路径
    const QFileInfo fileInfo(filePath);
    m_logger.info("正在浏览：" + fileInfo.fileName());
    statusBar()->showMessage("当前图片：" + fileInfo.fileName());
}

void MainWindow::updatePreviewPixmap(){
    if(!m_previewLabel || m_currentImage.isNull()){
        return;
    }

    const QSize targetSize = m_previewLabel->contentsRect().size();
    if(targetSize.isEmpty()){
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(m_currentImage).scaled(
        targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation
    );

    // 显示
    m_previewLabel->setPixmap(pixmap);

}

void MainWindow::resizeEvent(QResizeEvent *event){
    // 更新
    QMainWindow::resizeEvent(event);
    updatePreviewPixmap();
}

void MainWindow::appendLog(const QString &message){
    // slot：处理logger传来的数据
    if(m_logTextEdit){
        m_logTextEdit->appendPlainText(message);
    }
}