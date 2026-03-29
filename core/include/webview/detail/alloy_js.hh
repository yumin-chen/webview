#ifndef WEBVIEW_DETAIL_ALLOY_JS_HH
#define WEBVIEW_DETAIL_ALLOY_JS_HH

#include <string>

namespace webview {
namespace detail {

static const std::string alloy_js_code = R"javascript(
(function() {
    if (window.Alloy) return;

    class Subprocess {
        constructor(id, options) {
            this.id = id;
            this.pid = -1;
            this.exitCode = null;
            this.signalCode = null;
            this.killed = false;

            this._stdout_controller = null;
            this._stderr_controller = null;

            this.stdout = new ReadableStream({
                start: (controller) => { this._stdout_controller = controller; }
            });
            this.stderr = new ReadableStream({
                start: (controller) => { this._stderr_controller = controller; }
            });

            this.exited = new Promise((resolve) => {
                this._resolve_exit = resolve;
            });

            if (options.terminal) {
                this.terminal = new Terminal(this);
                this.stdout = null;
                this.stderr = null;
            }
        }

        kill(signal = 'SIGTERM') {
            window.__alloy_kill(this.id, signal);
            this.killed = true;
        }

        unref() {}
        ref() {}
        send(message) {}
        disconnect() {}
        resourceUsage() { return this._resourceUsage; }

        _onData(stream, data) {
            const bytes = new Uint8Array(atob(data).split("").map(c => c.charCodeAt(0)));
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
            this.signalCode = exitCode < 0 ? -exitCode : null;
            this._resourceUsage = resourceUsage;

            if (this._stdout_controller) this._stdout_controller.close();
            if (this._stderr_controller) this._stderr_controller.close();

            this._resolve_exit(this.exitCode !== null ? this.exitCode : 0);
        }
    }

    class Terminal {
        constructor(subprocess) {
            this.subprocess = subprocess;
        }
        write(data) { window.__alloy_write(this.subprocess.id, data); }
        resize(cols, rows) { window.__alloy_resize(this.subprocess.id, cols, rows); }
        setRawMode(enabled) {}
        close() { this.subprocess.kill(); }
    }

    const activeProcesses = new Map();

    window.Alloy = {
        spawn: function(cmd, options = {}) {
            if (Array.isArray(cmd)) { options.cmd = cmd; }
            else if (typeof cmd === 'object') { options = cmd; }
            const id = Math.random().toString(36).substr(2, 9);
            const proc = new Subprocess(id, options);
            activeProcesses.set(id, proc);
            window.__alloy_spawn(id, JSON.stringify(options)).then(pid => { proc.pid = pid; });
            return proc;
        },
        spawnSync: function(cmd, options = {}) {
            if (Array.isArray(cmd)) { options.cmd = cmd; }
            else if (typeof cmd === 'object') { options = cmd; }
            const resultJson = window.__alloy_spawn_sync(JSON.stringify(options));
            const result = JSON.parse(resultJson);
            if (result.stdout) result.stdout = new Uint8Array(atob(result.stdout).split("").map(c => c.charCodeAt(0)));
            if (result.stderr) result.stderr = new Uint8Array(atob(result.stderr).split("").map(c => c.charCodeAt(0)));
            return result;
        }
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

    class Statement {
        constructor(dbId, sql) {
            this.dbId = dbId;
            this.id = Math.random().toString(36).substr(2, 9);
            window.__alloy_sqlite_prepare(dbId, this.id, sql);
        }
        get(...params) {
            const result = window.__alloy_sqlite_step(this.id);
            const row = JSON.parse(result);
            window.__alloy_sqlite_finalize(this.id);
            return row;
        }
        all(...params) {
            const rows = [];
            while (true) {
                const result = window.__alloy_sqlite_step(this.id);
                const row = JSON.parse(result);
                if (row === null) break;
                rows.push(row);
            }
            window.__alloy_sqlite_finalize(this.id);
            return rows;
        }
        run(...params) {
            window.__alloy_sqlite_step(this.id);
            window.__alloy_sqlite_finalize(this.id);
            return { lastInsertRowid: 0, changes: 0 };
        }
        finalize() { window.__alloy_sqlite_finalize(this.id); }
    }

    class Database {
        constructor(filename = ':memory:', options = {}) {
            this.id = Math.random().toString(36).substr(2, 9);
            window.__alloy_sqlite_open(this.id, filename, options.flags || "");
        }
        query(sql) { return new Statement(this.id, sql); }
        prepare(sql) { return new Statement(this.id, sql); }
        run(sql, params) { return this.query(sql).run(params); }
        close() { window.__alloy_sqlite_close(this.id); }
    }

    window.Alloy.sqlite = { Database };
})();
)javascript";

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOY_JS_HH
