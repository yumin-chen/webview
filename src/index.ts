declare global {
  interface Window {
    Alloy: {
      spawn: (command: string, args: string[]) => Promise<number>;
      spawnSync: (command: string, args: string[]) => number;
      secureEval: (code: string) => string;
    };
  }
}

export const spawn = async (command: string, args: string[]): Promise<number> => {
  return window.Alloy.spawn(command, args);
};

export const spawnSync = (command: string, args: string[]): number => {
  return window.Alloy.spawnSync(command, args);
};

export const secureEval = (code: string): string => {
  return window.Alloy.secureEval(code);
};

import { ArrayBufferSink } from "./streams";
import { file, write, AlloyFile, FileSink } from "./file";

export * from "./sqlite";
export * from "./gui";
export { ArrayBufferSink, AlloyFile, FileSink };

export const Alloy = {
  ArrayBufferSink,
  file,
  write,
  stdin: new AlloyFile(0),
  stdout: new AlloyFile(1),
  stderr: new AlloyFile(2),
};
