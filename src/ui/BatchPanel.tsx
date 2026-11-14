type Props = {
  resourceCount: number;
  picked: string[];
  outputDir: string | null;
  running: boolean;
  progress: { done: number; total: number } | null;
  onPickFiles: () => void;
  onUseResources: () => void;
  onPickOutput: () => void;
  onStart: () => void;
  onCancel: () => void;
};

export function BatchPanel({
  resourceCount,
  picked,
  outputDir,
  running,
  progress,
  onPickFiles,
  onUseResources,
  onPickOutput,
  onStart,
  onCancel,
}: Props) {
  const usingPicked = picked.length > 0;
  const total = usingPicked ? picked.length : resourceCount;
  const percent = progress ? Math.round((progress.done / Math.max(1, progress.total)) * 100) : 0;

  return (
    <div className="panel">
      <p className="hint">
        {usingPicked ? `处理对象：已选 ${picked.length} 张` : `处理对象：资源列表中的 ${resourceCount} 张`}
      </p>

      <div className="row">
        <button type="button" onClick={onPickOutput} disabled={running}>
          输出目录
        </button>
        {usingPicked ? (
          <button type="button" className="link" onClick={onUseResources} disabled={running}>
            改回资源列表
          </button>
        ) : (
          <button type="button" className="link" onClick={onPickFiles} disabled={running}>
            选择其他图片…
          </button>
        )}
      </div>

      <p className="hint" title={outputDir ?? ""}>
        输出：{outputDir ?? "未选择"}
      </p>

      <div className="row">
        <button type="button" className="primary" onClick={onStart} disabled={running || total === 0 || !outputDir}>
          开始
        </button>
        <button type="button" onClick={onCancel} disabled={!running}>
          取消
        </button>
      </div>

      {progress && (
        <div className="progress">
          <div className="progress__bar" style={{ width: `${percent}%` }} />
          <span className="progress__label">
            {progress.done} / {progress.total}
          </span>
        </div>
      )}
    </div>
  );
}
