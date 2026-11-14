import { describe, expect, it } from "vitest";

import { makeStep } from "../effects/params";
import { previewChain } from "./preview";

const chain = [makeStep("grayscale"), makeStep("blur", 4)];

describe("previewChain", () => {
  it("没有草稿时原样返回链", () => {
    expect(previewChain(chain, null)).toBe(chain);
  });

  it("草稿追加在链尾", () => {
    const draft = makeStep("invert");
    const result = previewChain(chain, draft);
    expect(result).toHaveLength(3);
    expect(result[2]).toBe(draft);
  });

  it("草稿里调过的参数原样参与预览", () => {
    const draft = makeStep("blur", 12);
    draft.params.amount = 0.5;
    const result = previewChain([], draft);
    expect(result[0].params).toEqual({ amount: 0.5, contrast: 1, radius: 12, iterations: 1 });
  });

  it("不修改原链", () => {
    previewChain(chain, makeStep("sepia"));
    expect(chain).toHaveLength(2);
  });
});
