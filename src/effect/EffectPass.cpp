#include "EffectPass.h"
#include "NormalEffect.h"

#include <QColor>

bool EffectPass::isCanceled(const std::function<bool()> &shouldCancel){
    return shouldCancel && shouldCancel(); // 检查绑定 || 检查返回值
}
    
void EffectPass::reportProgress(
    const std::function<void(int)> &onProgress,
    int row,
    int totalRows){

    if(!onProgress || totalRows <= 0){
        return;
    }

    // 每20次任务报告一次，最后一次运行报告
    if(row % 20 == 0 || row == totalRows - 1){
        const int progress = (row + 1) * 100 / totalRows;
        onProgress(progress);
    }
}

std::unique_ptr<EffectPass> createEffectPass(EffectType type){
    switch(type){
    case EffectType::Null:
        return std::make_unique<NullEffect>();
    case EffectType::Grayscale:
        return std::make_unique<GrayscaleEffect>();
    case EffectType::Invert:
        return std::make_unique<InvertEffect>();
    case EffectType::Sepia:
        return std::make_unique<SepiaEffect>();
    }
    
    return std::make_unique<NullEffect>();
}