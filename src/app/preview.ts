import type { EffectStep } from "../lib/bindings/EffectStep";

/**
 * 下拉框里选中的特效只做临时预览：追加在链尾参与渲染，但不写进真实特效链。
 * 草稿本身带着用户已调好的参数；选「无」或点「添加」后清空。
 */
export function previewChain(
  chain: readonly EffectStep[],
  draft: EffectStep | null,
): EffectStep[] {
  if (!draft) {
    return chain as EffectStep[];
  }
  return [...chain, draft];
}
