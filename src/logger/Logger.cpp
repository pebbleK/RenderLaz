#include "Logger.h"

#include <QDateTime>

Logger::Logger(QObject *parent) : QObject(parent){}

void Logger::info(const QString &message){
    emit messageLogged("[Info]" + QDateTime::currentDateTime().toString("HH:mm:ss") + " " + message);
}

void Logger::warning(const QString &message){
    emit messageLogged("[Warning]" + QDateTime::currentDateTime().toString("HH:mm:ss") + " " + message);
}

void Logger::error(const QString &message){
    emit messageLogged("[Error]" + QDateTime::currentDateTime().toString("HH:mm:ss") + " " + message);
}