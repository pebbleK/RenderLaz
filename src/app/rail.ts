export type ResourceTabKey = "resources" | "batch";
export type EditorTabKey = "chain" | "log";
export type PanelTabKey = ResourceTabKey | EditorTabKey;

export type PanelTab<K extends PanelTabKey = PanelTabKey> = {
  key: K;
  label: string;
};

/** 资源区：导入、批处理、工程属于同一类，放在一个区里用页签切换。 */
export const RESOURCE_TABS: PanelTab<ResourceTabKey>[] = [
  { key: "resources", label: "资源" },
  { key: "batch", label: "批处理" },
];

/** 编辑区：链、参数、日志。 */
export const EDITOR_TABS: PanelTab<EditorTabKey>[] = [
  { key: "chain", label: "特效链" },
  { key: "log", label: "日志" },
];

export const DEFAULT_RESOURCE_TAB: ResourceTabKey = "resources";
export const DEFAULT_EDITOR_TAB: EditorTabKey = "chain";

/**
 * 批处理的输入来源：默认复用资源列表；显式选过其他图片时以选出者为准。
 */
export function resolveBatchFiles(picked: readonly string[], resources: readonly string[]): string[] {
  return picked.length > 0 ? [...picked] : [...resources];
}
