import { expect, test, describe } from "bun:test";

// Mock the window environment
const mockWindow = {
    __meta_spawn: async (handle: string, cmd: string, opts: string) => JSON.stringify({ pid: 1234 }),
    __meta_spawnSync: async (cmd: string, opts: string) => JSON.stringify({ success: true, stdout: btoa("hello"), stderr: "" }),
    __meta_write: (handle: string, data: string) => {},
    __meta_closeStdin: (handle: string) => {},
    __meta_kill: (handle: string, sig: string) => {},
    __meta_resize: (handle: string, c: number, r: number) => {},
    __meta_cleanup: (handle: string) => {},
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

describe("MetaScript Runtime JS Logic", () => {
    test("meta.spawn returns a Subprocess object", () => {
        const proc = meta.spawn(["ls"]);
        expect(proc).toBeDefined();
        expect(proc.handle).toMatch(/^proc_/);
    });

    test("meta.spawnSync returns process result", async () => {
        const res = await meta.spawnSync(["echo", "hello"]);
        const data = JSON.parse(res); // meta.spawnSync returns raw string from __meta_spawnSync
        expect(data.success).toBe(true);
        expect(new TextDecoder().decode(b64ToUint8(data.stdout))).toBe("hello");
    });

    function b64ToUint8(b64: string): Uint8Array {
        const bin = atob(b64);
        const bytes = new Uint8Array(bin.length);
        for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
        return bytes;
    }

    test("Cron Parser logic", () => {
        const from = new Date(Date.UTC(2025, 0, 1, 0, 0));
        const next = meta.cron.parse("0 0 * * *", from);
        expect(next.toISOString()).toBe("2025-01-02T00:00:00.000Z");
    });

    test("Subprocess stdout streaming", async () => {
        const proc = meta.spawn(["cat"]);
        const data = "hello world";
        meta._onData(proc.handle, 'stdout', btoa(data));
        meta._onExit(proc.handle, 0, 0);
        const text = await proc.stdout.text();
        expect(text).toBe(data);
    });
});
