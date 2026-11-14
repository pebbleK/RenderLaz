import type { EffectKind } from "../lib/bindings/EffectKind";
import type { EffectParams } from "../lib/bindings/EffectParams";
import type { EffectStep } from "../lib/bindings/EffectStep";

export const PARAM_LIMITS = {
  amount: { min: 0, max: 1, step: 0.01 },
  contrast: { min: 0.5, max: 2, step: 0.05 },
  radius: { min: 0.5, max: 64, step: 0.5 },
  iterations: { min: 1, max: 3, step: 1 },
} as const;

export type ParamKey = keyof typeof PARAM_LIMITS;

export const PARAM_LABELS: Record<ParamKey, string> = {
  amount: "强度",
  contrast: "对比度",
  radius: "模糊半径",
  iterations: "模糊迭代",
};

/** 每种特效要露出哪些参数；所有特效都至少有两个。 */
export function paramsFor(kind: EffectKind): ParamKey[] {
  return kind === "blur" ? ["amount", "contrast", "radius", "iterations"] : ["amount", "contrast"];
}

export function clampParam(key: ParamKey, value: number): number {
  const { min, max } = PARAM_LIMITS[key];
  const safe = Number.isFinite(value) ? value : min;
  return Math.min(max, Math.max(min, safe));
}

export function clampParams(params: Partial<EffectParams>): EffectParams {
  return {
    amount: clampParam("amount", params.amount ?? 1),
    contrast: clampParam("contrast", params.contrast ?? 1),
    radius: clampParam("radius", params.radius ?? 8),
    iterations: Math.round(clampParam("iterations", params.iterations ?? 1)),
  };
}

export function defaultParams(kind: EffectKind, blurRadius = 8): EffectParams {
  return clampParams({ radius: blurRadius, iterations: kind === "blur" ? 1 : 1 });
}

export function makeStep(kind: EffectKind, blurRadius = 8): EffectStep {
  return { kind, params: defaultParams(kind, blurRadius) };
}

/**
 * 把会话里读到的旧结构（可能只有 { kind, radius }）补齐成完整参数，
 * 同时丢掉未知的特效类型。
 */
export function normalizeStep(raw: unknown): EffectStep | null {
  if (!raw || typeof raw !== "object") {
    return null;
  }
  const candidate = raw as { kind?: unknown; radius?: unknown; params?: unknown };
  const kinds: EffectKind[] = ["null", "grayscale", "invert", "sepia", "blur"];
  if (typeof candidate.kind !== "string" || !kinds.includes(candidate.kind as EffectKind)) {
    return null;
  }
  const kind = candidate.kind as EffectKind;
  const rawParams = (candidate.params ?? {}) as Partial<EffectParams>;
  const legacyRadius = typeof candidate.radius === "number" ? candidate.radius : undefined;

  return {
    kind,
    params: clampParams({ ...rawParams, radius: rawParams.radius ?? legacyRadius }),
  };
}

export function normalizeSteps(raw: unknown): EffectStep[] {
  if (!Array.isArray(raw)) {
    return [];
  }
  return raw.map(normalizeStep).filter((step): step is EffectStep => step !== null);
}
