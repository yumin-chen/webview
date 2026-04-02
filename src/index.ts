/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
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

export * from "./sqlite";
export * from "./gui";
