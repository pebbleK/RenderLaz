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

    auto *leftWidget = new QWidget(mainSplitter);
    auto *leftLayout = new QVBoxLayout(leftWidget);

    auto *projectGroup = new QGroupBox("工程资源", leftWidget);
    auto *projectLayout = new QVBoxLayout(projectGroup);

    m_resourceList = new QListWidget(projectGroup);
    m_resourceList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_resourceList, &QListWidget::itemClicked, this, &MainWindow::importImages);

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

    auto *logTextEdit = new QPlainTextEdit(bottomTabs);
    logTextEdit->setReadOnly(true);
    logTextEdit->appendPlainText("RenderLaz启动，等待导入资源...");

    bottomTabs->addTab(parameterWidget, "参数面板");
    bottomTabs->addTab(shaderEditor, "Shader编辑");
    bottomTabs->addTab(taskTable, "批处理任务");
    bottomTabs->addTab(logTextEdit, "系统日志");

    // 布局
    rightLayout->addWidget(m_previewLabel);
    rightLayout->addWidget(bottomTabs);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);

    setCentralWidget(mainSplitter);
}

void MainWindow::setupStatusBar(){
    statusBar()->showMessage("未导入资源 | 渲染器未初始化 | 无批处理任务");
}
