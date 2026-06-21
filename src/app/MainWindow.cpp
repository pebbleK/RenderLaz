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
#include <QDir>
#include <QMessageBox>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){
    setWindowTitle("RenderLaz - 实时Shader特效编辑器");
    resize(1280, 820);

    connect(&m_logger, &Logger::messageLogged, this, &MainWindow::appendLog);
    connect(&m_taskManager, &TaskManager::batchStarted, this, &MainWindow::handleBatchStarted);
    connect(&m_taskManager, &TaskManager::taskStarted, this, &MainWindow::handleTaskStarted);
    connect(&m_taskManager, &TaskManager::taskProgressChanged, this, &MainWindow::handleTaskProgressChanged);
    connect(&m_taskManager, &TaskManager::taskFinished, this, &MainWindow::handleTaskFinished);
    connect(&m_taskManager, &TaskManager::taskFailed, this, &MainWindow::handleTaskFailed);
    connect(&m_taskManager, &TaskManager::batchFinished, this, &MainWindow::handleBatchFinished);
    connect(&m_taskManager, &TaskManager::batchCanceled, this, &MainWindow::handleBatchCanceled);
    connect(&m_taskManager, &TaskManager::logMessage, &m_logger, &Logger::info);

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
    QAction *openProjectAction = fileMenu->addAction("打开工程");
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    QAction *saveProjectAction = fileMenu->addAction("保存工程");
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);

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
    QAction *startBatchAction = taskMenu->addAction("开始批处理");
    connect(startBatchAction, &QAction::triggered, this, &MainWindow::startBatchProcess);

    QAction *cancelTaskAction = taskMenu->addAction("取消任务");
    connect(cancelTaskAction, &QAction::triggered, this, &MainWindow::cancelBatchProcess);

    QMenu *helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction("关于");
}

void MainWindow::setupToolBar(){
    QToolBar *toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);

    QAction *importAction = toolBar->addAction("导入资源");
    connect(importAction, &QAction::triggered, this, &MainWindow::importImages);

    QAction *saveProjectAction = toolBar->addAction("保存工程");
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);

    toolBar->addSeparator();
    toolBar->addAction("编译Shader");
    toolBar->addAction("开始预览");
    toolBar->addAction("停止预览");
    toolBar->addSeparator();

    QAction *batchExportAction = toolBar->addAction("批量导出");
    connect(batchExportAction, &QAction::triggered, this, &MainWindow::startBatchProcess);
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
    m_effectList = new QListWidget(effectGroup);
    m_addPassButton = new QPushButton("添加Pass", effectGroup);
    m_removePassButton = new QPushButton("删除Pass", effectGroup);
    connect(m_addPassButton, &QPushButton::clicked, this, 
    &MainWindow::addSelectedEffectPass);
    connect(m_removePassButton, &QPushButton::clicked, this, 
    &MainWindow::removeSelectedEffectPass);

    effectLayout->addWidget(m_effectList);
    effectLayout->addWidget(m_addPassButton);
    effectLayout->addWidget(m_removePassButton);

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

    m_effectCombo = new QComboBox(parameterWidget);
    m_effectCombo->addItems({{"Null", "Grayscale", "Invert", "Sepia", "Blur"}});
    connect(m_effectCombo, 
        &QComboBox::currentTextChanged, this, [this](const QString &){
            refreshEffectPreview();
    });

    auto *intensitySpin = new QDoubleSpinBox(parameterWidget);
    intensitySpin->setRange(0.0, 10.0);
    intensitySpin->setSingleStep(0.1);
    intensitySpin->setValue(1.0);

    auto *radiusSlider = new QSlider(Qt::Horizontal, parameterWidget);
    radiusSlider->setRange(0, 100);
    radiusSlider->setValue(25);

    m_outputPathEdit = new QLineEdit("./output", parameterWidget);
    m_taskProgressBar = new QProgressBar(parameterWidget);
    m_taskProgressBar->setRange(0, 100);
    m_taskProgressBar->setValue(0);

    parameterLayout->addRow("当前特效", m_effectCombo);
    parameterLayout->addRow("强度", intensitySpin);
    parameterLayout->addRow("半径", radiusSlider);
    parameterLayout->addRow("输出目录", m_outputPathEdit);
    parameterLayout->addRow("任务进度", m_taskProgressBar);

    auto *shaderEditor = new QPlainTextEdit(bottomTabs);
    shaderEditor->setPlainText("// Fragment shader placeholder\n// 后续在这里接入GLSL编辑与编译逻辑\n");

    m_taskTable = new QTableWidget(0, 5, bottomTabs);
    m_taskTable->setHorizontalHeaderLabels({"资源", "特效预设", "状态", "进度", "输出路径"});
    m_taskTable->horizontalHeader()->setStretchLastSection(true);

    m_logTextEdit = new QPlainTextEdit(bottomTabs);
    m_logTextEdit->setReadOnly(true);

    bottomTabs->addTab(parameterWidget, "参数面板");
    bottomTabs->addTab(shaderEditor, "Shader编辑");
    bottomTabs->addTab(m_taskTable, "批处理任务");
    bottomTabs->addTab(m_logTextEdit, "系统日志");

    rightLayout->addWidget(m_previewLabel);
    rightLayout->addWidget(bottomTabs);

    // 总布局
    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 1); // 设置组件拉伸占比
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
        // 创建一个列表项，显示文件名并添加到QListWidget中
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
        m_resourceList->setCurrentRow(0); // 带有选中高亮效果

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

    refreshEffectPreview();

    // QFileInfo这个类可以获取文件的各类属性，传入文件路径
    const QFileInfo fileInfo(filePath);
    m_logger.info("正在浏览：" + fileInfo.fileName());
    statusBar()->showMessage("当前图片：" + fileInfo.fileName());
}

