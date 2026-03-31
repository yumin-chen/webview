import { expect, test, mock, describe, beforeAll } from "bun:test";

// Mock the globals that the bridge expects
globalThis.window = globalThis as any;
globalThis.atob = (s: string) => Buffer.from(s, 'base64').toString('binary');
globalThis.btoa = (s: string) => Buffer.from(s, 'binary').toString('base64');

// Mock the backend calls
const mockBackend = {
    __alloy_spawn: mock(() => Promise.resolve("1234")),
    __alloy_spawn_sync: mock(() => JSON.stringify({
        stdout: btoa("hello\n"),
        stderr: "",
        exitCode: 0,
        success: true,
        pid: 1235,
        resourceUsage: { maxRSS: 1024, cpuTime: { user: 100, system: 50 } }
    })),
    __alloy_write: mock(() => {}),
    __alloy_kill: mock(() => {}),
    __alloy_sqlite_open: mock(() => {}),
    __alloy_sqlite_prepare: mock(() => JSON.stringify({ columnNames: ["id", "name"], paramsCount: 0 })),
    __alloy_sqlite_all: mock(() => JSON.stringify([{ id: 1, name: "test" }])),
    __alloy_gui_create_window: mock(() => Promise.resolve("999")),
    __alloy_gui_create_component: mock(() => Promise.resolve("1000")),
    __alloy_signal_create_str: mock(() => {}),
};

Object.assign(globalThis, mockBackend);

// Load the bridge code
const fs = require('fs');
const alloyJsPath = 'core/include/webview/detail/alloy_js.hh';
const content = fs.readFileSync(alloyJsPath, 'utf8');
const jsCodeMatch = content.match(/R"javascript\(([\s\S]*?)\)javascript"/);
if (jsCodeMatch) {
    eval(jsCodeMatch[1]);
}

describe("Alloy JS Bridge", () => {
    test("Alloy.spawn", async () => {
        // @ts-ignore
        const proc = Alloy.spawn(["echo", "hi"]);
        expect(proc).toBeDefined();
        expect(proc.exited).toBeDefined();
        // Wait for pid to be set (it's async in the mock)
        await new Promise(resolve => setTimeout(resolve, 10));
        expect(proc.pid).toBe(1234);
        expect(mockBackend.__alloy_spawn).toHaveBeenCalled();
    });

    test("Alloy.spawnSync", () => {
        // @ts-ignore
        const result = Alloy.spawnSync(["echo", "hi"]);
        expect(result.stdout).toBeInstanceOf(Uint8Array);
        expect(new TextDecoder().decode(result.stdout)).toBe("hello\n");
        expect(result.success).toBe(true);
    });

    test("Alloy.sqlite", () => {
        // @ts-ignore
        const db = new Alloy.sqlite.Database("test.db");
        expect(mockBackend.__alloy_sqlite_open).toHaveBeenCalled();
        const stmt = db.query("SELECT * FROM users");
        expect(stmt.columnNames).toEqual(["id", "name"]);
        const rows = stmt.all();
        expect(rows).toEqual([{ id: 1, name: "test" }]);
    });

    test("Alloy.gui", async () => {
        // @ts-ignore
        const win = await Alloy.gui.createWindow("Title", 800, 600);
        expect(win).toBeDefined();
        expect(win.handle).toBe("999");

        // createButton should work via Proxy
        // @ts-ignore
        const btn = await Alloy.gui.createButton(win);
        expect(btn).toBeDefined();
        expect(btn.handle).toBe("1000");
        expect(mockBackend.__alloy_gui_create_component).toHaveBeenCalledWith("button", "999");
    });
});
