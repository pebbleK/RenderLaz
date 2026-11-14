import type { PanelTab, PanelTabKey } from "../app/rail";

type Props<K extends PanelTabKey> = {
  tabs: PanelTab<K>[];
  active: K;
  onChange: (key: K) => void;
};

/** 通用页签栏：两个区各用一份，样式与行为完全一致。 */
export function TabBar<K extends PanelTabKey>({ tabs, active, onChange }: Props<K>) {
  return (
    <nav className="tabbar" role="tablist">
      {tabs.map((tab) => (
        <button
          key={tab.key}
          type="button"
          role="tab"
          aria-selected={tab.key === active}
          className={tab.key === active ? "tabbar__tab tabbar__tab--active" : "tabbar__tab"}
          onClick={() => onChange(tab.key)}
        >
          {tab.label}
        </button>
      ))}
    </nav>
  );
}
