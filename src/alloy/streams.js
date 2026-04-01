// Alloy Streams API Implementation

class ReadableStream {
  constructor(source = {}) {
    this.type = source.type || "standard";
    this._source = source;
    this._controller = this._createController();

    if (source.start) {
      source.start(this._controller);
    }
  }

  _createController() {
    if (this.type === "direct") {
      return {
        write: (chunk) => this._enqueue(chunk),
        close: () => this._close(),
        error: (e) => this._error(e)
      };
    } else {
      return {
        enqueue: (chunk) => this._enqueue(chunk),
        close: () => this._close(),
        error: (e) => this._error(e)
      };
    }
  }

  _enqueue(chunk) {
    if (this._onData) this._onData(chunk);
  }

  _close() {
    this._closed = true;
    if (this._onEnd) this._onEnd();
  }

  _error(e) {
    this._errored = true;
    if (this._onError) this._onError(e);
  }

  async *[Symbol.asyncIterator]() {
    while (!this._closed) {
      const chunk = await new Promise(resolve => {
        this._onData = resolve;
      });
      yield chunk;
    }
  }
}

class WritableStream {
  constructor(sink = {}) {
    this._sink = sink;
  }
}

// Global Alloy object and ArrayBufferSink
if (typeof Alloy === "undefined") {
  globalThis.Alloy = {};
}

class ArrayBufferSink {
  constructor() {
    this._buffer = null;
    this._options = {};
  }

  start(options = {}) {
    this._options = options;
    this._buffer = new Uint8Array(options.highWaterMark || 1024);
    this._pos = 0;
  }

  write(chunk) {
    let data;
    if (typeof chunk === "string") {
      data = new TextEncoder().encode(chunk);
    } else if (chunk instanceof Uint8Array) {
      data = chunk;
    } else if (chunk instanceof ArrayBuffer) {
      data = new Uint8Array(chunk);
    }

    if (this._pos + data.length > this._buffer.length) {
      const newBuf = new Uint8Array(Math.max(this._buffer.length * 2, this._pos + data.length));
      newBuf.set(this._buffer);
      this._buffer = newBuf;
    }
    this._buffer.set(data, this._pos);
    this._pos += data.length;
    return data.length;
  }

  flush() {
    const data = this._buffer.slice(0, this._pos);
    if (!this._options.stream) {
        return this._pos;
    }
    this._pos = 0;
    return this._options.asUint8Array ? data : data.buffer;
  }

  end() {
    const data = this._buffer.slice(0, this._pos);
    return this._options.asUint8Array ? data : data.buffer;
  }
}

Alloy.ArrayBufferSink = ArrayBufferSink;

// Async generator support for Response/Request (Mock)
class Response {
    constructor(body) {
        this.body = body;
    }
    async text() {
        if (typeof this.body === "string") return this.body;
        if (this.body && this.body[Symbol.asyncIterator]) {
            let res = "";
            for await (const chunk of this.body) {
                res += chunk;
            }
            return res;
        }
        return "";
    }
}

globalThis.ReadableStream = ReadableStream;
globalThis.WritableStream = WritableStream;
globalThis.Response = Response;

export { ReadableStream, WritableStream, ArrayBufferSink, Response };
