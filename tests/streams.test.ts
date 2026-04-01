import { expect, test, describe } from "bun:test";
import { ArrayBufferSink } from "../src/streams";

describe("ArrayBufferSink", () => {
  test("Basic write and end", () => {
    const sink = new ArrayBufferSink();
    sink.start();
    sink.write("h");
    sink.write("e");
    sink.write("llo");
    const result = sink.end();
    expect(result.byteLength).toBe(5);
    const view = new Uint8Array(result);
    expect(view[0]).toBe(104); // 'h'
    expect(view[4]).toBe(111); // 'o'
  });

  test("Uint8Array output", () => {
    const sink = new ArrayBufferSink();
    sink.start({ asUint8Array: true });
    sink.write("abc");
    const result = sink.end() as Uint8Array;
    expect(result instanceof Uint8Array).toBe(true);
    expect(result.length).toBe(3);
    expect(new TextDecoder().decode(result)).toBe("abc");
  });

  test("Streaming flush", () => {
    const sink = new ArrayBufferSink();
    sink.start({ stream: true, asUint8Array: true });
    sink.write("hello");
    const part1 = sink.flush() as Uint8Array;
    expect(part1.length).toBe(5);

    sink.write("world");
    const part2 = sink.flush() as Uint8Array;
    expect(part2.length).toBe(5);
    expect(new TextDecoder().decode(part2)).toBe("world");
  });
});
