import { useCallback, useEffect, useReducer, useRef, useState } from "react";
import { PhysicalSize } from "@tauri-apps/api/dpi";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { open, save } from "@tauri-apps/plugin-dialog";

import type { EffectKind } from "../lib/bindings/EffectKind";
import type { EffectParams } from "../lib/bindings/EffectParams";
import type { EffectStep } from "../lib/bindings/EffectStep";
import { makeStep } from "../effects/params";
import { runBatch } from "../lib/batch";
import { baseName, decodeToImageData, encodePng, joinPath, stemOf, toBase64 } from "../lib/image";
import { CommandError, ipc } from "../lib/ipc";
import { logger, subscribeLog } from "../lib/log";
import { createGlRenderer, GlUnavailableError, type GlRenderer } from "../render/gl";
import { runChain } from "../render/pipeline";
import { initialState, readSession, reducer, sessionSnapshot } from "./state";
import { previewChain } from "./preview";
import {
  DEFAULT_EDITOR_TAB,
  DEFAULT_RESOURCE_TAB,
  EDITOR_TABS,
  RESOURCE_TABS,
  resolveBatchFiles,
  type EditorTabKey,
  type ResourceTabKey,
} from "./rail";
import { BatchPanel } from "../ui/BatchPanel";
import { EffectChainPanel, effectLabels } from "../ui/EffectChainPanel";
import { LogPanel } from "../ui/LogPanel";
import { PreviewCanvas } from "../ui/PreviewCanvas";
import { ResourcePanel } from "../ui/ResourcePanel";
import { TabBar } from "../ui/TabBar";

const PREVIEW_MAX_DIM = 2048;
const IMAGE_FILTERS = [{ name: "图片", extensions: ["png", "jpg", "jpeg", "bmp", "webp"] }];

function messageOf(error: unknown): string {
  if (error instanceof CommandError) {
    return error.context ? `${error.message}（${error.context}）` : error.message;
  }
  return error instanceof Error ? error.message : String(error);
}

