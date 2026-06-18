#pragma once
#include "../effect/EffectPass.h"

#include <QObject>
#include <QString>
#include <QStringList>

class QThread;

// 图片处理线程
class ImageBatchWorker : public QObject{
    Q_OBJECT

public:
    ImageBatchWorker(QStringList inputPaths, QString outputDir, EffectType effectType)
    : m_inputPaths(std::move(inputPaths))
    , m_outputDir(std::move(outputDir))
    , m_effectType(effectType){}

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
    EffectType m_effectType;
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
    void startBatch(const QStringList &inputPaths, const QString &outputDir, EffectType effectType);
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
