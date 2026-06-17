#include "TaskManager.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QThread>
#include <QColor>

void ImageBatchWorker::process(){
    QDir outputDir(m_outputDir);

    // mkpath()创建目录，"."表示确保当前m_outputDir目录存在，不存在就创建
    if(!outputDir.exists() && !outputDir.mkpath(".")){
        emit logMessage("无法创建目录：" + m_outputDir);
        emit canceled();
        return;
    }

    for(int row = 0; row < m_inputPaths.size(); ++row){
        // if(m_cancelRequested){
        //     emit canceled();
        //     return;
        // }

        const QString inputPath = m_inputPaths.at(row); // at()自带边界检查，越界会抛异常
        emit taskStarted(row, inputPath);
        emit taskProgressChanged(row, 5);

        QImage image(inputPath);
        if(image.isNull()){
            emit taskFailed(row, "无法加载图片" + inputPath);
            continue;
        }

        emit taskProgressChanged(row, 30);

        QImage result = image.convertToFormat(QImage::Format_ARGB32);

        for(int y = 0; y < result.height(); ++y){
            // if(m_cancelRequested){
            //     emit canceled();
            //     return;
            // }

            if(QThread::currentThread()->isInterruptionRequested()){
                emit canceled();
                return;
            }

            // rbg按权重转换为灰度值
            QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
            for(int x = 0; x < result.width(); ++x){
                const QColor color(line[x]);
                const int gray = qGray(color.rgb());
                line[x] = qRgba(gray, gray, gray, color.alpha());
            }

            if(result.height() > 0 && y % 20 == 0){
                const int progress = 30 + (y * 50 / result.height());
                emit taskProgressChanged(row, progress);
            }
        }

        emit taskProgressChanged(row, 85);

        // 命名输出文件
        const QFileInfo inputInfo(inputPath);
        const QString baseName = inputInfo.completeBaseName();
        const QString outputPath = outputDir.filePath(baseName + "_grey.png");

        if(!result.save(outputPath)){
            emit taskFailed(row, "保存失败：" + outputPath);
            continue;
        }

        emit taskProgressChanged(row, 100);
        emit taskFinished(row, outputPath);
    }

    emit finished();
}

void ImageBatchWorker::cancel(){
    m_cancelRequested = true;
}

TaskManager::TaskManager(QObject *parent) : QObject(parent){}

TaskManager::~TaskManager(){
    cancel();

    if(m_thread){
        // 先通知线程退出，再等待线程完成（退出），而不是直接杀死线程
        m_thread->quit();
        m_thread->wait();
    }
}

bool TaskManager::isRunning() const{
    return m_running;
}

void TaskManager::startBatch(const QStringList &inputPaths, const QString &outputDir){
    if(m_running || inputPaths.isEmpty()){
        return;
    }

    m_running = true;

    m_thread = new QThread(this);
    m_worker = new ImageBatchWorker(inputPaths, outputDir);

    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &ImageBatchWorker::process);

    connect(m_worker, &ImageBatchWorker::taskStarted, this, &TaskManager::taskStarted);
    connect(m_worker, &ImageBatchWorker::taskProgressChanged, this, &TaskManager::taskProgressChanged);
    connect(m_worker, &ImageBatchWorker::taskFinished, this, &TaskManager::taskFinished);
    connect(m_worker, &ImageBatchWorker::taskFailed, this, &TaskManager::taskFailed);
    connect(m_worker, &ImageBatchWorker::logMessage, this, &TaskManager::logMessage);

    connect(m_worker, &ImageBatchWorker::finished, this, [this](){
        emit batchFinished();
        cleanupThread();
    });

    connect(m_worker, &ImageBatchWorker::canceled, this, [this](){
        emit batchCanceled();
        cleanupThread();
    });

    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    emit batchStarted(inputPaths.size());
    m_thread->start();
}

void TaskManager::cancel(){
    if(!m_worker){
        return;
    }
    // /* 异步调用ImageBatchWorker的cancel()方法：
    // 在m_worker对象所在的线程的事件循环中，异步调用它的 cancel 槽函数。
    // 因为m_worker运行在工作线程，调用方在主线程。直接调会跨线程访问对象，线程不安全且Qt会报警告。
    // QueuedConnection 把"调用cancel"这个动作包装成一个事件，投递到m_worker所属线程的事件队列里排队，等那个线程轮到处理时才执行。
    // */
    // QMetaObject::invokeMethod(m_worker, "cancel", Qt::QueuedConnection); // slot函数太长可能会导致运行结束才取消

    m_thread->requestInterruption();
}

void TaskManager::cleanupThread(){
    m_running = false;

    if(m_thread){
        m_thread->quit();
        m_thread->wait();
        m_thread->deleteLater();
    }

    m_thread = nullptr;
    m_worker = nullptr;
}