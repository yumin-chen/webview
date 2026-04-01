import { describe, it, expect } from "bun:test";
import { ReadableStream, WritableStream, ArrayBufferSink, Response } from "../src/alloy/streams.js";

describe("Alloy Streams API", () => {
  it("should create a ReadableStream", () => {
    const stream = new ReadableStream({
      start(controller) {
        controller.enqueue("hello");
        controller.close();
      },
    });
    expect(stream).toBeDefined();
    expect(stream.type).toBe("standard");
  });

  it("should support direct ReadableStream type", () => {
    const stream = new ReadableStream({
      type: "direct",
      pull(controller) {
        controller.write("hello");
      },
    });
    expect(stream.type).toBe("direct");
  });

  it("should create a WritableStream", () => {
    const sink = new WritableStream();
    expect(sink).toBeDefined();
  });

  it("should work with ArrayBufferSink", () => {
    const sink = new ArrayBufferSink();
    sink.start({ highWaterMark: 10 });
    sink.write("hello");
    const buf = sink.end();
    expect(buf.byteLength).toBe(5);
  });

  it("should handle async generator in Response", async () => {
    const response = new Response(
      (async function* () {
        yield "hello";
        yield "world";
      })()
    );
    expect(await response.text()).toBe("helloworld");
  });
});
