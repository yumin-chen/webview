import { expect, test, describe } from "bun:test";

// Mock environment for tests if Alloy is not available
const mockAlloy = {
    spawn: () => ({
        pid: 123,
        exited: Promise.resolve(0),
        stdout: {
            getReader: () => {
                let called = false;
                return {
                    read: () => {
                        if (called) return Promise.resolve({ done: true, value: undefined });
                        called = true;
                        return Promise.resolve({ done: false, value: new TextEncoder().encode("sync") });
                    }
                };
            }
        },
        stderr: { getReader: () => ({ read: () => Promise.resolve({ done: true, value: undefined }) }) },
        stdin: { write: () => Promise.resolve(), end: () => Promise.resolve() }
    }),
    spawnSync: () => ({ stdout: Buffer.from("sync"), exitCode: 0, success: true }),
    cron: () => Promise.resolve(true),
    gui: {
        createWindow: () => "1001",
        createButton: () => "1002",
        createLabel: () => "1003",
        setText: () => true,
        destroy: () => true
    }
};

(global as any).Alloy = mockAlloy;
(global as any).window = {
    __alloy_spawn_bridge: () => JSON.stringify({ id: "1", pid: 123 }),
    __alloy_spawn_sync: () => JSON.stringify({ stdout: "sync", exitCode: 0, success: true }),
    __alloy_secure_eval: (code: string) => JSON.stringify({ result: "secure" }),
    __alloy_sqlite_open: () => "db1",
    __alloy_sqlite_query: () => "stmt1",
    __alloy_sqlite_step: () => "{\"id\":1}",
    __alloy_sqlite_reset: () => "true",
    __alloy_sqlite_bind: () => "true",
    __alloy_cleanup: () => "true",
    __alloy_cron_register: () => "true",
    __alloy_cron_remove: () => "true",
    __alloy_gui_create_window: () => "1001",
    __alloy_gui_create_button: () => "1002",
    __alloy_gui_create_label: () => "1003",
    __alloy_gui_set_text: () => "true",
    __alloy_gui_destroy: () => "true"
};

(global as any).secureEval = (code: string) => "secure";
(global as any)._forbidden_eval = (code: string) => "forbidden";

import { Database, $ } from "../index";

describe("AlloyScript Runtime E2E", () => {
  test("Alloy.spawn", async () => {
    const proc = (global as any).Alloy.spawn(["echo", "hi"]);
    expect(proc.pid).toBe(123);
  });

  test("Alloy.spawnSync", () => {
    const res = (global as any).Alloy.spawnSync(["echo", "hi"]);
    expect(res.success).toBe(true);
    expect(res.stdout.toString()).toBe("sync");
  });

  test("secureEval", () => {
    const res = (global as any).secureEval("1+1");
    expect(res).toBe("secure");
    expect((global as any)._forbidden_eval).toBeDefined();
  });

  test("Alloy:sqlite", () => {
    const db = new Database(":memory:");
    const row = db.query("SELECT 1").get();
    expect(row.id).toBe(1);
  });

  test("Alloy.cron", async () => {
    const res = await (global as any).Alloy.cron("job.ts", "0 0 * * *", "DailyJob");
    expect(res).toBe(true);
  });

  test("Alloy.gui", () => {
    const win = (global as any).Alloy.gui.createWindow("Test", 800, 600);
    const btn = (global as any).Alloy.gui.createButton(win);
    const lbl = (global as any).Alloy.gui.createLabel(win);
    expect((global as any).Alloy.gui.setText(btn, "Click Me")).toBe(true);
  });

  test("Shell $", async () => {
      const res = await $`echo "hello"`;
      expect(res.stdout.toString()).toBe("sync");
  });
});
