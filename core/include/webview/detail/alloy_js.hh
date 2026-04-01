#ifndef WEBVIEW_DETAIL_ALLOY_JS_HH
#define WEBVIEW_DETAIL_ALLOY_JS_HH

#include <string>

namespace webview {
namespace detail {

static const std::string alloy_js_code = R"javascript(
(function() {
    if (window.Alloy) return;

    function wrapStream(stream) {
        if (!stream) return stream;
        stream.text = async function() {
            const reader = stream.getReader();
            let result = "";
            const decoder = new TextDecoder();
            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                result += decoder.decode(value, { stream: true });
            }
            result += decoder.decode();
            return result;
        };
        stream.json = async function() {
            const text = await stream.text();
            return JSON.parse(text);
        };
        stream.arrayBuffer = async function() {
            const reader = stream.getReader();
            const chunks = [];
            let length = 0;
            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                chunks.push(value);
                length += value.length;
            }
            const result = new Uint8Array(length);
            let offset = 0;
            for (const chunk of chunks) {
                result.set(chunk, offset);
                offset += chunk.length;
            }
            return result.buffer;
        };
        return stream;
    }

    class FileSink {
        constructor(id) {
            this.id = id;
            this.closed = false;
        }
        write(data) {
            if (this.closed) return;
            let payload = data;
            if (data instanceof Uint8Array || data instanceof ArrayBuffer || ArrayBuffer.isView(data)) {
                const bytes = new Uint8Array(data);
                let binary = "";
                for (let i = 0; i < bytes.byteLength; i++) {
                    binary += String.fromCharCode(bytes[i]);
                }
                payload = btoa(binary);
            }
            window.__alloy_write(this.id, payload);
        }
        flush() {}
        end() {
            this.closed = true;
        }
    }

    class Subprocess {
        constructor(id, options) {
            this.id = id;
            this.pid = -1;
            this.exitCode = null;
            this.signalCode = null;
            this.killed = false;
            this._resourceUsage = null;

            this._stdout_controller = null;
            this._stderr_controller = null;

            if (options.stdout !== "ignore" && options.stdout !== "inherit") {
                this.stdout = wrapStream(new ReadableStream({
                    start: (controller) => { this._stdout_controller = controller; }
                }));
            } else {
                this.stdout = null;
            }

            if (options.stderr !== "ignore" && options.stderr !== "inherit") {
                this.stderr = wrapStream(new ReadableStream({
                    start: (controller) => { this._stderr_controller = controller; }
                }));
            } else {
                this.stderr = null;
            }

            if (options.stdin === "pipe") {
                this.stdin = new FileSink(this.id);
            } else {
                this.stdin = null;
            }

            this.exited = new Promise((resolve) => {
                this._resolve_exit = resolve;
            });

            if (options.terminal) {
                this.terminal = new Terminal(this, options.terminal);
                this.stdout = null;
                this.stderr = null;
                this.stdin = null;
            }
        }

        kill(signal = 'SIGTERM') {
            window.__alloy_kill(this.id, String(signal));
            this.killed = true;
        }

        unref() {}
        ref() {}

        send(message) {
            const serialized = JSON.stringify(message);
            window.__alloy_write(this.id, "__ipc__:" + serialized);
        }

        disconnect() {
            window.__alloy_write(this.id, "__ipc_disconnect__");
        }

        resourceUsage() { return this._resourceUsage; }

        async [Symbol.asyncDispose]() {
            this.kill();
        }

        _onData(stream, data) {
            const b = atob(data);
            const bytes = new Uint8Array(b.length);
            for (let i = 0; i < b.length; i++) bytes[i] = b.charCodeAt(i);

            if (stream === 'stdout' && this._stdout_controller) {
                this._stdout_controller.enqueue(bytes);
            } else if (stream === 'stderr' && this._stderr_controller) {
                this._stderr_controller.enqueue(bytes);
            } else if (stream === 'terminal' && this.terminal && this.terminal._onData) {
                this.terminal._onData(bytes);
            }
        }

