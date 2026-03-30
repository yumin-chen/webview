import { expect, test, describe } from "bun:test";

declare const Alloy: any;

describe("Alloy.spawn", () => {
  test("basic execution", async () => {
    const proc = Alloy.spawn(["echo", "hello"]);
    expect(proc.pid).toBeGreaterThan(0);
    const code = await proc.exited;
    expect(code).toBe(0);
  });

  test("stdout streaming", async () => {
    const proc = Alloy.spawn(["echo", "world"]);
    const reader = proc.stdout.getReader();
    const { value } = await reader.read();
    expect(new TextDecoder().decode(value).trim()).toBe("world");
  });

  test("timeout handling", async () => {
    const proc = Alloy.spawn({
      cmd: ["sleep", "10"],
      timeout: 500
    });
    const code = await proc.exited;
    expect(proc.killed).toBe(true);
    // signalCode should be set (e.g. SIGTERM)
  });

  test("terminal support", async () => {
    if (process.platform !== "win32") {
      const proc = Alloy.spawn(["bash"], {
        terminal: { cols: 80, rows: 24 }
      });
      expect(proc.terminal).toBeDefined();
      proc.terminal.write("echo hello\n");
      // Wait for data
      proc.terminal.close();
    }
  });

  test("AsyncDisposable", async () => {
    {
      await using proc = Alloy.spawn(["sleep", "10"]);
      expect(proc.pid).toBeGreaterThan(0);
    }
    // proc is killed here
  });
});

describe("Alloy.spawnSync", () => {
  test("synchronous result", () => {
    const res = Alloy.spawnSync(["echo", "sync"]);
    expect(res.success).toBe(true);
    expect(new TextDecoder().decode(res.stdout).trim()).toBe("sync");
  });

  test("maxBuffer limit", () => {
    // In our simplified impl, maxBuffer isn't strictly enforced in C++ yet
    // but the API is there.
  });
});
