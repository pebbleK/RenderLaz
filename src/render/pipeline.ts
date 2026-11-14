import { applyCpu, blendAndContrast, type CpuKind } from "../effects/cpu";
import { clampParams } from "../effects/params";
import type { EffectParams } from "../lib/bindings/EffectParams";
import type { GlRenderer } from "./gl";

export type ChainKind = CpuKind | "blur";

export type ChainStep = {
  kind: ChainKind;
  params: EffectParams;
};

const KNOWN_KINDS: ChainKind[] = ["null", "grayscale", "invert", "sepia", "blur"];

/** 过滤未知类型，把参数补齐并夹到合法区间；顺序保持原样。 */
export function normalizeChain(
  steps: readonly { kind: ChainKind; params?: Partial<EffectParams> }[],
): ChainStep[] {
  return steps
    .filter((step) => KNOWN_KINDS.includes(step.kind))
    .map((step) => ({ kind: step.kind, params: clampParams(step.params ?? {}) }));
}

function blurPass(renderer: GlRenderer, image: ImageData, params: EffectParams): ImageData {
  let current = image;
  const iterations = Math.max(1, Math.round(params.iterations));
  for (let pass = 0; pass < iterations; pass += 1) {
    current = renderer.blur(current, params.radius);
  }
  return current;
}

/**
 * 执行整条特效链：每个特效先算出自身结果，再与上一步画面按「强度」混合、
 * 按「对比度」调整。预览与批处理都调用这里，保证两者结果一致。
 */
export function runChain(
  source: ImageData,
  steps: readonly { kind: ChainKind; params?: Partial<EffectParams> }[],
  renderer: GlRenderer,
  onProgress?: (ratio: number) => void,
): ImageData {
  const chain = normalizeChain(steps);
  let current = source;

  chain.forEach((step, index) => {
    const before = current;
    const effect =
      step.kind === "blur" ? blurPass(renderer, current, step.params) : applyCpu(current, step.kind);
    current = blendAndContrast(before, effect, step.params.amount, step.params.contrast);
    onProgress?.((index + 1) / chain.length);
  });

  return current;
}
