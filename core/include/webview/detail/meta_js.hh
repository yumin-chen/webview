#ifndef WEBVIEW_DETAIL_META_JS_HH
#define WEBVIEW_DETAIL_META_JS_HH

#include <string>

namespace webview {
namespace detail {

static const std::string meta_js = R"js(
(function() {
  'use strict';

  function b64ToUint8(b64) {
    const bin = atob(b64);
    const bytes = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
    return bytes;
  }

  function uint8ToB64(uint8) {
    let bin = '';
    const len = uint8.byteLength;
    for (let i = 0; i < len; i++) bin += String.fromCharCode(uint8[i]);
    return btoa(bin);
  }

  class Subprocess {
    constructor(handle, options) {
      this.handle = handle;
      this.pid = null;
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
          write: (data) => { window.meta._write(this.handle, data); },
          end: () => { window.meta._closeStdin(this.handle); },
          flush: () => {}
        };
      }
    }

    kill(signal) {
      this.killed = true;
      window.meta._kill(this.handle, signal || 'SIGTERM');
    }

    ref() {}
    unref() {}

    resourceUsage() {
      return { maxRSS: 0, cpuTime: { user: 0, system: 0 } };
    }
  }

  class Terminal {
    constructor(options_or_proc) {
      if (options_or_proc instanceof Subprocess) {
        this.proc = options_or_proc;
        this.handle = this.proc.handle;
      } else {
        this.options = options_or_proc || {};
        this.handle = "term_" + (++window.meta._handleCounter);
        // This mode would need a way to spawn without meta.spawn,
        // but for now let's just make it a holder.
      }
      this.closed = false;
    }
    write(data) {
      window.meta._write(this.handle, data);
    }
    resize(cols, rows) {
      window.meta._resize(this.handle, cols, rows);
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
    _handleCounter: 0,
    Terminal: Terminal,

    spawn: function(command, options) {
      if (!Array.isArray(command)) {
        if (typeof command === 'object' && command.cmd) {
          options = command;
          command = command.cmd;
        }
      }

      const handle = "proc_" + (++this._handleCounter);
      const proc = new Subprocess(handle, options);
      this._processes[handle] = proc;

      (async () => {
        try {
          const res = await window.__meta_spawn(handle, JSON.stringify(command), JSON.stringify(options || {}));
          if (res.error) {
             console.error("meta.spawn error:", res.error);
             return;
          }
          proc.pid = res.pid;
        } catch (e) {
          console.error("meta.spawn failed:", e);
        }
      })();

      return proc;
    },

    spawnSync: function(command, options) {
      if (!Array.isArray(command)) {
        if (typeof command === 'object' && command.cmd) {
          options = command;
          command = command.cmd;
        }
      }
      // Return a Promise as a fallback since true sync is not available.
      return (async () => {
        const res = await window.__meta_spawnSync(JSON.stringify(command), JSON.stringify(options || {}));
        if (res.stdout) res.stdout = b64ToUint8(res.stdout);
        if (res.stderr) res.stderr = b64ToUint8(res.stderr);
        return res;
      })();
    },

    _onData: function(handle, type, data_b64) {
      const proc = this._processes[handle];
      if (!proc) return;
      const encoded = b64ToUint8(data_b64);
      if (type === 'stdout' && proc._stdout_controller) {
        proc._stdout_controller.enqueue(encoded);
      } else if (type === 'stderr' && proc._stderr_controller) {
        proc._stderr_controller.enqueue(encoded);
      } else if (type === 'terminal' && proc.options.terminal && proc.options.terminal.data) {
        proc.options.terminal.data(proc.terminal, encoded);
      }
    },

    _onExit: function(handle, exitCode, signalCode) {
      const proc = this._processes[handle];
      if (!proc) return;
      proc.exitCode = exitCode;
      proc.signalCode = signalCode;
      if (proc._stdout_controller) try { proc._stdout_controller.close(); } catch(e) {}
      if (proc._stderr_controller) try { proc._stderr_controller.close(); } catch(e) {}
      if (proc.options.onExit) proc.options.onExit(proc, exitCode, signalCode);
      if (proc.options.terminal && proc.options.terminal.exit) {
        proc.options.terminal.exit(proc.terminal, 0, null);
      }
      proc._exited_resolve(exitCode);
      window.__meta_cleanup(handle);
    },

    _write: function(handle, data) {
      let b64;
      if (typeof data === 'string') {
        b64 = btoa(data);
      } else {
        b64 = uint8ToB64(new Uint8Array(data));
      }
      window.__meta_write(handle, b64);
    },
    _closeStdin: function(handle) {
      window.__meta_closeStdin(handle);
    },
    _kill: function(handle, signal) {
      window.__meta_kill(handle, signal);
    },
    _resize: function(handle, cols, rows) {
      window.__meta_resize(handle, cols, rows);
    }
  };

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
