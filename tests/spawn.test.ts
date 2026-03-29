import { expect, test, describe } from "bun:test";

// Mock the window environment
const mockWindow = {
    __meta_spawn: async (h: string, c: string, o: string) => JSON.stringify({ pid: 1234 }),
    __meta_spawnSync: async (c: string, o: string) => JSON.stringify({ success: true, stdout: btoa("hello"), stderr: "" }),
    __meta_write: (h: string, d: string) => {},
    __meta_closeStdin: (h: string) => {},
    __meta_kill: (h: string, s: string) => {},
    __meta_resize: (h: string, c: number, r: number) => {},
    __meta_cleanup: (h: string) => {},
    __meta_gui_create: (h: string, t: string, p: string) => {},
    __meta_gui_append: (ph: string, ch: string) => {},
    __meta_gui_set_text: (h: string, t: string) => {},
};

(global as any).window = mockWindow;
(global as any).atob = (s: string) => Buffer.from(s, 'base64').toString('binary');
(global as any).btoa = (s: string) => Buffer.from(s, 'binary').toString('base64');
(global as any).TextEncoder = class { encode(s: string) { return Buffer.from(s); } };
(global as any).TextDecoder = class { decode(b: any) { return Buffer.from(b).toString(); } };
(global as any).ReadableStream = class {
    constructor(opts: any) {
        this._data = [];
        if (opts.start) opts.start({ enqueue: (v: any) => this._data.push(v), close: () => {} });
    }
    _data: any[];
    async text() { return this._data.map(b => Buffer.from(b).toString()).join(''); }
    getReader() {
        let i = 0;
        return {
            read: async () => {
                if (i < this._data.length) return { done: false, value: this._data[i++] };
                return { done: true, value: undefined };
            }
        };
    }
};

// Import the runtime
require("../src/runtime.ts");

const meta = (window as any).meta;

describe("MetaScript Comprehensive Tests", () => {
    test("meta.spawn with handle", () => {
        const proc = meta.spawn(["ls"]);
        expect(proc.handle).toMatch(/^proc_/);
    });

    test("meta.gui component handling", () => {
        const win = meta.gui.Window({ title: "Test" });
        expect(win.handle).toMatch(/^gui_/);
        expect(meta._widgets[win.handle]).toBe(win);
    });

    test("meta.cron.parse correctness", () => {
        const from = new Date(Date.UTC(2025, 0, 1, 10, 0));
        const next = meta.cron.parse("0 11 * * *", from);
        expect(next.toISOString()).toBe("2025-01-01T11:00:00.000Z");
    });

    test("Binary safe data transfer via b64", async () => {
        const proc = meta.spawn(["cat"]);
        const raw = "\x00\xFF\xAA\x55";
        meta._onData(proc.handle, 'stdout', btoa(raw));
        meta._onExit(proc.handle, 0, 0);
        const out = await proc.stdout.getReader().read();
        expect(out.value[0]).toBe(0);
        expect(out.value[1]).toBe(255);
    });
});
