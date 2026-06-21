#include "ProjectConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool ProjectConfig::saveToFile(const QString &filePath, const ProjectState &state, QString *errorMessage){
    // ProjectConfig -> QJsonObject -> QJsonDocument -> Config.Json
    QJsonObject root;
    root["version"] = 1;
    root["currentImagePath"] = state.currentImagePath;
    root["outputDir"] = state.outputDir;

    QJsonArray resources;
    for(const QString path : state.resourcePaths){
        resources.append(path);
    }
    root["resources"] = resources;

    QJsonArray effectCahin;
    for(EffectType effect : state.effectTypes){
        effectCahin.append(effectTypeToString(effect));
    }
    root["effectChain"] = effectCahin;

    QFile file(filePath); // QIODevice::Truncate模式：清空内容
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        if(errorMessage){
            *errorMessage = "无法保存工程：" + file.errorString();
        }
        return false;
    }

    // toJson(QJsonDocument::Indented)转换为带缩进可读Json文本
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ProjectConfig::loadFromFile(const QString &filePath, ProjectState *state, QString *errorMessage){
    if(!state){
        if(errorMessage){
            *errorMessage = "工程状态为空";
        }
        return false;
    }

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        if(errorMessage){
            *errorMessage = "无法打开工程：" + file.errorString();
        }
        return false;
    }

    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);

    // isObject() 根元素是否为{}的对象
    if(parseError.error != QJsonParseError::NoError || !document.isObject()){
        if(errorMessage){
            *errorMessage = "工程文件格式错误：" + parseError.errorString();
        }
        return false;
    }

    const QJsonObject root = document.object();

    ProjectState loadState;
    loadState.currentImagePath = root.value("currentImagePath").toString();
    loadState.outputDir = root.value("outputDir").toString();

    const QJsonArray resources = root.value("resources").toArray();
    for(const QJsonValue &value : resources){
        const QString path = value.toString();
        if(!path.isEmpty()){
            loadState.resourcePaths.append(path);
        }
    }

    const QJsonArray effectChain = root.value("effectChain").toArray();
    for(const QJsonValue &value : effectChain){
        loadState.effectTypes.append(ProjectConfig::effectTypeFromString(value.toString()));
    }

    *state = loadState;
    return true;
}

QString ProjectConfig::effectTypeToString(const EffectType type){
    switch(type){
    case EffectType::Null:
        return "Null";
    case EffectType::Grayscale:
        return "Grayscale";
    case EffectType::Invert:
        return "Invert";
    case EffectType::Sepia:
        return "Sepia";
    case EffectType::Blur:
        return "Blur";
    }

    return "Null";
}

EffectType ProjectConfig::effectTypeFromString(const QString &value){
    if(value == "Grayscale"){
        return EffectType::Grayscale;
    }

    if(value == "Invert"){
        return EffectType::Invert;
    }

    if(value == "Sepia"){
        return EffectType::Sepia;
    }

    if(value == "Blur"){
        return EffectType::Blur;
    }

    return EffectType::Null;
}