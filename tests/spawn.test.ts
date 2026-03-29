import { expect, test, describe } from "bun:test";

describe("Spawn Bridge", () => {
    test("alloy.spawn exists", () => {
        expect((window as any).alloy.spawn).toBeDefined();
    });

    test("should spawn a process and read stdout", async () => {
        const proc = (window as any).alloy.spawn(["echo", "hello"]);
        const text = await proc.stdout.text();
        expect(text.trim()).toBe("hello");
        const exitCode = await proc.exited;
        expect(exitCode).toBe(0);
        expect(proc.pid).toBeGreaterThan(0);
    });

    test("should handle spawnSync", async () => {
        const result = await (window as any).alloy.spawnSync(["echo", "sync"]);
        const dec = new TextDecoder();
        expect(dec.decode(result.stdout).trim()).toBe("sync");
        expect(result.exitCode).toBe(0);
    });

    test("should pipe stdin", async () => {
        const proc = (window as any).alloy.spawn(["cat"], {
            stdin: "pipe",
            stdout: "pipe"
        });
        proc.stdin.write("alloy");
        proc.stdin.end();
        const text = await proc.stdout.text();
        expect(text.trim()).toBe("alloy");
    });
});
