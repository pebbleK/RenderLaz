#include "TaskManager.h"
#include "../effect/EffectChain.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QThread>

void ImageBatchWorker::process(){
    QDir outputDir(m_outputDir);

    // mkpath()创建目录，"."表示确保当前m_outputDir目录存在，不存在就创建
    if(!outputDir.exists() && !outputDir.mkpath(".")){
        emit logMessage("无法创建目录：" + m_outputDir);
        emit canceled();
        return;
    }

    EffectChain effectChain;
    for(EffectType type : m_effectTypes){
        effectChain.addPass(type);
    }

    for(int row = 0; row < m_inputPaths.size(); ++row){
        if(QThread::currentThread()->isInterruptionRequested()){
            emit canceled();
            return;
        }

        const QString inputPath = m_inputPaths.at(row); // at()自带边界检查，越界会抛异常
        emit taskStarted(row, inputPath);
        emit taskProgressChanged(row, 5);

        QImage image(inputPath);
        if(image.isNull()){
            emit taskFailed(row, "无法加载图片" + inputPath);
            continue;
        }

        emit taskProgressChanged(row, 30);

        QImage result = effectChain.apply(image, 
            [](){
            return QThread::currentThread()->isInterruptionRequested();},
            [this, row](int effectProgress){
                const int progress = 30 + effectProgress * 50 / 100;
                emit taskProgressChanged(row, progress);
            });
        
        if(QThread::currentThread()->isInterruptionRequested()){
            emit canceled();
            return;
        }

        if(result.isNull()){
            emit taskFailed(row, "图片处理失败：" + inputPath);
            continue;
        }

        emit taskProgressChanged(row, 85);

        // 命名输出文件
        const QFileInfo inputInfo(inputPath);
        const QString baseName = inputInfo.completeBaseName();
        const QString outputPath = outputDir.filePath(
        baseName + "_output" + ".png");
        if(!result.save(outputPath)){
            emit taskFailed(row, "保存失败：" + outputPath);
            continue;
        }

        emit taskProgressChanged(row, 100);
        emit taskFinished(row, outputPath);
    }

    emit finished();
}

// void ImageBatchWorker::cancel(){
//     m_cancelRequested = true;
// }

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

void TaskManager::startBatch(const QStringList &inputPaths, 
    const QString &outputDir, 
    QList<EffectType> effectTypes){

    if(m_running || inputPaths.isEmpty()){
        return;
    }

    m_running = true;

    m_thread = new QThread(this);
    m_worker = new ImageBatchWorker(inputPaths, outputDir, effectTypes);

    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &ImageBatchWorker::process);

    connect(m_worker, &ImageBatchWorker::taskStarted, this, &TaskManager::taskStarted);
    connect(m_worker, &ImageBatchWorker::taskProgressChanged, this, &TaskManager::taskProgressChanged);
    
    // 线程任务结束，由worker通知QThread和TaskManager
    connect(m_worker, &ImageBatchWorker::finished, m_thread, &QThread::quit);
    connect(m_worker, &ImageBatchWorker::canceled, m_thread, &QThread::quit);

    connect(m_worker, &ImageBatchWorker::finished, this, [this](){
        emit batchFinished();
    });
    connect(m_worker, &ImageBatchWorker::canceled, this, [this](){
        emit batchCanceled();
    });

    // TaskManager在QThread退出后再清理状态
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, this, [this](){
        m_running = false;
        m_thread->deleteLater();
        m_thread = nullptr;
        m_worker = nullptr;
    });
    // 分离TaskManager和QThread线程退出过程

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
        // m_thread->deleteLater();
    }

    m_thread = nullptr;
    m_worker = nullptr;
}