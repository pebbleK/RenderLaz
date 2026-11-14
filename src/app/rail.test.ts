import { describe, expect, it } from "vitest";

import {
  DEFAULT_EDITOR_TAB,
  DEFAULT_RESOURCE_TAB,
  EDITOR_TABS,
  RESOURCE_TABS,
  resolveBatchFiles,
} from "./rail";

describe("页签常量", () => {
  it("资源区是 资源/批处理", () => {
    expect(RESOURCE_TABS.map((tab) => tab.label)).toEqual(["资源", "批处理"]);
  });

  it("编辑区是 特效链/日志", () => {
    expect(EDITOR_TABS.map((tab) => tab.label)).toEqual(["特效链", "日志"]);
  });

  it("默认页签落在各自区的第一项", () => {
    expect(DEFAULT_RESOURCE_TAB).toBe(RESOURCE_TABS[0].key);
    expect(DEFAULT_EDITOR_TAB).toBe(EDITOR_TABS[0].key);
  });

  it("资源区 2 个页签、编辑区 2 个页签", () => {
    expect(RESOURCE_TABS).toHaveLength(2);
    expect(EDITOR_TABS).toHaveLength(2);
  });
});

describe("resolveBatchFiles", () => {
  it("未显式选图时复用资源列表", () => {
    expect(resolveBatchFiles([], ["a.png", "b.png"])).toEqual(["a.png", "b.png"]);
  });

  it("显式选图时以选出者为准", () => {
    expect(resolveBatchFiles(["x.png"], ["a.png"])).toEqual(["x.png"]);
  });

  it("返回新数组，调用方改动不会污染入参", () => {
    const resources = ["a.png"];
    const result = resolveBatchFiles([], resources);
    result.push("b.png");
    expect(resources).toEqual(["a.png"]);
  });
});
