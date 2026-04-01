import { expect, test, describe, beforeAll } from "bun:test";
import { Alloy } from "../src/index";

// Mocking window.Alloy for tests since we are running in bun:test environment
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = {
    file_size: (path: string) => 42,
    file_read: async (path: string, format: string) => "file content",
    file_write: async (path: string, data: any) => 12,
    file_exists: async (path: string) => true,
    file_delete: async (path: string) => {}
};

describe("Alloy File API", () => {
  test("Alloy.file properties", () => {
    const f = Alloy.file("test.txt");
    expect(f.size).toBe(42);
    expect(f.type).toBe("text/plain;charset=utf-8");
  });

  test("AlloyFile reading", async () => {
    const f = Alloy.file("test.txt");
    const text = await f.text();
    expect(text).toBe("file content");

    const exists = await f.exists();
    expect(exists).toBe(true);
  });

  test("Alloy.write", async () => {
    const written = await Alloy.write("output.txt", "hello");
    expect(written).toBe(12);
  });

  test("Standard streams", () => {
    expect(Alloy.stdin.path).toBe(0);
    expect(Alloy.stdout.path).toBe(1);
    expect(Alloy.stderr.path).toBe(2);
  });
});
