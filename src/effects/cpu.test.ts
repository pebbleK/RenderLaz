import { describe, expect, it } from "vitest";

import { blendPixels, transformPixels } from "./cpu";

const pixel = (r: number, g: number, b: number, a = 255) =>
  new Uint8ClampedArray([r, g, b, a]);

describe("transformPixels", () => {
  it("灰度使用 BT.601 权重", () => {
    const [r, g, b, a] = transformPixels(pixel(255, 0, 0), "grayscale");
    expect([r, g, b, a]).toEqual([76, 76, 76, 255]);
  });

  it("反色翻转三个通道并保留 alpha", () => {
    const [r, g, b, a] = transformPixels(pixel(10, 20, 30, 128), "invert");
    expect([r, g, b, a]).toEqual([245, 235, 225, 128]);
  });

  it("棕褐把白色映射为暖色且不越界", () => {
    const [r, g, b] = transformPixels(pixel(255, 255, 255), "sepia");
    expect(r).toBe(255);
    expect(g).toBeGreaterThan(230);
    expect(b).toBeLessThan(240);
  });

  it("null 返回等值副本且不改动入参", () => {
    const source = pixel(1, 2, 3, 4);
    const result = transformPixels(source, "null");
    expect(Array.from(result)).toEqual([1, 2, 3, 4]);
    expect(result).not.toBe(source);
  });

  it("多像素保持长度与 alpha 不变", () => {
    const source = new Uint8ClampedArray([0, 0, 0, 200, 255, 255, 255, 100]);
    const result = transformPixels(source, "grayscale");
    expect(result.length).toBe(source.length);
    expect(result[3]).toBe(200);
    expect(result[7]).toBe(100);
  });
});

describe("blendPixels", () => {
  const base = pixel(0, 100, 200, 255);
  const effect = pixel(200, 100, 0, 128);

  it("强度 0 时等于原画面（对比度 100%）", () => {
    expect(Array.from(blendPixels(base, effect, 0, 1))).toEqual([0, 100, 200, 255]);
  });

  it("强度 1 时完全采用特效结果，alpha 取特效的", () => {
    const [r, g, b, a] = blendPixels(base, effect, 1, 1);
    expect([r, g, b, a]).toEqual([200, 100, 0, 128]);
  });

  it("强度 0.5 时两值各占一半", () => {
    const [r, g, b] = blendPixels(base, effect, 0.5, 1);
    expect([r, g, b]).toEqual([100, 100, 100]);
  });

  it("对比度 200% 把像素推离中灰", () => {
    const [r] = blendPixels(pixel(128, 128, 128), pixel(160, 0, 0), 1, 2);
    expect(r).toBe(192);
  });
});
