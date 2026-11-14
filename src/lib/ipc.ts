import { invoke } from "@tauri-apps/api/core";

import type { AppSettings } from "./bindings/AppSettings";
import type { BatchRun } from "./bindings/BatchRun";
import type { BatchRunResult } from "./bindings/BatchRunResult";
import type { DatabaseStatus } from "./bindings/DatabaseStatus";
import type { ImportOutcome } from "./bindings/ImportOutcome";

export type { AppSettings, BatchRun, BatchRunResult, DatabaseStatus, ImportOutcome };

/** Rust 侧 ApiError 的镜像；命令失败时 invoke 抛出的是这个对象。 */
export type ApiError = {
  code: string;
  message: string;
  context?: string | null;
};

export class CommandError extends Error {
  readonly code: string;
  readonly context?: string | null;

  constructor(error: ApiError) {
    super(error.message);
    this.name = "CommandError";
    this.code = error.code;
    this.context = error.context ?? null;
  }
}

function toCommandError(raw: unknown): CommandError {
  if (raw && typeof raw === "object" && "code" in raw && "message" in raw) {
    const candidate = raw as ApiError;
    return new CommandError(candidate);
  }
  const message = raw instanceof Error ? raw.message : String(raw);
  return new CommandError({ code: "E_UNKNOWN", message });
}

async function call<T>(command: string, args?: Record<string, unknown>): Promise<T> {
  try {
    return await invoke<T>(command, args);
  } catch (error) {
    throw toCommandError(error);
  }
}

export const ipc = {
  importImages: (paths: string[]) => call<ImportOutcome[]>("import_images", { paths }),

  /** 返回原始文件字节，前端用 Blob + createImageBitmap 解码。 */
  readImage: (path: string) => call<ArrayBuffer>("read_image", { path }),

  ensureDir: (path: string) => call<void>("ensure_dir", { path }),

  writePng: (path: string, pngBase64: string) => call<void>("write_png", { path, pngBase64 }),

  databaseStatus: () => call<DatabaseStatus>("database_status"),

  getSettings: () => call<AppSettings>("get_settings"),
  saveSettings: (settings: AppSettings) => call<void>("save_settings", { settings }),

  startBatchRun: (inputCount: number, outputDir: string | null) =>
    call<number>("start_batch_run", { inputCount, outputDir }),
  finishBatchRun: (result: BatchRunResult) => call<void>("finish_batch_run", { result }),
  listBatchRuns: (limit: number) => call<BatchRun[]>("list_batch_runs", { limit }),
};