void MainWindow::refreshEffectPreview(){
    if(!m_previewLabel){
        return;
    }

    if(m_currentImage.isNull()){
        m_previewImage = QImage();
        m_previewLabel->clear();
        m_previewLabel->setText("OpenGL Preview\n请先导入图片");
        return;
    }

    if(m_effectChain.isEmpty()){
        m_previewImage = m_currentImage;
        updatePreviewPixmap();
        return;
    }

    const QImage result = m_effectChain.apply(m_currentImage); // 特效应用到读取图片

    if(result.isNull()){
        m_previewImage = m_currentImage;
        m_logger.error("预览特效应用失败，已显示原图");
    }else{
        m_previewImage = result;
    }

    updatePreviewPixmap();
}

void MainWindow::updatePreviewPixmap(){
    if(!m_previewLabel || m_previewImage.isNull()){
        return;
    }

    const QSize targetSize = m_previewLabel->contentsRect().size();
    if(targetSize.isEmpty()){
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(m_previewImage).scaled(
        targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation
    );

    // 显示
    m_previewLabel->setPixmap(pixmap);

}

void MainWindow::resizeEvent(QResizeEvent *event){
    // 拉伸窗口比例后
    QMainWindow::resizeEvent(event);
    updatePreviewPixmap();
}

void MainWindow::appendLog(const QString &message){
    // slot：处理logger传来的数据
    if(m_logTextEdit){
        m_logTextEdit->appendPlainText(message);
    }
}

QStringList MainWindow::importedImagePaths() const{
    QStringList paths;

    for(const ImageResource &resource : m_resourceManager.resources()){
        paths.append(resource.filePath);
    }

    return paths;
}

EffectType MainWindow::selectedEffectType() const{
    if(!m_effectCombo){
        return EffectType::Null;
    }

    // 从下拉combo中获取特效类型
    const QString effectName = m_effectCombo->currentText();

    if(effectName == "Null"){
        return EffectType::Null;
    }

    if(effectName == "Grayscale"){
        return EffectType::Grayscale;
    }

    if(effectName == "Invert"){
        return EffectType::Invert;
    }

    if(effectName == "Sepia"){
        return EffectType::Sepia;
    }

    if(effectName == "Blur"){
        return EffectType::Blur;
    }

    return EffectType::Null;
}

void MainWindow::addSelectedEffectPass(){
    const EffectType type = selectedEffectType();
    m_effectChain.addPass(type);

    const auto pass = createEffectPass(type);
    m_effectList->addItem(pass->name());

    refreshEffectPreview();
}

void MainWindow::removeSelectedEffectPass(){
    const int row = m_effectList->currentRow();
    if(row < 0){
        return;
    }

    m_effectChain.removePass(row);
    delete m_effectList->takeItem(row);

    refreshEffectPreview();
}

QString structPlace::statusToString(Status status)
{
    switch (status) {
    case Status::Pending:
        return "等待中";
    case Status::Running:
        return "处理中";
    case Status::Finished:
        return "已完成";
    case Status::Failed:
        return "失败";
    case Status::Canceled:
        return "已取消";
    }

    return "未知";
}

void MainWindow::startBatchProcess(){
    if(m_taskManager.isRunning()){
        m_logger.warning("任务正在处理...");
        return;
    }

    const QStringList inputPaths = importedImagePaths();
    if(inputPaths.empty()){
        QMessageBox::information(this, "批处理", "请先导入图片资源");
        return;
    }

    const QString outputDir = m_outputPathEdit->text().trimmed(); // .trimmed()两头空格去除
    if(outputDir.isEmpty()){
        QMessageBox::information(this, "批处理", "请填写输出目录");
        return;
    }

    const QList<EffectType> effectTypes = m_effectChain.effectTypes();
    const QString effectName = m_effectChain.isEmpty() ? QString("Null")
    : m_effectChain.passNames().join("->"); // join()把列表里所有字符串串成一整条，中间用“separator“分隔。

    if(m_taskTable){
        m_taskTable->setRowCount(inputPaths.size());

        for(int row = 0; row < inputPaths.size(); ++row){
            const QFileInfo fileInfo(inputPaths.at(row));

            // 名字栏，特效栏，状态栏，进度栏，输出路径
            m_taskTable->setItem(row, 0, new QTableWidgetItem(fileInfo.fileName()));
            m_taskTable->setItem(row, 1, new QTableWidgetItem(effectName));
            m_taskTable->setItem(row, 2, new QTableWidgetItem(statusToString(structPlace::Status::Pending)));
            m_taskTable->setItem(row, 3, new QTableWidgetItem("0%"));
            m_taskTable->setItem(row, 4, new QTableWidgetItem(""));
        }
    }

    if(m_taskProgressBar){
        m_taskProgressBar->setValue(0);
    }

    m_logger.info(QString("开始后台批处理，共 %1 张图片").arg(inputPaths.size()));
    statusBar()->showMessage("批处理运行中");

    // 转入任务管理中运行
    m_taskManager.startBatch(inputPaths, outputDir, effectTypes);
}

void MainWindow::cancelBatchProcess(){
    if(!m_taskManager.isRunning()){
        m_logger.warning("当前没有运行的任务");
        return;
    }

    m_logger.warning("正在取消批处理任务");
    m_taskManager.cancel();
}

void MainWindow::handleBatchStarted(int totalCount){
    m_logger.info(QString("批处理任务已启动，任务数量：%1").arg(totalCount));
}

void MainWindow::handleTaskStarted(int row, const QString &inputPath){
    if(m_taskTable && row >= 0 && row < m_taskTable->rowCount()){
        m_taskTable->setItem(row, 2, 
            new QTableWidgetItem(statusToString(structPlace::Status::Running)));
    }

    const QFileInfo fileInfo(inputPath);
    m_logger.info("正在处理：" + fileInfo.fileName());
}

void MainWindow::handleTaskProgressChanged(int row, int progress){
    if(m_taskTable && row >= 0 && row < m_taskTable->rowCount()){
            m_taskTable->setItem(row, 3, 
        new QTableWidgetItem(QString("%1%").arg(progress)));
    }

    /*
    row * 100 - 前row张已完成的图贡献的进度（每张100per）
    progress — 当前这张图内部的进度（0~100）
    totalRows — 把总分平摊到总张数
    */
    if(m_taskProgressBar && m_taskTable && m_taskTable->rowCount() > 0){
        const int totalRows = m_taskTable->rowCount();
        const int overallProgress = ((row * 100) + progress) / totalRows;
        m_taskProgressBar->setValue(overallProgress);
    }
}

void MainWindow::handleTaskFinished(int row, const QString &outputPath){
    if(m_taskTable && row >=0 && row < m_taskTable->rowCount()){
        m_taskTable->setItem(row, 2, 
            new QTableWidgetItem(statusToString(structPlace::Status::Finished)));
        m_taskTable->setItem(row, 3, new QTableWidgetItem("100%"));
        m_taskTable->setItem(row, 4, new QTableWidgetItem(outputPath));
    }

    m_logger.info("输出完成：" + outputPath);
}

void MainWindow::handleTaskFailed(int row, const QString &errorMessage){
    if(m_taskTable && row >= 0 && row < m_taskTable->rowCount()){
        m_taskTable->setItem(row, 2, 
            new QTableWidgetItem(statusToString(structPlace::Status::Failed)));
    }

    m_logger.error(errorMessage);
}

void MainWindow::handleBatchFinished(){
    if(m_taskProgressBar){
        m_taskProgressBar->setValue(100);
    }

    m_logger.info("批处理任务全部完成");
    statusBar()->showMessage("批处理完成");
}

void MainWindow::handleBatchCanceled(){
    if(m_taskTable){
        for(int row = 0; row < m_taskTable->rowCount(); ++row){
            QTableWidgetItem *statusItem = m_taskTable->item(row, 2);
            const QString status = statusItem ? statusItem->text() : QString();

            // “等待中“和“运行中“改为“已取消“，完成和失败的不改动
            if(status != statusToString(structPlace::Status::Finished) 
            && status != statusToString(structPlace::Status::Failed)){
                m_taskTable->setItem(row, 2, 
                    new QTableWidgetItem(statusToString(structPlace::Status::Canceled)));
            }
        }
    }

    m_logger.warning("批处理任务已取消");
    statusBar()->showMessage("批处理取消");
}

void MainWindow::saveProject(){
    // 弹窗选择保存位置和名字
    const QString filePath = QFileDialog::getSaveFileName(
        this, "保存工程", QString(), 
        "RenderLaz Project(*.render)");

    if(filePath.isEmpty()){
        return;
    }

    QString errorMessage;
    if(!ProjectConfig::saveToFile(filePath, currentProjectState(), &errorMessage)){
        QMessageBox::warning(this, "保存工程", errorMessage);
        m_logger.error(errorMessage);
        return;
    }

    m_logger.info("工程已保存：" + filePath);
    statusBar()->showMessage("工程已保存");
}

void MainWindow::openProject(){
    // 弹窗选择读取位置和名字
    const QString filePath = QFileDialog::getOpenFileName(
    this, "打开工程", QString(),
    "RenderLaz Project (*.renderlaz)");
    
    if(filePath.isEmpty()){
        return;
    }

    ProjectState state;
    QString errorMessage;

    if(!ProjectConfig::loadFromFile(filePath, &state, &errorMessage)){
        QMessageBox::warning(this, "打开工程", errorMessage);
        m_logger.error(errorMessage);
        return;
    }

    applyProjectState(state);

    m_logger.info("工程已打开" + filePath);
    statusBar()->showMessage("工程已打开");
}

ProjectState MainWindow::currentProjectState() const{
    ProjectState state;
    state.resourcePaths = importedImagePaths();
    state.currentImagePath = m_currentImagePath;
    state.effectTypes = m_effectChain.effectTypes();

    if(m_outputPathEdit){
        state.outputDir = m_outputPathEdit->text().trimmed();
    }

    return state;
}

void MainWindow::applyProjectState(const ProjectState &state){
    m_resourceManager.clear();
    m_resourceList->clear();

    QStringList errorMessages;
    const QList<ImageResource> addedResources = 
        m_resourceManager.addImage(state.resourcePaths, &errorMessages);

    for(const QString &errorMessage : errorMessages){
        m_logger.warning(errorMessage);
    }

    for(const ImageResource &resource : addedResources){
        auto item = new QListWidgetItem(resource.fileName, m_resourceList);
        item->setData(Qt::UserRole, resource.filePath);
        item->setToolTip(resource.filePath);
    }

    m_effectChain.clear();
    for(const EffectType type: state.effectTypes){
        m_effectChain.addPass(type);
    }
    refreshEffectList();

    m_outputPathEdit->setText(state.outputDir.isEmpty() ? "./output" : state.outputDir);

    m_currentImagePath.clear();
    m_currentImage = QImage();
    m_previewImage = QImage();

    int currentRow = -1;
    for(int row = 0; row < m_resourceList->count(); ++row){
        QListWidgetItem *item = m_resourceList->item(row);
        // 遍历找到工程保存的图片
        if(item && item->data(Qt::UserRole).toString() == state.currentImagePath){
            currentRow = row;
            break;
        }
    }

    if(currentRow == -1 && m_resourceList->count() > 0){
        currentRow = 0;
    }

    if(currentRow >= 0){
        m_resourceList->setCurrentRow(currentRow);
        showSelectedResource(m_resourceList->item(currentRow));
    }else{
        refreshEffectPreview();
    }
}

void MainWindow::refreshEffectList(){
    if(!m_effectList){
        return;
    }

    m_effectList->clear();

    const QStringList names = m_effectChain.passNames();
    for(const QString &name : names){
        m_effectList->addItem(name);
    }
}