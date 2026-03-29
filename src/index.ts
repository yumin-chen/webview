declare global {
  interface Window {
    Alloy: {
      spawn: (command: string, args: string[]) => Promise<number>;
      spawnSync: (command: string, args: string[]) => number;
    };
  }
}

export const spawn = async (command: string, args: string[]): Promise<number> => {
  return window.Alloy.spawn(command, args);
};

export const spawnSync = (command: string, args: string[]): number => {
  return window.Alloy.spawnSync(command, args);
};

export * from "./sqlite";
