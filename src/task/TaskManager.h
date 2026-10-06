/**
 * @file TaskManager.h
 * @brief 批处理任务框架
 * @details TaskManager 在独立线程上驱动 ImageBatchWorker，对输入图片列表依次应用
 *          特效链并导出 PNG，通过信号上报每张图片的进度、完成或失败状态。
 *          取消采用线程中断请求的方式实现，worker 在处理每张图片及特效执行过程
 *          中检查该标志。
 */
#pragma once
#include "../effect/EffectPass.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

class QThread;

// 图片处理线程
class ImageBatchWorker : public QObject{
    Q_OBJECT

public:
    ImageBatchWorker(QStringList inputPaths, QString outputDir, QList<EffectType> effectTypes)
    : m_inputPaths(std::move(inputPaths))
    , m_outputDir(std::move(outputDir))
    , m_effectTypes(effectTypes){}

public slots:
    void process();
    // void cancel();

signals: 
    void taskStarted(int row, const QString &inputPath);
    void taskFinished(int row, const QString &outputPath);
    void taskFailed(int row, const QString &errorMessage);

    void finished();
    void canceled();

    void taskProgressChanged(int row, int progress);

    void logMessage(const QString &message);

private:
    QStringList m_inputPaths;
    QString m_outputDir;
    QList<EffectType> m_effectTypes;
    // bool m_cancelRequested = false;
};

// 任务管理器
class TaskManager : public QObject{
    Q_OBJECT

public:
    explicit TaskManager(QObject *parent = nullptr);
    ~TaskManager() override;

    bool isRunning() const;

public slots:
    void startBatch(const QStringList &inputPaths, const QString &outputDir, QList<EffectType> effectTypes);
    void cancel();

signals:
    void batchStarted(int totalNum);
    void taskStarted(int row, const QString &inputPath);

    void taskFinished(int row, const QString &outputPath);
    void taskFailed(int row, const QString &errorMessage);

    void batchFinished();
    void batchCanceled();

    // 进度显示：0-100
    void taskProgressChanged(int row, int progress);

    void logMessage(const QString &message);

private:
    void cleanupThread();

private:
    QThread *m_thread = nullptr;
    ImageBatchWorker *m_worker = nullptr;
    bool m_running = false;
};
