import { baseName } from "../lib/image";

type Props = {
  resources: string[];
  missing: string[];
  current: string | null;
  onImport: () => void;
  onSelect: (path: string) => void;
  onRemove: (path: string) => void;
};

export function ResourcePanel({ resources, missing, current, onImport, onSelect, onRemove }: Props) {
  return (
    <div className="panel">
      <div className="row">
        <button type="button" onClick={onImport}>
          导入图片
        </button>
        <span className="hint">{resources.length} 张</span>
      </div>

      {resources.length === 0 ? (
        <p className="panel__empty">还没有导入图片</p>
      ) : (
        <ul className="list">
          {resources.map((path) => {
            const isMissing = missing.includes(path);
            return (
              <li
                key={path}
                className={`list__item${current === path ? " list__item--active" : ""}`}
                onClick={() => onSelect(path)}
              >
                <span className="list__label" title={path}>
                  {isMissing ? "⚠ " : ""}
                  {baseName(path)}
                </span>
                <button
                  type="button"
                  className="link"
                  onClick={(event) => {
                    event.stopPropagation();
                    onRemove(path);
                  }}
                >
                  移除
                </button>
              </li>
            );
          })}
        </ul>
      )}

      {missing.length > 0 && <p className="hint warn">有 {missing.length} 个文件缺失，预览与导出会跳过它们</p>}
    </div>
  );
}
