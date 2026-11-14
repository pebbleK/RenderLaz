import { describe, expect, it, vi } from "vitest";

import { candidateName, runBatch, uniqueName } from "./batch";

describe("uniqueName", () => {
  it("同名时追加序号", () => {
    const used = new Set<string>();
    expect(uniqueName(used, "a-out.png")).toBe("a-out.png");
    expect(uniqueName(used, "a-out.png")).toBe("a-out-2.png");
    expect(uniqueName(used, "a-out.png")).toBe("a-out-3.png");
  });
});

describe("candidateName", () => {
  it("取主文件名并加 -out 后缀", () => {
    expect(candidateName("/tmp/a.jpg")).toBe("a-out.png");
    expect(candidateName("C:\\pics\\b.PNG")).toBe("b-out.png");
    expect(candidateName("noext")).toBe("noext-out.png");
  });
});

describe("runBatch", () => {
  it("逐张执行并统计成功与失败", async () => {
    const processed: string[] = [];
    const outcome = await runBatch({
      files: ["/a.png", "/bad.png", "/c.png"],
      outputDir: "/out",
      process: async (path) => {
        if (path.includes("bad")) {
          throw new Error("解码失败");
        }
        processed.push(path);
      },
      shouldCancel: () => false,
    });

    expect(processed).toEqual(["/a.png", "/c.png"]);
    expect(outcome.okCount).toBe(2);
    expect(outcome.failCount).toBe(1);
    expect(outcome.items[1].status).toBe("failed");
    expect(outcome.items[1].error).toBe("解码失败");
    expect(outcome.items[0].output_path).toBe("/out/a-out.png");
  });

  it("取消后剩余项标记为 skipped 且不再调用处理函数", async () => {
    let canceled = false;
    const spy = vi.fn(async () => {
      canceled = true;
    });

    const outcome = await runBatch({
      files: ["/a.png", "/b.png"],
      outputDir: "/out",
      process: spy,
      shouldCancel: () => canceled,
    });

    expect(spy).toHaveBeenCalledTimes(1);
    expect(outcome.canceled).toBe(true);
    expect(outcome.items[1].status).toBe("skipped");
  });

  it("进度回调按完成张数上报", async () => {
    const progress: number[] = [];
    await runBatch({
      files: ["/a.png", "/b.png"],
      outputDir: "/out",
      process: async () => {},
      shouldCancel: () => false,
      onProgress: (done) => progress.push(done),
    });
    expect(progress).toEqual([1, 2]);
  });
});
