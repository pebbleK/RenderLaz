#include "EffectChain.h"

QImage EffectChain::apply(
    const QImage &image,
    const std::function<bool()> &shouldCancel,
    const std::function<void(int)> &onProgress
    ) const{
    if(image.isNull()){
        return QImage();
    }

    if(m_effectTypes.isEmpty()){
        if(onProgress){
            onProgress(100);
        }
        return image;
    }

    QImage current = image;

    for(int i = 0; i < m_effectTypes.size(); ++i){
        if(shouldCancel && shouldCancel()){
            return QImage();
        }

        const auto pass = createEffectPass(m_effectTypes.at(i));

        current = pass->apply(current, shouldCancel, 
        [onProgress, i, total = m_effectTypes.size()](int passProgress){
            if(onProgress){
                const int overall = (i * 100 + passProgress) / total;
                onProgress(overall);
            }
        }); // total初始化捕获，C++14的新特性

        if(current.isNull()){
            return QImage();
        }
    }

    if(onProgress){
        onProgress(100);
    }

    return current;
}

void EffectChain::addPass(EffectType type){
    m_effectTypes.push_back(type);
}

void EffectChain::removePass(int index){
    if(index < 0 || index >= m_effectTypes.size()){
        return;
    }

    m_effectTypes.removeAt(index);
}

void EffectChain::clear(){
    m_effectTypes.clear();
}

bool EffectChain::isEmpty() const{
    return m_effectTypes.isEmpty();
}

int EffectChain::size() const{
    return m_effectTypes.size();
}

QList<EffectType> EffectChain::effectTypes() const{
    return m_effectTypes;
}

QStringList EffectChain::passNames() const{
    QStringList names;

    for(EffectType type : m_effectTypes){
        const auto pass = createEffectPass(type);
        names.push_back(pass->name());
    }

    return names;
}