        _onExit(exitCode, resourceUsage) {
            this.exitCode = exitCode >= 0 ? exitCode : null;
            this.signalCode = exitCode < 0 ? "SIG" + (-exitCode) : null;
            this._resourceUsage = resourceUsage;

            if (this._stdout_controller) try { this._stdout_controller.close(); } catch(e){}
            if (this._stderr_controller) try { this._stderr_controller.close(); } catch(e){}

            this._resolve_exit(this.exitCode !== null ? this.exitCode : 0);
            if (this._onExitCallback) this._onExitCallback(this, this.exitCode, this.signalCode);
        }
    }

    class Terminal {
        constructor(subprocess, options) {
            this.subprocess = subprocess;
            this._onDataCallback = typeof options === 'object' ? options.data : null;
        }
        write(data) { window.__alloy_write(this.subprocess.id, data); }
        resize(cols, rows) { window.__alloy_resize(this.subprocess.id, String(cols), String(rows)); }
        setRawMode(enabled) {}
        ref() {}
        unref() {}
        close() { this.subprocess.kill(); }
        _onData(bytes) {
            if (this._onDataCallback) this._onDataCallback(this, bytes);
        }
        async [Symbol.asyncDispose]() { this.close(); }
    }

    const activeProcesses = new Map();
    const activeComponents = new Map();

    window.Alloy = {
        spawn: function(cmd, options = {}) {
            let actualCmd = cmd;
            let actualOpts = options;
            if (!Array.isArray(cmd) && typeof cmd === 'object') {
                actualCmd = cmd.cmd;
                actualOpts = cmd;
            }
            const id = Math.random().toString(36).substr(2, 9);
            const proc = new Subprocess(id, actualOpts);
            if (actualOpts.onExit) proc._onExitCallback = actualOpts.onExit;

            activeProcesses.set(id, proc);
            window.__alloy_spawn(id, JSON.stringify({...actualOpts, cmd: actualCmd})).then(pid => {
                proc.pid = Number(pid);
            });
            return proc;
        },
        spawnSync: function(cmd, options = {}) {
            let actualCmd = cmd;
            let actualOpts = options;
            if (!Array.isArray(cmd) && typeof cmd === 'object') {
                actualCmd = cmd.cmd;
                actualOpts = cmd;
            }
            const resultJson = window.__alloy_spawn_sync(JSON.stringify({...actualOpts, cmd: actualCmd}));
            const result = JSON.parse(resultJson);
            if (result.stdout) {
                const b = atob(result.stdout);
                const bytes = new Uint8Array(b.length);
                for(let i=0; i<b.length; i++) bytes[i] = b.charCodeAt(i);
                result.stdout = bytes;
            }
            if (result.stderr) {
                const b = atob(result.stderr);
                const bytes = new Uint8Array(b.length);
                for(let i=0; i<b.length; i++) bytes[i] = b.charCodeAt(i);
                result.stderr = bytes;
            }
            result.stdout = result.stdout || new Uint8Array(0);
            result.stderr = result.stderr || new Uint8Array(0);
            return result;
        },
        Terminal: class {
             constructor(options) {
                 this.id = Math.random().toString(36).substr(2, 9);
                 this.options = options;
             }
             write(data) {}
             resize(cols, rows) {}
             close() {}
             async [Symbol.asyncDispose]() { this.close(); }
        },
        file: function(path) { return { path: path }; }
    };

    window.__alloy_on_data = function(id, stream, data) {
        const proc = activeProcesses.get(id);
        if (proc) proc._onData(stream, data);
    };

    window.__alloy_on_exit = function(id, exitCode, resourceUsage) {
        const proc = activeProcesses.get(id);
        if (proc) {
            proc._onExit(exitCode, resourceUsage);
            activeProcesses.delete(id);
        }
    };

    window.__alloy_on_gui_event = function(handle, event) {
        const comp = activeComponents.get(handle);
        if (comp && comp._callbacks[event]) {
            comp._callbacks[event](comp, event);
        }
    };

    // SQLite
    class Statement {
        constructor(dbId, sql, cached = true) {
            this.dbId = dbId;
            this.sql = sql;
            this.id = Math.random().toString(36).substr(2, 9);
            this._cached = cached;
            const meta = window.__alloy_sqlite_prepare(dbId, this.id, sql, String(cached));
            const parsedMeta = JSON.parse(meta);
            this.columnNames = parsedMeta.columnNames;
            this.paramsCount = parsedMeta.paramsCount;
        }