export default function App() {
  const [state, dispatch] = useReducer(reducer, initialState);
  const rendererRef = useRef<GlRenderer | null>(null);
  // 预览用源图缓存：换特效时不必重新解码原图，保证下拉框切换是即时的
  const sourceCacheRef = useRef<{ path: string; image: ImageData } | null>(null);

  const [resourceTab, setResourceTab] = useState<ResourceTabKey>(DEFAULT_RESOURCE_TAB);
  const [editorTab, setEditorTab] = useState<EditorTabKey>(DEFAULT_EDITOR_TAB);

  const [draft, setDraft] = useState<EffectStep | null>(null);
  const [editingIndex, setEditingIndex] = useState<number | null>(null);
  const [pickedFiles, setPickedFiles] = useState<string[]>([]);
  const [batchOutput, setBatchOutput] = useState<string | null>(null);
  const [batchProgress, setBatchProgress] = useState<{ done: number; total: number } | null>(null);
  const [batchRunning, setBatchRunning] = useState(false);
  const cancelRef = useRef(false);

  const refreshHistory = useCallback(async () => {
    try {
      dispatch({ type: "history", history: await ipc.listBatchRuns(20) });
    } catch (error) {
      logger.warn(`读取历史失败：${messageOf(error)}`);
    }
  }, []);

  // 渲染器只建一次；WebGL2 不可用时直接报错，不静默降级
  useEffect(() => {
    try {
      rendererRef.current = createGlRenderer();
    } catch (error) {
      const message =
        error instanceof GlUnavailableError ? error.message : `GPU 初始化失败：${messageOf(error)}`;
      logger.error(message);
      dispatch({ type: "preview", preview: { image: null, error: message } });
    }
    return () => {
      rendererRef.current?.dispose();
      rendererRef.current = null;
    };
  }, []);

  useEffect(() => subscribeLog((entry) => dispatch({ type: "log", entry })), []);

  // 启动：读设置、恢复会话、拉工程与历史
  useEffect(() => {
    let cancelled = false;
    void (async () => {
      try {
        const settings = await ipc.getSettings();
        if (cancelled) return;
        dispatch({ type: "settings", settings });
        setBatchOutput(settings.output_dir);

        if (settings.window_width && settings.window_height) {
          try {
            await getCurrentWindow().setSize(
              new PhysicalSize(settings.window_width, settings.window_height),
            );
          } catch (error) {
            logger.warn(`恢复窗口尺寸失败：${messageOf(error)}`);
          }
        }

        try {
          const status = await ipc.databaseStatus();
          if (status.warning) {
            dispatch({ type: "db-warning", message: status.warning });
            logger.warn(status.warning);
          }
        } catch {
          // 状态读不到不影响启动
        }

        const session = readSession(settings.session_state);
        if (session) {
          dispatch({ type: "resources/add", paths: session.resources });
          dispatch({ type: "chain/set", steps: session.effects });
          if (session.currentImage) {
            dispatch({ type: "current", path: session.currentImage });
          }
        }
        await refreshHistory();
      } catch (error) {
        logger.error(`初始化失败：${messageOf(error)}`);
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [refreshHistory]);

  // 设置与会话防抖落库
  const persistedOnce = useRef(false);
  useEffect(() => {
    if (!persistedOnce.current) {
      persistedOnce.current = true;
      return;
    }
    const timer = window.setTimeout(() => {
      void (async () => {
        try {
          const size = await getCurrentWindow().innerSize();
          await ipc.saveSettings({
            ...state.settings,
            window_width: size.width,
            window_height: size.height,
            session_state: sessionSnapshot(state),
          });
        } catch (error) {
          logger.warn(`设置保存失败：${messageOf(error)}`);
        }
      })();
    }, 400);
    return () => window.clearTimeout(timer);
  }, [state.resources, state.chain, state.currentImage, state.settings]);

  // 预览：当前图片 + 特效链
  useEffect(() => {
    const path = state.currentImage;
    const renderer = rendererRef.current;
    if (!path) {
      dispatch({ type: "preview", preview: { image: null, error: null } });
      return;
    }
    if (!renderer) {
      return;
    }

    let cancelled = false;
    dispatch({ type: "previewing", value: true });

    void (async () => {
      try {
        let source: ImageData;
        if (sourceCacheRef.current?.path === path) {
          source = sourceCacheRef.current.image;
        } else {
          const bytes = await ipc.readImage(path);
          source = await decodeToImageData(bytes, PREVIEW_MAX_DIM);
          sourceCacheRef.current = { path, image: source };
        }
        if (cancelled) return;
        const chain = previewChain(state.chain, draft);
        const result = runChain(source, chain, renderer);
        if (cancelled) return;
        dispatch({ type: "preview", preview: { image: result, error: null } });
      } catch (error) {
        if (cancelled) return;
        const message = messageOf(error);
        dispatch({ type: "preview", preview: { image: null, error: message } });
        dispatch({ type: "resources/missing", paths: [path] });
        logger.error(`预览失败：${message}`);
      } finally {
        if (!cancelled) {
          dispatch({ type: "previewing", value: false });
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [state.currentImage, state.chain, draft]);

  // 离开特效链页签时结束临时预览，避免预览与真实链长期不一致
  useEffect(() => {
    if (editorTab !== "chain") {
      setDraft(null);
    }
  }, [editorTab]);

  const handleImport = useCallback(async () => {
    const selection = await open({ multiple: true, filters: IMAGE_FILTERS });
    const paths = Array.isArray(selection) ? selection : selection ? [selection] : [];
    if (paths.length === 0) {
      return;
    }

    try {
      const outcomes = await ipc.importImages(paths);
      const okPaths = outcomes.filter((item) => item.ok).map((item) => item.path);
      const failed = outcomes.filter((item) => !item.ok);

      dispatch({ type: "resources/add", paths: okPaths });
      if (failed.length > 0) {
        dispatch({ type: "resources/missing", paths: failed.map((item) => item.path) });
        failed.forEach((item) => logger.warn(`导入失败 ${baseName(item.path)}：${item.error ?? "未知原因"}`));
      }
      if (okPaths.length > 0) {
        logger.success(`导入了 ${okPaths.length} 张图片`);
      }
    } catch (error) {
      logger.error(`导入失败：${messageOf(error)}`);
    }
  }, []);

  const handleChoose = useCallback(
    (kind: EffectKind | null) => {
      setEditingIndex(null);
      setDraft(kind ? makeStep(kind, state.settings.blur_radius) : null);
    },
    [state.settings.blur_radius],
  );

  const handleParamsChange = useCallback(
    (patch: Partial<EffectParams>) => {
      if (draft) {
        setDraft({ ...draft, params: { ...draft.params, ...patch } });
        return;
      }
      if (editingIndex !== null) {
        dispatch({ type: "chain/params", index: editingIndex, patch });
      }
    },
    [draft, editingIndex],
  );

  const handleAddEffect = useCallback(() => {
    if (!draft) {
      return;
    }
    dispatch({ type: "chain/add", step: draft });
    setEditingIndex(state.chain.length);
    setDraft(null);
    logger.info(`添加特效：${effectLabels[draft.kind]}`);
  }, [draft, state.chain.length]);

  const handleExport = useCallback(async () => {
    const path = state.currentImage;
    const renderer = rendererRef.current;
    if (!path || !renderer) {
      return;
    }

    try {
      const target = await save({
        defaultPath: `${stemOf(path)}-out.png`,
        filters: [{ name: "PNG", extensions: ["png"] }],
      });
      if (!target) {
        return;
      }

      const bytes = await ipc.readImage(path);
      const source = await decodeToImageData(bytes);
      const result = runChain(source, state.chain, renderer);
      const png = await encodePng(result);
      await ipc.writePng(target, toBase64(png));
      logger.success(`已导出：${target}`);
    } catch (error) {
      logger.error(`导出失败：${messageOf(error)}`);
    }
  }, [state.chain, state.currentImage]);

  const handlePickBatchFiles = useCallback(async () => {
    const selection = await open({ multiple: true, filters: IMAGE_FILTERS });
    const paths = Array.isArray(selection) ? selection : selection ? [selection] : [];
    if (paths.length > 0) {
      setPickedFiles(paths);
    }
  }, []);

  const handlePickBatchOutput = useCallback(async () => {
    const selection = await open({ directory: true, multiple: false });
    if (typeof selection === "string") {
      setBatchOutput(selection);
      dispatch({ type: "settings", settings: { ...state.settings, output_dir: selection } });
    }
  }, [state.settings]);

  const handleStartBatch = useCallback(async () => {
    const renderer = rendererRef.current;
    const files = resolveBatchFiles(pickedFiles, state.resources);
    if (!renderer || !batchOutput || files.length === 0) {
      return;
    }

    cancelRef.current = false;
    setBatchRunning(true);
    setBatchProgress({ done: 0, total: files.length });

    try {
      await ipc.ensureDir(batchOutput);
      const runId = await ipc.startBatchRun(files.length, batchOutput);

      const outcome = await runBatch({
        files: files,
        outputDir: batchOutput,
        shouldCancel: () => cancelRef.current,
        onProgress: (done, total) => setBatchProgress({ done, total }),
        process: async (path, outputName) => {
          const bytes = await ipc.readImage(path);
          const source = await decodeToImageData(bytes);
          // 批处理只认真实特效链，下拉框里的临时预览不参与导出
          const result = runChain(source, state.chain, renderer);
          await ipc.writePng(joinPath(batchOutput, outputName), toBase64(await encodePng(result)));
        },
      });

      if (outcome.failCount > 0) {
        outcome.items
          .filter((item) => item.status === "failed")
          .forEach((item) => logger.warn(`处理失败 ${baseName(item.path)}：${item.error ?? "未知原因"}`));
      }

      await ipc.finishBatchRun({
        id: runId,
        status: outcome.canceled ? "canceled" : "completed",
        ok_count: outcome.okCount,
        fail_count: outcome.failCount,
        items: outcome.items,
      });

      logger.success(
        `批处理结束：成功 ${outcome.okCount}，失败 ${outcome.failCount}${outcome.canceled ? "（已取消）" : ""}`,
      );
      setEditorTab("log");
      await refreshHistory();
    } catch (error) {
      logger.error(`批处理失败：${messageOf(error)}`);
    } finally {
      setBatchRunning(false);
    }
  }, [batchOutput, pickedFiles, refreshHistory, state.chain, state.resources]);


  return (
    <div className="app">
      <header className="app__header">
        <h1>RenderLaz</h1>
        <span className="role">懒人式体验特效渲染</span>
        {state.dbWarning && <span className="hint warn">{state.dbWarning}</span>}
      </header>

      <main className="app__main">
        <aside className="rail">
          <section className="rail__region rail__region--resources">
            <TabBar tabs={RESOURCE_TABS} active={resourceTab} onChange={setResourceTab} />
            <div className="rail__body rail__body--resources">
              {resourceTab === "resources" && (
                <ResourcePanel
                  resources={state.resources}
                  missing={state.missing}
                  current={state.currentImage}
                  onImport={() => void handleImport()}
                  onSelect={(path) => dispatch({ type: "current", path })}
                  onRemove={(path) => dispatch({ type: "resources/remove", path })}
                />
              )}
              {resourceTab === "batch" && (
                <BatchPanel
                  resourceCount={state.resources.length}
                  picked={pickedFiles}
                  outputDir={batchOutput}
                  running={batchRunning}
                  progress={batchProgress}
                  onPickFiles={() => void handlePickBatchFiles()}
                  onUseResources={() => setPickedFiles([])}
                  onPickOutput={() => void handlePickBatchOutput()}
                  onStart={() => void handleStartBatch()}
                  onCancel={() => {
                    cancelRef.current = true;
                    logger.warn("已请求取消，当前图片处理完后停止");
                  }}
                />
              )}
            </div>
          </section>

          <section className="rail__region rail__region--editor">
            <TabBar tabs={EDITOR_TABS} active={editorTab} onChange={setEditorTab} />
            <div className="rail__body rail__body--editor">
              {editorTab === "chain" && (
                <EffectChainPanel
                  chain={state.chain}
                  draft={draft}
                  editingIndex={editingIndex}
                  onChoose={handleChoose}
                  onAdd={handleAddEffect}
                  onParamsChange={handleParamsChange}
                  onSelectStep={(index) => {
                    setDraft(null);
                    setEditingIndex(index);
                  }}
                  onRemove={(index) => {
                    dispatch({ type: "chain/remove", index });
                    setEditingIndex((current) => {
                      if (current === null) return null;
                      if (current === index) return null;
                      return current > index ? current - 1 : current;
                    });
                  }}
                  onClear={() => {
                    dispatch({ type: "chain/clear" });
                    setEditingIndex(null);
                  }}
                />
              )}
              {editorTab === "log" && (
                <LogPanel
                  entries={state.logs}
                  runs={state.history}
                  onClear={() => dispatch({ type: "logs/clear" })}
                />
              )}
            </div>
          </section>
        </aside>

        <PreviewCanvas
          image={state.preview.image}
          error={state.preview.error}
          previewing={state.previewing}
          pendingLabel={draft ? effectLabels[draft.kind] : null}
          canExport={Boolean(state.currentImage) && !state.preview.error}
          onExport={() => void handleExport()}
        />
      </main>
    </div>
  );
}
