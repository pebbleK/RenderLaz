import { useEffect, useRef } from "react";

import { drawToCanvas } from "../lib/image";

type Props = {
  image: ImageData | null;
  error: string | null;
  previewing: boolean;
  /** 下拉框里正在临时预览、尚未加入链的特效名 */
  pendingLabel: string | null;
  canExport: boolean;
  onExport: () => void;
};

export function PreviewCanvas({ image, error, previewing, pendingLabel, canExport, onExport }: Props) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    if (canvasRef.current) {
      drawToCanvas(canvasRef.current, image);
    }
  }, [image]);

  return (
    <section className="preview">
      <header className="preview__head">
        <h2 className="preview__title">预览</h2>
        <div className="row">
          {pendingLabel && <span className="hint accent">预览：{pendingLabel}（未加入链）</span>}
          {image && (
            <span className="hint">
              {image.width} × {image.height}
            </span>
          )}
          <button type="button" onClick={onExport} disabled={!canExport}>
            导出 PNG
          </button>
        </div>
      </header>

      <div className="preview__stage">
        {error ? (
          <p className="hint error">{error}</p>
        ) : image ? (
          <canvas ref={canvasRef} className="preview__canvas" />
        ) : (
          <p className="hint">导入图片后这里显示结果</p>
        )}
        {previewing && <span className="preview__badge">渲染中…</span>}
      </div>
    </section>
  );
}
