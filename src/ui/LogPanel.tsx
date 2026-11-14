import { baseName } from "../lib/image";
import type { BatchRun } from "../lib/ipc";
import type { LogEntry } from "../lib/log";
import { runStatusLabel } from "../app/state";

type Props = {
  entries: LogEntry[];
  runs: BatchRun[];
  onClear: () => void;
};

/** 日志页：上半批处理历史，下半实时日志，共用一个滚动容器。 */
export function LogPanel({ entries, runs, onClear }: Props) {
  return (
    <div className="panel">
      <div className="divider">批处理历史</div>

      {runs.length === 0 ? (
        <p className="panel__empty">还没有批处理记录</p>
      ) : (
        <div className="history">
          {runs.map((run) => (
            <details key={run.id} className="history__item">
              <summary>
                <span>{run.started_at.replace("T", " ").slice(0, 19)}</span>
                <span className="hint">
                  {" "}
                  · {runStatusLabel[run.status]} · {run.input_count} 张 · 成功 {run.ok_count} / 失败 {run.fail_count}
                </span>
              </summary>
              <ul className="list">
                {run.items.map((item, index) => (
                  <li key={`${run.id}-${index}`} className="list__item">
                    <span className="list__label" title={item.path}>
                      {baseName(item.path)}
                      {item.status === "failed" && <span className="hint error"> · {item.error}</span>}
                      {item.status === "skipped" && <span className="hint"> · 跳过</span>}
                    </span>
                  </li>
                ))}
              </ul>
            </details>
          ))}
        </div>
      )}

      <div className="divider">
        运行日志
        <button type="button" className="link" onClick={onClear} disabled={entries.length === 0}>
          清空
        </button>
      </div>

      <div className="log">
        {entries.length === 0 ? (
          <p className="panel__empty">暂无日志</p>
        ) : (
          entries.map((entry) => (
            <div key={entry.id} className={`log__line log__line--${entry.level}`}>
              <span className="log__time">{entry.time}</span>
              <span>{entry.message}</span>
            </div>
          ))
        )}
      </div>
    </div>
  );
}
