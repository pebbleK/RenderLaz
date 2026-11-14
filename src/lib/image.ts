/** 图片编解码与路径小工具：给预览、导出、批处理共用。 */

export function baseName(path: string): string {
  const parts = path.split(/[\\/]/);
  return parts[parts.length - 1] || path;
}

export function stemOf(path: string): string {
  const name = baseName(path);
  const dot = name.lastIndexOf(".");
  return dot > 0 ? name.slice(0, dot) : name;
}

export function joinPath(dir: string, name: string): string {
  const separator = dir.includes("\\") ? "\\" : "/";
  return dir.replace(/[\\/]+$/, "") + separator + name;
}

/** 解码文件字节到 ImageData；maxDim 用于预览降采样，导出时传 Infinity。 */
export async function decodeToImageData(bytes: ArrayBuffer, maxDim = Number.POSITIVE_INFINITY): Promise<ImageData> {
  // imageOrientation 默认就是 from-image，这里显式写出来，配合 EXIF 方向要求
  const bitmap = await createImageBitmap(new Blob([bytes]), { imageOrientation: "from-image" });
  try {
    const scale = Math.min(1, maxDim / Math.max(bitmap.width, bitmap.height));
    const width = Math.max(1, Math.round(bitmap.width * scale));
    const height = Math.max(1, Math.round(bitmap.height * scale));

    const canvas = new OffscreenCanvas(width, height);
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("无法创建 2D 画布");
    }
    context.drawImage(bitmap, 0, 0, width, height);
    return context.getImageData(0, 0, width, height);
  } finally {
    bitmap.close();
  }
}

export function imageDataToCanvas(image: ImageData): OffscreenCanvas {
  const canvas = new OffscreenCanvas(image.width, image.height);
  const context = canvas.getContext("2d");
  if (!context) {
    throw new Error("无法创建 2D 画布");
  }
  context.putImageData(image, 0, 0);
  return canvas;
}

export async function encodePng(image: ImageData): Promise<Uint8Array> {
  const blob = await imageDataToCanvas(image).convertToBlob({ type: "image/png" });
  return new Uint8Array(await blob.arrayBuffer());
}

export function toBase64(bytes: Uint8Array): string {
  const chunk = 0x8000;
  let binary = "";
  for (let index = 0; index < bytes.length; index += chunk) {
    binary += String.fromCharCode(...bytes.subarray(index, index + chunk));
  }
  return btoa(binary);
}

export function drawToCanvas(canvas: HTMLCanvasElement, image: ImageData | null): void {
  const context = canvas.getContext("2d");
  if (!context) {
    return;
  }

  if (!image) {
    context.clearRect(0, 0, canvas.width, canvas.height);
    return;
  }

  if (canvas.width !== image.width || canvas.height !== image.height) {
    canvas.width = image.width;
    canvas.height = image.height;
  }
  context.putImageData(image, 0, 0);
}
