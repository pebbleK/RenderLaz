import type { EffectKind } from "../lib/bindings/EffectKind";
import type { EffectParams } from "../lib/bindings/EffectParams";
import type { EffectStep } from "../lib/bindings/EffectStep";
import { paramsFor, PARAM_LABELS, PARAM_LIMITS, type ParamKey } from "../effects/params";

export const effectLabels: Record<EffectKind, string> = {
  null: "直通",
  grayscale: "灰度",
  invert: "反色",
  sepia: "棕褐",
  blur: "模糊",
};

const kinds: EffectKind[] = ["grayscale", "invert", "sepia", "blur", "null"];

function display(key: ParamKey, value: number): string {
  if (key === "amount" || key === "contrast") {
    return `${Math.round(value * 100)}%`;
  }
  if (key === "iterations") {
    return `${Math.round(value)}`;
  }
  return `${value}`;
}

export function summarize(kind: EffectKind, params: EffectParams): string {
  return paramsFor(kind)
    .map((key) => `${PARAM_LABELS[key]} ${display(key, params[key])}`)
    .join(" · ");
}

type Props = {
  chain: EffectStep[];
  /** 下拉框选中、尚未入链的特效草稿 */
  draft: EffectStep | null;
  /** 正在编辑的链项索引 */
  editingIndex: number | null;
  onChoose: (kind: EffectKind | null) => void;
  onAdd: () => void;
  onParamsChange: (patch: Partial<EffectParams>) => void;
  onSelectStep: (index: number) => void;
  onRemove: (index: number) => void;
  onClear: () => void;
};

/** 特效链页签：下拉框 + 参数区 + 链条列表，参数不再单占页签。 */
export function EffectChainPanel({
  chain,
  draft,
  editingIndex,
  onChoose,
  onAdd,
  onParamsChange,
  onSelectStep,
  onRemove,
  onClear,
}: Props) {
  const editing = editingIndex !== null ? (chain[editingIndex] ?? null) : null;
  const active = draft ?? editing;
  const activeKind = active?.kind ?? null;

  return (
    <div className="panel">
      <div className="row">
        <select
          value={draft ? draft.kind : "none"}
          onChange={(event) => {
            const value = event.target.value;
            onChoose(value === "none" ? null : (value as EffectKind));
          }}
        >
          <option value="none">无</option>
          {kinds.map((kind) => (
            <option key={kind} value={kind}>
              {effectLabels[kind]}
            </option>
          ))}
        </select>
        <button type="button" onClick={onAdd} disabled={!draft}>
          添加
        </button>
        <button type="button" className="link" onClick={onClear} disabled={chain.length === 0}>
          清空
        </button>
      </div>

      <div className="params">
        <div className="params__title">
          <b>{activeKind ? `参数：${effectLabels[activeKind]}` : "参数"}</b>
          <span className="hint">
            {draft
              ? "未加入链，点「添加」才会保存"
              : editingIndex !== null
                ? `正在编辑链中第 ${editingIndex + 1} 项`
                : "先从下拉框选一种特效，或点下面的链项"}
          </span>
        </div>

        {!active || !activeKind ? (
          <p className="hint">当前没有可编辑的特效</p>
        ) : (
          paramsFor(activeKind).map((key) => {
            const limit = PARAM_LIMITS[key];
            const value = active.params[key];
            return (
              <label key={key} className="field">
                <span>
                  {PARAM_LABELS[key]}：{display(key, value)}
                </span>
                <input
                  type="range"
                  min={limit.min}
                  max={limit.max}
                  step={limit.step}
                  value={value}
                  onChange={(event) =>
                    onParamsChange({ [key]: Number(event.target.value) } as Partial<EffectParams>)
                  }
                />
              </label>
            );
          })
        )}
      </div>

      <div className="divider">特效链</div>

      {chain.length === 0 ? (
        <p className="panel__empty">还没有特效，预览显示原图</p>
      ) : (
        <ol className="list">
          {chain.map((step, index) => (
            <li
              key={`${step.kind}-${index}`}
              className={`list__item${editingIndex === index ? " list__item--active" : ""}`}
              onClick={() => onSelectStep(index)}
            >
              <span className="list__label" title={summarize(step.kind, step.params)}>
                {index + 1}. {effectLabels[step.kind]}
                <span className="hint"> · {summarize(step.kind, step.params)}</span>
              </span>
              <button
                type="button"
                className="link"
                onClick={(event) => {
                  event.stopPropagation();
                  onRemove(index);
                }}
              >
                删除
              </button>
            </li>
          ))}
        </ol>
      )}

      <p className="hint">点一项可回到上面的参数区调整它；顺序即执行顺序</p>
    </div>
  );
}
