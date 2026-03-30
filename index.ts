export * from "./shell";
export * from "./sqlite";

export interface AlloyFile {
  readonly size: number;
  readonly type: string;
  text(): Promise<string>;
  stream(): ReadableStream;
  arrayBuffer(): Promise<ArrayBuffer>;
  json(): Promise<any>;
  writer(params: { highWaterMark?: number }): FileSink;
  exists(): Promise<boolean>;
  delete(): Promise<boolean>;
}

export interface FileSink {
  write(chunk: string | ArrayBufferView | ArrayBuffer | SharedArrayBuffer): number;
  flush(): number | Promise<number>;
  end(error?: Error): number | Promise<number>;
  ref(): void;
  unref(): void;
}

declare global {
  interface Window {
    Alloy: {
      file(path: string | number | URL, options?: { type?: string }): AlloyFile;
      write(destination: string | number | AlloyFile | URL, input: any): Promise<number>;
      stdin: AlloyFile;
      stdout: AlloyFile;
      stderr: AlloyFile;
      gui: any;
      cron: any;
      spawn: any;
      spawnSync: any;
    };
    secureEval: (code: string) => any;
    _forbidden_eval: (code: string) => any;
  }
  const Alloy: any;
}
