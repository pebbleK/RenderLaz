import type { ItemStatus } from "./bindings/ItemStatus";

export type BatchItemResult = {
  path: string;
  output_path: string | null;
  status: ItemStatus;
  error: string | null;
};

export type BatchOutcome = {
  items: BatchItemResult[];
  okCount: number;
  failCount: number;
  canceled: boolean;
};

export type BatchOptions = {
  files: string[];
  outputDir: string;
  /** 处理单张图片；抛错即记为失败，不影响后续图片 */
  process: (path: string, outputName: string) => Promise<void>;
  shouldCancel: () => boolean;
  onProgress?: (done: number, total: number) => void;
};

/** 同名文件追加 -2、-3…，避免一次批处理里互相覆盖。 */
export function uniqueName(used: Set<string>, candidate: string): string {
  let name = candidate;
  let counter = 2;
  while (used.has(name)) {
    name = candidate.replace(/\.png$/i, `-${counter}.png`);
    counter += 1;
  }
  used.add(name);
  return name;
}

/**
 * 顺序执行批处理。取消后剩余图片标记为 skipped，当前这张不会落半成品，
 * 因为 process 由调用方保证「先写临时文件再改名」。
 */
export async function runBatch(options: BatchOptions): Promise<BatchOutcome> {
  const { files, outputDir, process, shouldCancel, onProgress } = options;
  const items: BatchItemResult[] = [];
  const used = new Set<string>();
  let okCount = 0;
  let failCount = 0;

  for (let index = 0; index < files.length; index += 1) {
    const path = files[index];

    if (shouldCancel()) {
      items.push({ path, output_path: null, status: "skipped", error: "已取消" });
      onProgress?.(index + 1, files.length);
      continue;
    }

    try {
      const outputName = uniqueName(used, candidateName(path));
      await process(path, outputName);
      items.push({ path, output_path: joinOutput(outputDir, outputName), status: "ok", error: null });
      okCount += 1;
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      items.push({ path, output_path: null, status: "failed", error: message });
      failCount += 1;
    }

    onProgress?.(index + 1, files.length);
    // 让出事件循环，界面能刷新进度、响应取消
    await new Promise((resolve) => setTimeout(resolve, 0));
  }

  return { items, okCount, failCount, canceled: shouldCancel() };
}

export function candidateName(path: string): string {
  const base = path.split(/[\\/]/).pop() ?? path;
  const dot = base.lastIndexOf(".");
  const stem = dot > 0 ? base.slice(0, dot) : base;
  return `${stem}-out.png`;
}

export function joinOutput(dir: string, name: string): string {
  const separator = dir.includes("\\") ? "\\" : "/";
  return `${dir.replace(/[\\/]+$/, "")}${separator}${name}`;
}
