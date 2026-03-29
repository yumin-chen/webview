import { expect, test, describe } from "bun:test";

declare const Alloy: any;

describe("Alloy.spawn", () => {
  test("basic spawn and exit code", async () => {
    // spawn is now synchronous
    const proc = Alloy.spawn(["echo", "hello"]);
    expect(proc.pid).toBeGreaterThan(0);
    const exitCode = await proc.exited;
    expect(exitCode).toBe(0);
  });

  test("stdout stream", async () => {
    const proc = Alloy.spawn(["echo", "hello world"]);
    const reader = proc.stdout.getReader();
    let output = "";
    const decoder = new TextDecoder();
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }
    expect(output.trim()).toBe("hello world");
  });

  test("stderr stream", async () => {
    const proc = Alloy.spawn(["sh", "-c", "echo error >&2"]);
    const reader = proc.stderr.getReader();
    let output = "";
    const decoder = new TextDecoder();
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }
    expect(output.trim()).toBe("error");
  });

  test("stdin write", async () => {
    const proc = Alloy.spawn(["cat"]);
    await proc.stdin.write("hello from stdin");
    await proc.stdin.end();

    const reader = proc.stdout.getReader();
    let output = "";
    const decoder = new TextDecoder();
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }
    expect(output).toBe("hello from stdin");
  });

  test("kill process", async () => {
    const proc = Alloy.spawn(["sleep", "10"]);
    proc.kill();
    const exitCode = await proc.exited;
    expect(exitCode).not.toBe(0);
  });
});

describe("Alloy.spawnSync", () => {
  test("basic spawnSync", () => {
    const res = Alloy.spawnSync(["echo", "hello sync"]);
    expect(res.success).toBe(true);
    expect(res.exitCode).toBe(0);
    expect(res.stdout.trim()).toBe("hello sync");
  });

  test("spawnSync with error", () => {
    const res = Alloy.spawnSync(["sh", "-c", "exit 1"]);
    expect(res.success).toBe(false);
    expect(res.exitCode).toBe(1);
  });
});

if (process.platform !== "win32") {
  describe("Alloy Terminal (PTY)", () => {
    test("terminal spawn", async () => {
      const proc = Alloy.spawn(["echo", "terminal"], { terminal: { cols: 80, rows: 24 } });
      const reader = proc.stdout.getReader();
      let output = "";
      const decoder = new TextDecoder();
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        output += decoder.decode(value);
      }
      expect(output).toContain("terminal");
      await proc.exited;
    });
  });
}
