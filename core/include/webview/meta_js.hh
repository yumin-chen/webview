#ifndef WEBVIEW_META_JS_HH
#define WEBVIEW_META_JS_HH

namespace webview {
namespace detail {

const char* meta_bootstrap_js = R"js(
(function() {
    'use strict';

    function generateId() {
        var crypto = window.crypto || window.msCrypto;
        var bytes = new Uint8Array(16);
        crypto.getRandomValues(bytes);
        return Array.prototype.slice.call(bytes).map(function(n) {
            var s = n.toString(16);
            return ((s.length % 2) == 1 ? '0' : '') + s;
        }).join('');
    }

    const _promises = {};

    function callNative(method, ...params) {
        const id = generateId();
        const promise = new Promise((resolve, reject) => {
            _promises[id] = { resolve, reject };
        });
        window.__webview__.post(JSON.stringify({
            id: id,
            method: method,
            params: params
        }));
        return promise;
    }

    const _originalOnReply = window.__webview__.onReply;
    window.__webview__.onReply = function(id, status, result) {
        if (_promises[id]) {
            if (status === 0) _promises[id].resolve(result);
            else _promises[id].reject(result);
            delete _promises[id];
        } else if (_originalOnReply) {
            _originalOnReply.apply(this, arguments);
        }
    };

    class Terminal {
        constructor(id, options) {
            this.id = id;
            this.options = options;
            this.closed = false;
        }
        write(data) { callNative('__meta_terminal_write', this.id, data); }
        resize(cols, rows) { callNative('__meta_terminal_resize', this.id, cols, rows); }
        setRawMode(enabled) { callNative('__meta_terminal_set_raw_mode', this.id, enabled); }
        close() { callNative('__meta_terminal_close', this.id); this.closed = true; }
        ref() {}
        unref() {}
    }

    class Subprocess {
        constructor(id, options) {
            this.id = id;
            this.options = options;
            this.pid = -1;
            this.exitCode = null;
            this.signalCode = null;
            this.killed = false;
            this._resourceUsage = null;
            this.exited = new Promise(resolve => { this._exited_resolve = resolve; });

            if (options.terminal) {
                this.terminal = new Terminal(id, options.terminal);
            } else {
                if (options.stdout !== 'inherit' && options.stdout !== 'ignore') {
                    this.stdout = new ReadableStream({ start: (c) => { this._stdout_controller = c; } });
                }
                if (options.stderr === 'pipe') {
                    this.stderr = new ReadableStream({ start: (c) => { this._stderr_controller = c; } });
                }

                if (options.stdin instanceof ReadableStream) {
                    this._pipeStdin(options.stdin);
                } else if (options.stdin === 'pipe') {
                    this.stdin = {
                        write: (data) => { callNative('__meta_stdin_write', this.id, data); },
                        end: () => { callNative('__meta_stdin_close', this.id); },
                        flush: () => {}
                    };
                }
            }
        }

        async _pipeStdin(stream) {
            const reader = stream.getReader();
            try {
                while (true) {
                    const { done, value } = await reader.read();
                    if (done) break;
                    await callNative('__meta_stdin_write', this.id, value);
                }
            } finally {
                await callNative('__meta_stdin_close', this.id);
            }
        }

        kill(signal = 'SIGTERM') { callNative('__meta_kill', this.id, signal); this.killed = true; }
        send(message) { callNative('__meta_ipc_send', this.id, JSON.stringify(message)); }
        disconnect() { callNative('__meta_ipc_disconnect', this.id); }
        resourceUsage() { return this._resourceUsage; }
        unref() {}
        ref() {}
    }

    const _subprocesses = new Map();

    window.meta = {
        spawn: function(cmd, options = {}) {
            let actualCmd = Array.isArray(cmd) ? cmd : (cmd.cmd || [cmd]);
            let opts = Object.assign({ stdout: 'pipe', stderr: 'inherit', stdin: 'none', serialization: 'advanced' },
                                     typeof cmd === 'object' && !Array.isArray(cmd) ? cmd : options);

            const procId = generateId();
            const proc = new Subprocess(procId, opts);
            _subprocesses.set(procId, proc);

            let nativeStdin = opts.stdin;
            if (opts.stdin instanceof ReadableStream) nativeStdin = 'pipe';

            callNative('__meta_spawn', procId, actualCmd, Object.assign({}, opts, { stdin: nativeStdin })).then(result => {
                if (result) {
                    const parsed = JSON.parse(result);
                    if (parsed.pid) proc.pid = parsed.pid;
                }
            });
            return proc;
        },

        spawnSync: function(cmd, options = {}) {
             // Sync implementation would require synchronous C++ binding.
             // Currently returning a promise for compatibility.
             let actualCmd = Array.isArray(cmd) ? cmd : (cmd.cmd || [cmd]);
             let opts = Object.assign({}, typeof cmd === 'object' && !Array.isArray(cmd) ? cmd : options);
             return callNative('__meta_spawnSync', actualCmd, opts).then(res => JSON.parse(res));
        },

        __on_data: function(id, stream, data, isBase64) {
            const proc = _subprocesses.get(id);
            if (!proc) return;

            let finalData = data;
            if (isBase64) {
                const bin = atob(data);
                const bytes = new Uint8Array(bin.length);
                for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
                finalData = bytes;
            }

            if (stream === 'ipc') {
                if (proc.options.ipc) {
                    const str = new TextDecoder().decode(finalData);
                    try { proc.options.ipc(JSON.parse(str), proc); } catch(e) { proc.options.ipc(str, proc); }
                }
            } else if (proc.terminal) {
                if (proc.options.terminal.data) proc.options.terminal.data(proc.terminal, finalData);
            } else {
                const c = stream === 'stdout' ? proc._stdout_controller : proc._stderr_controller;
                if (c) c.enqueue(finalData);
            }
        },

        __on_exit: function(id, exitCode, usage) {
            const proc = _subprocesses.get(id);
            if (!proc) return;
            proc.exitCode = exitCode;
            proc._resourceUsage = usage;
            if (proc._stdout_controller) proc._stdout_controller.close();
            if (proc._stderr_controller) proc._stderr_controller.close();
            proc._exited_resolve(exitCode);
            if (proc.options.onExit) proc.options.onExit(proc, exitCode, null);
        }
    };

    if (!ReadableStream.prototype.text) {
        ReadableStream.prototype.text = async function() {
            const reader = this.getReader();
            let res = '', dec = new TextDecoder();
            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                res += dec.decode(value);
            }
            return res;
        };
    }
    window.meta.Terminal = Terminal;
})();
)js";

} // namespace detail
} // namespace webview

#endif // WEBVIEW_META_JS_HH
