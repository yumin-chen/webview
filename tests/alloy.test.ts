import { expect, test, describe } from "bun:test";

const mockWindow = {
    __alloy_spawn: async (h: string, c: string, o: string) => JSON.stringify({ pid: 1234 }),
    __alloy_spawnSync: async (c: string, o: string) => JSON.stringify({ success: true, stdout: btoa("hello"), stderr: "" }),
    __alloy_write: (h: string, d: string) => {},
    __alloy_closeStdin: (h: string) => {},
    __alloy_kill: (h: string, s: string) => {},
    __alloy_resize: (h: string, c: number, r: number) => {},
    __alloy_cleanup: (h: string) => {},
    __alloy_gui_create: (h: string, t: string, p: string) => {},
    __alloy_gui_append: (ph: string, ch: string) => {},
    __alloy_gui_set_text: (h: string, t: string) => {},
    __alloy_gui_set_value: (h: string, v: string) => {},
    __alloy_cron_register: async (p: string, s: string, t: string) => {},
    __alloy_cron_remove: async (t: string) => {},
};

(global as any).window = mockWindow;
(global as any).atob = (s: string) => Buffer.from(s, 'base64').toString('binary');
(global as any).btoa = (s: string) => Buffer.from(s, 'binary').toString('base64');
(global as any).TextEncoder = class { encode(s: string) { return Buffer.from(s); } };
(global as any).TextDecoder = class { decode(b: any) { return Buffer.from(b).toString(); } };
(global as any).ReadableStream = class {
    constructor(opts: any) { this._data = []; if (opts.start) opts.start({ enqueue: (v: any) => this._data.push(v), close: () => {} }); }
    _data: any[];
    async text() { return this._data.map(b => Buffer.from(b).toString()).join(''); }
    getReader() { let i = 0; return { read: async () => { if (i < this._data.length) return { done: false, value: this._data[i++] }; return { done: true, value: undefined }; } }; }
};

// Clear require cache to ensure runtime.ts runs again for this file
delete require.cache[require.resolve("../src/runtime.ts")];
require("../src/runtime.ts");
const Alloy = (window as any).Alloy;

describe("AlloyScript Comprehensive API", () => {
    describe("Spawn", () => {
        test("Alloy.spawn with array", () => {
            const proc = Alloy.spawn(["ls"]);
            expect(proc.handle).toMatch(/^proc_/);
        });

        test("Alloy.spawn with options object", () => {
            const proc = Alloy.spawn({ cmd: ["ls"], cwd: "/" });
            expect(proc.handle).toMatch(/^proc_/);
        });

        test("Alloy.spawnSync behavior", async () => {
            const res = await Alloy.spawnSync(["echo", "hi"]);
            expect(res.success).toBe(true);
        });
    });

    describe("GUI", () => {
        test("Component creation", () => {
            const win = Alloy.gui.Window({ title: "Test" });
            expect(win).toBeDefined();
            expect(Alloy._widgets[win.handle]).toBe(win);
        });

        test("Expanded components exist", () => {
            expect(typeof Alloy.gui.TextArea).toBe("function");
            expect(typeof Alloy.gui.CheckBox).toBe("function");
            expect(typeof Alloy.gui.Slider).toBe("function");
            expect(typeof Alloy.gui.ProgressBar).toBe("function");
            expect(typeof Alloy.gui.Switch).toBe("function");
        });
    });

    describe("Cron", () => {
        test("Alloy.cron.parse", () => {
            const from = new Date(Date.UTC(2025, 0, 1, 0, 0));
            const next = Alloy.cron.parse("0 0 * * *", from);
            expect(next.toISOString()).toBe("2025-01-02T00:00:00.000Z");
        });
    });
});
