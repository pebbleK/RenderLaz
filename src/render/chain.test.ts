import { describe, expect, it } from "vitest";

import { normalizeChain } from "./pipeline";

describe("normalizeChain", () => {
  it("保留顺序并丢弃未知类型", () => {
    const steps = [{ kind: "grayscale" }, { kind: "nope" }, { kind: "invert" }] as never;
    expect(normalizeChain(steps).map((step) => step.kind)).toEqual(["grayscale", "invert"]);
  });

  it("缺省参数补齐为 强度 100% / 对比度 100%", () => {
    const [step] = normalizeChain([{ kind: "grayscale" }]);
    expect(step.params).toEqual({ amount: 1, contrast: 1, radius: 8, iterations: 1 });
  });

  it("越界参数被夹到合法区间", () => {
    const [step] = normalizeChain([
      { kind: "blur", params: { radius: 999, contrast: 5, amount: -1, iterations: 9 } },
    ]);
    expect(step.params).toEqual({ amount: 0, contrast: 2, radius: 64, iterations: 3 });
  });
});
