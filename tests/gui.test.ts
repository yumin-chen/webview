import { expect, test, describe } from "bun:test";

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
    constructor(opts: any) { this._data = []; if (opts.start) opts.start({ enqueue: (v: any) => this._data.push(v), close: () => {} }); }
    _data: any[];
    async text() { return this._data.map(b => Buffer.from(b).toString()).join(''); }
    getReader() { let i = 0; return { read: async () => { if (i < this._data.length) return { done: false, value: this._data[i++] }; return { done: true, value: undefined }; } }; }
};

require("../src/runtime.ts");
const meta = (window as any).meta;

describe("MetaScript GUI Runtime", () => {
    test("meta.gui component creation and handle registration", () => {
        const win = meta.gui.Window({ title: "My App" });
        expect(win).toBeDefined();
        expect(meta._widgets[win.handle]).toBe(win);
    });

    test("meta.gui event routing", () => {
        let clicked = false;
        const btn = meta.gui.Button({ label: "Click" });
        btn.addEventListener('click', () => { clicked = true; });
        meta.gui._onEvent(btn.handle, 'click');
        expect(clicked).toBe(true);
    });

    test("meta.gui set text", () => {
        const lbl = meta.gui.Label({ text: "Initial" });
        lbl.setText("Updated");
        // Verify via spy or mock if needed, but here we just ensure no crash
    });
});
