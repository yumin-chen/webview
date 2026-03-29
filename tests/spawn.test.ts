import { expect, test, describe } from "bun:test";

describe("Alloy.spawn", () => {
  test("should spawn a process and capture output", async () => {
    const proc = Alloy.spawn(["echo", "hello world"]);
    const reader = proc.stdout.getReader();
    const decoder = new TextDecoder();
    let output = "";

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }

    expect(output.trim()).toBe("hello world");
    const exitCode = await proc.exited;
    expect(exitCode).toBe(0);
  });

  test("should handle stderr", async () => {
    const proc = Alloy.spawn(["sh", "-c", "echo error >&2"]);
    const reader = proc.stderr.getReader();
    const decoder = new TextDecoder();
    let output = "";

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }

    expect(output.trim()).toBe("error");
    await proc.exited;
  });

  test("should support working directory (cwd)", async () => {
    const proc = Alloy.spawn(["pwd"], { cwd: "/tmp" });
    const reader = proc.stdout.getReader();
    const decoder = new TextDecoder();
    let output = "";

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }

    expect(output.trim()).toBe("/tmp");
    await proc.exited;
  });

  test("should support environment variables", async () => {
    const proc = Alloy.spawn(["sh", "-c", "echo $FOO"], {
      env: { FOO: "bar" }
    });
    const reader = proc.stdout.getReader();
    const decoder = new TextDecoder();
    let output = "";

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      output += decoder.decode(value);
    }

    expect(output.trim()).toBe("bar");
    await proc.exited;
  });

  test("should be able to kill a process", async () => {
    const proc = Alloy.spawn(["sleep", "10"]);
    proc.kill();
    const exitCode = await proc.exited;
    expect(exitCode).toBeLessThan(0);
    expect(proc.killed).toBe(true);
  });

  test("should support PTY (terminal)", async () => {
    const proc = Alloy.spawn(["bash"], {
      terminal: { cols: 80, rows: 24 }
    });

    let output = "";
    const decoder = new TextDecoder();

    const ptyPromise = new Promise((resolve) => {
        proc.terminal._onData = (data) => {
            output += decoder.decode(data);
            if (output.includes("hello-from-pty")) {
                resolve();
            }
        };
    });

    proc.terminal.write("echo hello-from-pty\n");
    await ptyPromise;

    proc.terminal.write("exit\n");
    const exitCode = await proc.exited;
    expect(exitCode).toBe(0);
    expect(output).toContain("hello-from-pty");
  });
});

describe("Alloy.spawnSync", () => {
  test("should execute synchronously and return result", () => {
    const result = Alloy.spawnSync(["echo", "sync-test"]);
    const decoder = new TextDecoder();

    expect(result.success).toBe(true);
    expect(result.exitCode).toBe(0);
    expect(decoder.decode(result.stdout).trim()).toBe("sync-test");
  });

  test("should return resource usage", () => {
    const result = Alloy.spawnSync(["ls"]);
    expect(result.resourceUsage).toBeDefined();
    expect(result.resourceUsage.maxRSS).toBeGreaterThan(0);
  });
});
