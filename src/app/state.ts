import type { EffectParams } from "../lib/bindings/EffectParams";
import type { EffectStep } from "../lib/bindings/EffectStep";
import type { RunStatus } from "../lib/bindings/RunStatus";
import type { AppSettings, BatchRun } from "../lib/ipc";
import { normalizeSteps } from "../effects/params";
import type { LogEntry } from "../lib/log";

export type PreviewState = {
  image: ImageData | null;
  error: string | null;
};

export type AppState = {
  settings: AppSettings;
  resources: string[];
  missing: string[];
  currentImage: string | null;
  chain: EffectStep[];
  preview: PreviewState;
  logs: LogEntry[];
  previewing: boolean;
  history: BatchRun[];
  dbWarning: string | null;
};

export const emptySettings: AppSettings = {
  output_dir: null,
  blur_radius: 8,
  window_width: null,
  window_height: null,
  session_state: null,
};

export const initialState: AppState = {
  settings: emptySettings,
  resources: [],
  missing: [],
  currentImage: null,
  chain: [],
  preview: { image: null, error: null },
  logs: [],
  previewing: false,
  history: [],
  dbWarning: null,
};

export type Action =
  | { type: "settings"; settings: AppSettings }
  | { type: "resources/add"; paths: string[] }
  | { type: "resources/replace"; paths: string[] }
  | { type: "resources/missing"; paths: string[] }
  | { type: "resources/remove"; path: string }
  | { type: "current"; path: string | null }
  | { type: "chain/add"; step: EffectStep }
  | { type: "chain/remove"; index: number }
  | { type: "chain/clear" }
  | { type: "chain/set"; steps: EffectStep[] }
  | { type: "chain/params"; index: number; patch: Partial<EffectParams> }
  | { type: "preview"; preview: PreviewState }
  | { type: "previewing"; value: boolean }
  | { type: "log"; entry: LogEntry }
  | { type: "logs/clear" }
  | { type: "history"; history: BatchRun[] }
  | { type: "db-warning"; message: string };

export function reducer(state: AppState, action: Action): AppState {
  switch (action.type) {
    case "settings":
      return { ...state, settings: action.settings };

    case "resources/replace":
      return {
        ...state,
        resources: action.paths,
        missing: [],
        currentImage: action.paths[0] ?? null,
      };

    case "resources/add": {
      const seen = new Set(state.resources);
      const added = action.paths.filter((path) => !seen.has(path));
      if (added.length === 0) {
        return state;
      }
      return {
        ...state,
        resources: [...state.resources, ...added],
        currentImage: state.currentImage ?? added[0],
      };
    }

    case "resources/missing":
      return { ...state, missing: Array.from(new Set([...state.missing, ...action.paths])) };

    case "resources/remove":
      return {
        ...state,
        resources: state.resources.filter((path) => path !== action.path),
        missing: state.missing.filter((path) => path !== action.path),
        currentImage: state.currentImage === action.path ? null : state.currentImage,
      };

    case "current":
      return { ...state, currentImage: action.path };

    case "chain/add":
      return { ...state, chain: [...state.chain, action.step] };

    case "chain/remove":
      return { ...state, chain: state.chain.filter((_, index) => index !== action.index) };

    case "chain/clear":
      return { ...state, chain: [] };

    case "chain/set":
      return { ...state, chain: action.steps };

    case "chain/params":
      return {
        ...state,
        chain: state.chain.map((step, index) =>
          index === action.index ? { ...step, params: { ...step.params, ...action.patch } } : step,
        ),
      };

    case "preview":
      return { ...state, preview: action.preview };

    case "previewing":
      return { ...state, previewing: action.value };

    case "log":
      return { ...state, logs: [...state.logs, action.entry].slice(-500) };

    case "logs/clear":
      return { ...state, logs: [] };

    case "history":
      return { ...state, history: action.history };

    case "db-warning":
      return { ...state, dbWarning: action.message };
  }
}

/** 会话恢复用的最小快照。 */
export function sessionSnapshot(state: AppState) {
  return JSON.stringify({
    resources: state.resources,
    effects: state.chain,
    currentImage: state.currentImage,
  });
}

export function readSession(sessionState: string | null): {
  resources: string[];
  effects: EffectStep[];
  currentImage: string | null;
} | null {
  if (!sessionState) {
    return null;
  }
  try {
    const parsed = JSON.parse(sessionState) as {
      resources?: unknown;
      effects?: unknown;
      currentImage?: unknown;
    };
    const resources = Array.isArray(parsed.resources)
      ? parsed.resources.filter((value): value is string => typeof value === "string")
      : [];
    const effects = normalizeSteps(parsed.effects);
    const currentImage = typeof parsed.currentImage === "string" ? parsed.currentImage : null;
    return { resources, effects, currentImage: currentImage ?? resources[0] ?? null };
  } catch {
    return null;
  }
}

export const runStatusLabel: Record<RunStatus, string> = {
  running: "进行中",
  completed: "已完成",
  canceled: "已取消",
  failed: "失败",
};
