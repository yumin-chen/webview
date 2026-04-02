#ifndef WEBVIEW_META_JS_HH
#define WEBVIEW_META_JS_HH

#include <string>

namespace webview {

const std::string META_JS = R"js(
(function() {
    'use strict';

    if (window.meta) return;

    class Subprocess {
        constructor(pid, options = {}) {
            this.pid = pid;
            this.killed = false;
            this.exitCode = null;
            this.signalCode = null;
            this._options = options;
            this._exitedPromise = new Promise(resolve => {
                this._resolveExited = resolve;
            });
            this.stdout = options.stdout === 'ignore' ? null : new ReadableStream({
                start: (controller) => { this._stdoutController = controller; }
            });
            this.stderr = options.stderr === 'ignore' ? null : new ReadableStream({
                start: (controller) => { this._stderrController = controller; }
            });
            this.terminal = options.terminal ? {
                write: (data) => window.__webview__.call('meta_write', this.pid, data),
                resize: (cols, rows) => {},
                close: () => this.kill()
            } : undefined;

            this.stdin = (options.stdin === 'pipe' && !options.terminal) ? {
                write: (data) => window.__webview__.call('meta_write', this.pid, data),
                flush: () => {},
                end: () => {}
            } : null;
        }

        get exited() { return this._exitedPromise; }

        kill(signal = 'SIGTERM') {
            window.__webview__.call('meta_kill', this.pid, signal);
            this.killed = true;
        }
    }

    const _subprocesses = new Map();

    window.meta = {
        spawn: async function(cmd, options = {}) {
            const res = await window.__webview__.call('meta_spawn', cmd, JSON.stringify(options));
            const data = JSON.parse(res);
            if (data.error) throw new Error(data.error);
            const proc = new Subprocess(data.pid, options);
            _subprocesses.set(data.pid, proc);
            return proc;
        },

        spawnSync: function(cmd, options = {}) {
            const msg = JSON.stringify({method: 'meta_spawnSync', params: [cmd, JSON.stringify(options)], id: 'sync'});
            const resRaw = prompt('__webview_sync__:' + msg);
            if (!resRaw) return { pid: -1, success: false, exitCode: -1 };
            const data = JSON.parse(resRaw);
            if (data.error) throw new Error(data.error);
            return {
                pid: data.pid,
                stdout: data.stdout,
                stderr: data.stderr,
                exitCode: data.exitCode,
                success: data.exitCode === 0
            };
        },

        __onData: function(pid, stream, data) {
            const proc = _subprocesses.get(pid);
            if (!proc) return;
            const controller = stream === 'stdout' ? proc._stdoutController : (stream === 'stderr' ? proc._stderrController : null);
            if (controller) {
                controller.enqueue(new TextEncoder().encode(data));
            } else if (stream === 'terminal' && proc._options.terminal && typeof proc._options.terminal.data === 'function') {
                proc._options.terminal.data(proc.terminal, new TextEncoder().encode(data));
            }
        },

        __onExit: function(pid, exitCode) {
            const proc = _subprocesses.get(pid);
            if (!proc) return;
            proc.exitCode = exitCode;
            if (proc._stdoutController) proc._stdoutController.close();
            if (proc._stderrController) proc._stderrController.close();
            if (proc._options.onExit) proc._options.onExit(proc, exitCode, null);
            proc._resolveExited(exitCode);
            _subprocesses.delete(pid);
        }
    };

    if (ReadableStream.prototype.text === undefined) {
        ReadableStream.prototype.text = async function() {
            const reader = this.getReader();
            let result = '';
            const decoder = new TextDecoder();
            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                result += decoder.decode(value);
            }
            return result;
        };
    }
})();
)js";

} // namespace webview

#endif // WEBVIEW_META_JS_HH
