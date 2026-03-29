#ifndef WEBVIEW_DETAIL_META_JS_HH
#define WEBVIEW_DETAIL_META_JS_HH

#include <string>

namespace webview {
namespace detail {

static const std::string meta_js = R"js(
(function() {
  'use strict';

  class Subprocess {
    constructor(pid, options) {
      this.pid = pid;
      this.options = options || {};
      this._exited_resolve = null;
      this.exited = new Promise(resolve => {
        this._exited_resolve = resolve;
      });
      this.exitCode = null;
      this.signalCode = null;
      this.killed = false;

      if (this.options.terminal) {
        this.terminal = new Terminal(this);
        this.stdin = null;
        this.stdout = null;
        this.stderr = null;
      } else {
        this.terminal = null;
        this._stdout_controller = null;
        this.stdout = new ReadableStream({
          start: (controller) => { this._stdout_controller = controller; }
        });
        this._stderr_controller = null;
        this.stderr = new ReadableStream({
          start: (controller) => { this._stderr_controller = controller; }
        });
        this.stdin = {
          write: (data) => { window.meta._write(this.pid, data); },
          end: () => { window.meta._closeStdin(this.pid); },
          flush: () => {}
        };
      }
    }

    kill(signal) {
      this.killed = true;
      window.meta._kill(this.pid, signal || 'SIGTERM');
    }

    ref() {}
    unref() {}
  }

  class Terminal {
    constructor(proc) {
      this.proc = proc;
      this.closed = false;
    }
    write(data) {
      window.meta._write(this.proc.pid, data);
    }
    resize(cols, rows) {
      window.meta._resize(this.proc.pid, cols, rows);
    }
    setRawMode(enabled) {}
    close() {
      this.closed = true;
    }
    ref() {}
    unref() {}
  }

  window.meta = {
    _processes: {},

    spawn: async function(command, options) {
      if (Array.isArray(command)) {
        // ok
      } else if (typeof command === 'object' && command.cmd) {
        options = command;
        command = command.cmd;
      }

      const res_json = await window.__meta_spawn(command, JSON.stringify(options || {}));
      const res = JSON.parse(res_json);
      if (res.error) throw new Error(res.error);

      const proc = new Subprocess(res.pid, options);
      this._processes[res.pid] = proc;
      return proc;
    },

    spawnSync: async function(command, options) {
      if (Array.isArray(command)) {
        // ok
      } else if (typeof command === 'object' && command.cmd) {
        options = command;
        command = command.cmd;
      }
      const res_json = await window.__meta_spawnSync(command, JSON.stringify(options || {}));
      return JSON.parse(res_json);
    },

    _onData: function(pid, type, data) {
      const proc = this._processes[pid];
      if (!proc) return;
      if (type === 'stdout' && proc._stdout_controller) {
        proc._stdout_controller.enqueue(new TextEncoder().encode(data));
      } else if (type === 'stderr' && proc._stderr_controller) {
        proc._stderr_controller.enqueue(new TextEncoder().encode(data));
      } else if (type === 'terminal' && proc.options.terminal && proc.options.terminal.data) {
        proc.options.terminal.data(proc.terminal, new TextEncoder().encode(data));
      }
    },

    _onExit: function(pid, exitCode, signalCode) {
      const proc = this._processes[pid];
      if (!proc) return;
      proc.exitCode = exitCode;
      proc.signalCode = signalCode;
      if (proc._stdout_controller) proc._stdout_controller.close();
      if (proc._stderr_controller) proc._stderr_controller.close();
      if (proc.options.onExit) {
        proc.options.onExit(proc, exitCode, signalCode);
      }
      if (proc.options.terminal && proc.options.terminal.exit) {
        proc.options.terminal.exit(proc.terminal, 0, null);
      }
      proc._exited_resolve(exitCode);
    },

    _write: function(pid, data) {
      window.__meta_write(pid, typeof data === 'string' ? data : new TextDecoder().decode(data));
    },
    _closeStdin: function(pid) {
      window.__meta_closeStdin(pid);
    },
    _kill: function(pid, signal) {
      window.__meta_kill(pid, signal);
    },
    _resize: function(pid, cols, rows) {
      window.__meta_resize(pid, cols, rows);
    }
  };

  // Polyfill for text() on ReadableStream if needed
  if (!ReadableStream.prototype.text) {
    ReadableStream.prototype.text = async function() {
      const reader = this.getReader();
      let decoder = new TextDecoder();
      let result = '';
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        result += decoder.decode(value, { stream: true });
      }
      result += decoder.decode();
      return result;
    };
  }

})();
)js";

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_META_JS_HH
