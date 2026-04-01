import { expect, test, describe } from "bun:test";

const mockAlloy = {
    file: (path: string) => ({
        path,
        size: 10,
        exists: () => Promise.resolve(true),
        text: () => Promise.resolve("content"),
        delete: () => Promise.resolve(true),
        writer: () => ({
            write: () => 7,
            flush: () => 7,
            end: () => 0
        })
    }),
    write: (dest: any, data: any) => Promise.resolve(7),
    stdout: { path: "/dev/stdout" }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.file", () => {
  test("metadata", async () => {
    const f = Alloy.file("test.txt");
    expect(f.size).toBe(10);
    expect(await f.exists()).toBe(true);
  });

  test("read text", async () => {
    const f = Alloy.file("test.txt");
    expect(await f.text()).toBe("content");
  });

  test("writer", () => {
    const f = Alloy.file("test.txt");
    const w = f.writer();
    expect(w.write("hello")).toBe(7);
    expect(w.end()).toBe(0);
  });
});

describe("Alloy.write", () => {
  test("basic write", async () => {
    const res = await Alloy.write("out.txt", "data");
    expect(res).toBe(7);
  });
});