        all(...params) {
            let p = params;
            if (params.length === 1 && typeof params[0] === 'object' && params[0] !== null && !Array.isArray(params[0])) {
                p = params[0];
            }
            const result = window.__alloy_sqlite_all(this.id, JSON.stringify(p));
            return JSON.parse(result);
        }

        get(...params) {
            let p = params;
            if (params.length === 1 && typeof params[0] === 'object' && params[0] !== null && !Array.isArray(params[0])) {
                p = params[0];
            }
            const result = window.__alloy_sqlite_get(this.id, JSON.stringify(p));
            return JSON.parse(result);
        }

        run(...params) {
            let p = params;
            if (params.length === 1 && typeof params[0] === 'object' && params[0] !== null && !Array.isArray(params[0])) {
                p = params[0];
            }
            const result = window.__alloy_sqlite_run(this.id, JSON.stringify(p));
            return JSON.parse(result);
        }

        values(...params) {
            let p = params;
            if (params.length === 1 && typeof params[0] === 'object' && params[0] !== null && !Array.isArray(params[0])) {
                p = params[0];
            }
            const result = window.__alloy_sqlite_values(this.id, JSON.stringify(p));
            return JSON.parse(result);
        }

        finalize() {
            window.__alloy_sqlite_finalize(this.id);
        }

        toString() {
            return window.__alloy_sqlite_stmt_to_string(this.id);
        }
    }

    class Database {
        constructor(filename = ':memory:', options = {}) {
            this.id = Math.random().toString(36).substr(2, 9);
            window.__alloy_sqlite_open(this.id, filename, JSON.stringify(options));
        }

        query(sql) { return new Statement(this.id, sql, true); }
        prepare(sql) { return new Statement(this.id, sql, false); }

        exec(sql) {
            window.__alloy_sqlite_exec(this.id, sql);
        }

        serialize() {
            const b64 = window.__alloy_sqlite_serialize(this.id);
            const b = atob(b64);
            const bytes = new Uint8Array(b.length);
            for(let i=0; i<b.length; i++) bytes[i] = b.charCodeAt(i);
            return bytes;
        }

        close() { window.__alloy_sqlite_close(this.id); }
    }

    window.Alloy.sqlite = { Database };

    // GUI
    class Signal {
        constructor(initial) {
            this.id = Math.random().toString(36).substr(2, 9);
            window.__alloy_signal_create_str(this.id, String(initial));
        }
        set(v) { window.__alloy_signal_set_str(this.id, String(v)); }
    }

    class Component {
        constructor(handle) {
            this.handle = handle;
            this._callbacks = {};
            activeComponents.set(handle, this);
        }
        setText(text) { window.__alloy_gui_set_text(this.handle, text); }
        addChild(child) { window.__alloy_gui_add_child(this.handle, child.handle); }
        bind(prop, signal) { window.__alloy_gui_bind_property(this.handle, String(prop), signal.id); }
        on(event, callback) {
            this._callbacks[event] = callback;
        }
    }

    const guiProxy = {
        get(target, prop) {
            if (prop in target) return target[prop];
            if (prop === 'createWindow') {
                return (title, w, h) => {
                    return window.__alloy_gui_create_window(title, String(w), String(h)).then(handle => new Component(handle));
                };
            }
            if (prop.startsWith('create')) {
                const type = prop.slice(6).toLowerCase();
                return (parent) => {
                    const parentHandle = (parent && parent instanceof Component) ? parent.handle : "";
                    return window.__alloy_gui_create_component(type, String(parentHandle)).then(handle => new Component(handle));
                };
            }
        }
    };

    window.Alloy.gui = new Proxy({
        Signal: Signal,
        Events: { CLICK: 0, CHANGE: 1, CLOSE: 2 },
        Props: { TEXT: 0, CHECKED: 1, VALUE: 2, ENABLED: 3, VISIBLE: 4, LABEL: 5, TITLE: 6 }
    }, guiProxy);
})();
)javascript";

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOY_JS_HH
