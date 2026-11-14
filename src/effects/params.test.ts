import { describe, expect, it } from "vitest";

import {
  clampParam,
  defaultParams,
  makeStep,
  normalizeStep,
  normalizeSteps,
  paramsFor,
} from "./params";

describe("paramsFor", () => {
  it("CPU 特效露出两个参数", () => {
    for (const kind of ["null", "grayscale", "invert", "sepia"] as const) {
      expect(paramsFor(kind)).toHaveLength(2);
    }
  });

  it("模糊露出四个参数", () => {
    expect(paramsFor("blur")).toEqual(["amount", "contrast", "radius", "iterations"]);
  });
});

describe("clampParam", () => {
  it("夹到合法区间", () => {
    expect(clampParam("amount", -1)).toBe(0);
    expect(clampParam("amount", 2)).toBe(1);
    expect(clampParam("contrast", 0)).toBe(0.5);
    expect(clampParam("radius", 1000)).toBe(64);
    expect(clampParam("iterations", 9)).toBe(3);
  });

  it("非法数值回落到下界", () => {
    expect(clampParam("contrast", Number.NaN)).toBe(0.5);
  });
});

describe("defaultParams / makeStep", () => {
  it("默认强度与对比度都是 100%", () => {
    expect(defaultParams("grayscale")).toEqual({ amount: 1, contrast: 1, radius: 8, iterations: 1 });
  });

  it("模糊沿用传入的默认半径", () => {
    expect(makeStep("blur", 20).params.radius).toBe(20);
  });
});

describe("normalizeStep", () => {
  it("兼容只有 radius 的旧结构", () => {
    const step = normalizeStep({ kind: "blur", radius: 5 });
    expect(step?.kind).toBe("blur");
    expect(step?.params.radius).toBe(5);
    expect(step?.params.amount).toBe(1);
  });

  it("丢弃未知类型与非法输入", () => {
    expect(normalizeStep({ kind: "sharpen" })).toBeNull();
    expect(normalizeStep(null)).toBeNull();
    expect(normalizeStep("grayscale")).toBeNull();
  });

  it("normalizeSteps 过滤无效项并保留顺序", () => {
    const steps = normalizeSteps([{ kind: "invert" }, { kind: "bogus" }, { kind: "sepia" }]);
    expect(steps.map((step) => step.kind)).toEqual(["invert", "sepia"]);
  });
});
