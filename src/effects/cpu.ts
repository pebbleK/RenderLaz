export type CpuKind = "null" | "grayscale" | "invert" | "sepia";

/**
 * 纯像素变换：输入 RGBA 字节数组，输出新的数组，不修改入参。
 * 与旧 Qt 版本保持同一套公式（灰度用 BT.601 权重，棕褐用经典矩阵）。
 */
export function transformPixels(
  source: Uint8ClampedArray<ArrayBufferLike>,
  kind: CpuKind,
): Uint8ClampedArray<ArrayBuffer> {
  if (kind === "null") {
    return new Uint8ClampedArray(source);
  }

  const target = new Uint8ClampedArray(new ArrayBuffer(source.length));

  for (let index = 0; index < source.length; index += 4) {
    const r = source[index];
    const g = source[index + 1];
    const b = source[index + 2];
    const a = source[index + 3];

    switch (kind) {
      case "grayscale": {
        const value = 0.299 * r + 0.587 * g + 0.114 * b;
        target[index] = value;
        target[index + 1] = value;
        target[index + 2] = value;
        break;
      }
      case "invert": {
        target[index] = 255 - r;
        target[index + 1] = 255 - g;
        target[index + 2] = 255 - b;
        break;
      }
      case "sepia": {
        target[index] = 0.393 * r + 0.769 * g + 0.189 * b;
        target[index + 1] = 0.349 * r + 0.686 * g + 0.168 * b;
        target[index + 2] = 0.272 * r + 0.534 * g + 0.131 * b;
        break;
      }
    }

    target[index + 3] = a;
  }

  return target;
}

/** CPU 特效：输入输出都是 ImageData。 */
export function applyCpu(image: ImageData, kind: CpuKind): ImageData {
  if (kind === "null") {
    return image;
  }
  return new ImageData(transformPixels(image.data, kind), image.width, image.height);
}

/**
 * 把特效结果与它之前的画面按 amount 混合，再按 contrast 调整对比度。
 * amount=1 且 contrast=1 时原样返回特效结果。
 */
export function blendPixels(
  base: Uint8ClampedArray<ArrayBufferLike>,
  effect: Uint8ClampedArray<ArrayBufferLike>,
  amount: number,
  contrast: number,
): Uint8ClampedArray<ArrayBuffer> {
  const alpha = Math.min(1, Math.max(0, amount));
  const gain = Math.min(2, Math.max(0.5, contrast));
  const target = new Uint8ClampedArray(new ArrayBuffer(effect.length));

  for (let index = 0; index < effect.length; index += 4) {
    for (let channel = 0; channel < 3; channel += 1) {
      const mixed = base[index + channel] + (effect[index + channel] - base[index + channel]) * alpha;
      target[index + channel] = (mixed - 128) * gain + 128;
    }
    // alpha 只参与强度混合，不受对比度影响
    target[index + 3] = base[index + 3] + (effect[index + 3] - base[index + 3]) * alpha;
  }

  return target;
}

export function blendAndContrast(
  base: ImageData,
  effect: ImageData,
  amount: number,
  contrast: number,
): ImageData {
  if (amount >= 1 && Math.abs(contrast - 1) < 1e-6) {
    return effect;
  }
  return new ImageData(blendPixels(base.data, effect.data, amount, contrast), effect.width, effect.height);
}
