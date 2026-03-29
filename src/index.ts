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
