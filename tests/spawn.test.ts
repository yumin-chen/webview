import { expect, test, describe } from "bun:test";

// Since Alloy is injected into the WebView, we need to mock or
// assume its presence if running directly under Bun for unit testing logic.
// In a real WebView environment, window.Alloy would be provided by C++.

const AlloyMock = {
    spawn: async (cmd: string[], options: any = {}) => {
        const proc = Bun.spawn(cmd, {
            cwd: options.cwd,
            env: options.env,
            stdout: "pipe",
            stderr: "pipe",
        });
        return {
            pid: proc.pid,
            stdout: proc.stdout,
            stderr: proc.stderr,
            exited: proc.exited,
            kill: () => proc.kill(),
        };
    },
    spawnSync: (cmd: string[], options: any = {}) => {
        const proc = Bun.spawnSync(cmd, {
            cwd: options.cwd,
            env: options.env,
        });
        return {
            success: proc.success,
            stdout: proc.stdout,
            stderr: proc.stderr,
            exitCode: proc.exitCode,
        };
    }
};

describe("Alloy.spawn", () => {
    test("should spawn a process and capture output", async () => {
        const proc = await AlloyMock.spawn(["echo", "hello"]);
        const text = await new Response(proc.stdout).text();
        expect(text.trim()).toBe("hello");
        const code = await proc.exited;
        expect(code).toBe(0);
    });

    test("should respect working directory", async () => {
        const proc = await AlloyMock.spawn(["pwd"], { cwd: "/tmp" });
        const text = await new Response(proc.stdout).text();
        expect(text.trim()).toContain("tmp");
    });
});

describe("Alloy.spawnSync", () => {
    test("should execute synchronously", () => {
        const result = AlloyMock.spawnSync(["echo", "sync-test"]);
        expect(result.success).toBe(true);
        expect(result.stdout.toString().trim()).toBe("sync-test");
    });

    test("should return non-zero exit code on failure", () => {
        const result = AlloyMock.spawnSync(["false"]);
        expect(result.success).toBe(false);
        expect(result.exitCode).toBe(1);
    });
});
