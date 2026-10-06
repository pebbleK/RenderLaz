/**
 * @file Logger.h
 * @brief 日志输出
 * @details 提供 info、warning、error 三个级别的日志接口，统一加上时间戳和级别
 *          前缀后，通过 messageLogged 信号发出，由界面层订阅并显示到日志面板。
 */
#pragma once

#include <QObject>
#include <QString>

class Logger : public QObject{
    Q_OBJECT

public:
    explicit Logger(QObject *parent = nullptr);

    void info(const QString &message);
    void warning(const QString &message);
    void error(const QString &message);

signals:
    // logger通过signal传递到slot，把日志显示在UI上。
    void messageLogged(const QString &message);
};
