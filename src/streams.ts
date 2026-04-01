export class ArrayBufferSink {
  private buffer: Uint8Array | null = null;
  private offset: number = 0;
  private highWaterMark: number = 65536; // 64KB default
  private isStream: boolean = false;
  private asUint8Array: boolean = false;

  constructor() {}

  start(options?: {
    asUint8Array?: boolean;
    highWaterMark?: number;
    stream?: boolean;
  }): void {
    if (options?.highWaterMark) this.highWaterMark = options.highWaterMark;
    if (options?.stream) this.isStream = options.stream;
    if (options?.asUint8Array) this.asUint8Array = options.asUint8Array;
    this.buffer = new Uint8Array(this.highWaterMark);
    this.offset = 0;
  }

  write(chunk: string | ArrayBufferView | ArrayBuffer | SharedArrayBuffer): number {
    if (!this.buffer) this.start();
    let data: Uint8Array;
    if (typeof chunk === "string") {
      data = new TextEncoder().encode(chunk);
    } else if (chunk instanceof ArrayBuffer || chunk instanceof SharedArrayBuffer) {
      data = new Uint8Array(chunk);
    } else {
      data = new Uint8Array(chunk.buffer, chunk.byteOffset, chunk.byteLength);
    }

    if (this.offset + data.length > this.buffer!.length) {
      const newSize = Math.max(this.buffer!.length * 2, this.offset + data.length);
      const newBuffer = new Uint8Array(newSize);
      newBuffer.set(this.buffer!);
      this.buffer = newBuffer;
    }

    this.buffer!.set(data, this.offset);
    this.offset += data.length;
    return data.length;
  }

  flush(): number | Uint8Array | ArrayBuffer {
    if (!this.buffer) return 0;
    const result = this.buffer.slice(0, this.offset);
    if (this.isStream) {
      this.offset = 0; // Reset for next flush
      return this.asUint8Array ? result : result.buffer;
    }
    return this.offset;
  }

  end(): ArrayBuffer | Uint8Array {
    const result = this.buffer ? this.buffer.slice(0, this.offset) : new Uint8Array(0);
    this.buffer = null;
    this.offset = 0;
    return this.asUint8Array ? result : result.buffer;
  }
}
