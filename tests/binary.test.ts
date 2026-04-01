import { describe, it, expect } from "bun:test";
import { ReadableStream, ArrayBufferSink } from "../src/alloy/streams.js";

// Mock Alloy global for tests
globalThis.Alloy = globalThis.Alloy || {};
Object.assign(globalThis.Alloy, {
    ArrayBufferSink: ArrayBufferSink,
    _readFile: async (path) => new ArrayBuffer(10)
});

import { Buffer, Blob, File } from "../src/alloy/binary.js";

describe("Alloy Binary Data API", () => {
  it("should create a Buffer from string", () => {
    const buf = Buffer.from("hello");
    expect(buf).toBeInstanceOf(Uint8Array);
    expect(buf.length).toBe(5);
  });

  it("should create a Blob from parts", async () => {
    const blob = new Blob(["hello", new Uint8Array([32]), "world"]);
    expect(blob.size).toBe(11);
    expect(await blob.text()).toBe("hello world");
  });

  it("should create a File", () => {
    const file = new File(["data"], "test.txt", { type: "text/plain" });
    expect(file.name).toBe("test.txt");
    expect(file.type).toBe("text/plain");
  });

  it("should handle Alloy.file (AlloyFile)", async () => {
    const file = Alloy.file("example.txt");
    expect(file.name).toBe("example.txt");
    const buf = await file.arrayBuffer();
    expect(buf.byteLength).toBe(10);
  });
});
