// Alloy Binary Data Implementation

// Buffer implementation (Node.js compatible)
class Buffer extends Uint8Array {
  static from(data, encodingOrOffset, length) {
    if (typeof data === "string") {
      const bytes = new TextEncoder().encode(data);
      const buf = new Buffer(bytes.length);
      buf.set(bytes);
      return buf;
    }
    if (data instanceof ArrayBuffer) {
      return new Buffer(data, encodingOrOffset, length);
    }
    if (ArrayBuffer.isView(data)) {
        const buf = new Buffer(data.byteLength);
        buf.set(new Uint8Array(data.buffer, data.byteOffset, data.byteLength));
        return buf;
    }
    return new Buffer(data);
  }

  toString(encoding = "utf8") {
    if (encoding === "base64") {
      return Alloy.bytesToBase64(this);
    }
    if (encoding === "hex") {
      return Alloy.bytesToHex(this);
    }
    return new TextDecoder().decode(this);
  }
}

globalThis.Buffer = Buffer;

// Blob implementation
class Blob {
  constructor(parts = [], options = {}) {
    this._parts = Array.isArray(parts) ? parts : [parts];
    this.type = options.type || "";
    this._size = -1;
  }

  get size() {
    if (this._size === -1) {
      this._size = this._parts.reduce((acc, part) => {
        if (typeof part === "string") return acc + new TextEncoder().encode(part).length;
        if (part instanceof ArrayBuffer) return acc + part.byteLength;
        if (ArrayBuffer.isView(part)) return acc + part.byteLength;
        if (part instanceof Blob) return acc + part.size;
        return acc;
      }, 0);
    }
    return this._size;
  }

  async arrayBuffer() {
    const sink = new Alloy.ArrayBufferSink();
    sink.start();
    for (const part of this._parts) {
      if (part instanceof Blob) {
        sink.write(await part.arrayBuffer());
      } else {
        sink.write(part);
      }
    }
    return sink.end();
  }

  async text() {
    const buf = await this.arrayBuffer();
    return new TextDecoder().decode(buf);
  }

  async bytes() {
    return new Uint8Array(await this.arrayBuffer());
  }

  stream() {
    const parts = this._parts;
    return new ReadableStream({
      async start(controller) {
        for (const part of parts) {
          if (part instanceof Blob) {
            const s = part.stream();
            const reader = s.getReader();
            while (true) {
              const { done, value } = await reader.read();
              if (done) break;
              controller.enqueue(value);
            }
          } else {
            controller.enqueue(part);
          }
        }
        controller.close();
      },
    });
  }
}

globalThis.Blob = Blob;

// File implementation
class File extends Blob {
  constructor(parts, name, options = {}) {
    super(parts, options);
    this.name = name;
    this.lastModified = options.lastModified || Date.now();
  }
}

globalThis.File = File;

// AlloyFile implementation (lazy-loaded file)
class AlloyFile extends Blob {
    constructor(path) {
        super();
        this.path = path;
        this._size = -1; // Determined by native binding
    }

    get name() {
        return this.path.split(/[\\/]/).pop();
    }

    async arrayBuffer() {
        return globalThis.Alloy._readFile(this.path);
    }
}

if (typeof Alloy !== "undefined") {
    Alloy.file = (path) => new AlloyFile(path);
}

// Alloy conversion utilities
if (typeof Alloy !== "undefined") {
    Alloy.readableStreamToArrayBuffer = async (stream) => {
    const response = new Response(stream);
    return await response.arrayBuffer();
};

    Alloy.readableStreamToBytes = async (stream) => {
    const response = new Response(stream);
    return await response.bytes();
};

    Alloy.readableStreamToText = async (stream) => {
    const response = new Response(stream);
    return await response.text();
};

    Alloy.readableStreamToArray = async (stream) => {
        const reader = stream.getReader();
        const chunks = [];
        while (true) {
            const { done, value } = await reader.read();
            if (done) break;
            chunks.push(value);
        }
        return chunks;
    };
}

export { Buffer, Blob, File };
