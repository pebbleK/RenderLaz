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
    void messageLogged(const QString &message); // 声明的一个信号，不用实现，只需驱动槽函数即可。
};
