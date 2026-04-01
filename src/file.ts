declare global {
  interface Window {
    Alloy: {
      file_size: (path: string | number) => number;
      file_read: (path: string | number, format: string) => Promise<string | ArrayBuffer>;
      file_write: (path: string | number, data: any) => Promise<number>;
      file_delete: (path: string) => Promise<void>;
      file_exists: (path: string) => Promise<boolean>;
    };
  }
}

export class FileSink {
  private path: string | number;
  private highWaterMark: number = 65536;

  constructor(path: string | number, options?: { highWaterMark?: number }) {
    this.path = path;
    if (options?.highWaterMark) this.highWaterMark = options.highWaterMark;
  }

  write(chunk: string | ArrayBufferView | ArrayBuffer | SharedArrayBuffer): number {
    // Synchronous bridge call or async depending on implementation
    return 0; // Stub
  }

  flush(): number | Promise<number> {
    return 0; // Stub
  }

  end(error?: Error): number | Promise<number> {
    return 0; // Stub
  }

  ref(): void {}
  unref(): void {}
}

export class AlloyFile {
  readonly path: string | number | URL;
  private _type: string = "text/plain;charset=utf-8";

  constructor(path: string | number | URL, options?: { type?: string }) {
    this.path = path;
    if (options?.type) this._type = options.type;
  }

  get size(): number {
    return window.Alloy.file_size(this.path.toString());
  }

  get type(): string {
    return this._type;
  }

  async text(): Promise<string> {
    return window.Alloy.file_read(this.path.toString(), "text") as Promise<string>;
  }

  async json(): Promise<any> {
    const text = await this.text();
    return JSON.parse(text);
  }

  async arrayBuffer(): Promise<ArrayBuffer> {
    return window.Alloy.file_read(this.path.toString(), "arrayBuffer") as Promise<ArrayBuffer>;
  }

  async bytes(): Promise<Uint8Array> {
    const buffer = await this.arrayBuffer();
    return new Uint8Array(buffer);
  }

  stream(): ReadableStream {
    // Implement stream integration
    return new ReadableStream();
  }

  writer(params?: { highWaterMark?: number }): FileSink {
    return new FileSink(this.path.toString(), params);
  }

  async exists(): Promise<boolean> {
    return window.Alloy.file_exists(this.path.toString());
  }

  async delete(): Promise<void> {
    return window.Alloy.file_delete(this.path.toString());
  }
}

export const file = (path: string | number | URL, options?: { type?: string }): AlloyFile => {
  return new AlloyFile(path, options);
};

export const write = async (
  destination: string | number | AlloyFile | URL,
  input: string | Blob | ArrayBuffer | SharedArrayBuffer | TypedArray | Response
): Promise<number> => {
  const path = destination instanceof AlloyFile ? destination.path.toString() : destination.toString();
  let data: any = input;
  if (input instanceof Response) {
    data = await input.arrayBuffer();
  } else if (input instanceof Blob) {
    data = await input.arrayBuffer();
  }
  return window.Alloy.file_write(path, data);
};
