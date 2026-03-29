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
    const chunk = 8192;
    for (let i = 0; i < len; i += chunk) {
      bin += String.fromCharCode.apply(null, uint8.subarray(i, i + chunk));
    }
    return btoa(bin);
  }

  const CRON_MONTHS = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"];
  const CRON_DAYS = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
  const CRON_NICKNAMES = {
    "@yearly": "0 0 1 1 *",
    "@annually": "0 0 1 1 *",
    "@monthly": "0 0 1 * *",
    "@weekly": "0 0 * * 0",
    "@daily": "0 0 * * *",
    "@midnight": "0 0 * * *",
    "@hourly": "0 * * * *"
  };

  function parseCronField(field, min, max, names) {
    if (field === '*') return null;
    const parts = field.split(',');
    const result = new Set();
    for (const part of parts) {
      let [range, step] = part.split('/');
      step = step ? parseInt(step, 10) : 1;
      let start, end;
      if (range === '*') {
        start = min; end = max;
      } else if (range.includes('-')) {
        let [s, e] = range.split('-');
        start = parseCronValue(s, names);
        end = parseCronValue(e, names);
      } else {
        start = end = parseCronValue(range, names);
      }
      for (let i = start; i <= end; i += step) result.add(i);
    }
    return result;
  }

  function parseCronValue(val, names) {
    if (names) {
      const idx = names.indexOf(val.toUpperCase().substring(0, 3));
      if (idx !== -1) return names === CRON_DAYS ? idx : idx + 1;
    }
    return parseInt(val, 10);
  }

  class CronParser {
    constructor(expr) {
      expr = CRON_NICKNAMES[expr] || expr;
      const fields = expr.split(/\s+/);
      if (fields.length !== 5) throw new Error("Invalid cron expression");
      this.min = parseCronField(fields[0], 0, 59);
      this.hour = parseCronField(fields[1], 0, 23);
      this.dom = parseCronField(fields[2], 1, 31);
      this.month = parseCronField(fields[3], 1, 12, CRON_MONTHS);
      this.dow = parseCronField(fields[4], 0, 7, CRON_DAYS);
      if (this.dow && this.dow.has(7)) this.dow.add(0);
    }

    next(from) {
      let curr = new Date(from || Date.now());
      curr.setUTCSeconds(0, 0);
      curr.setUTCMinutes(curr.getUTCMinutes() + 1);

      const startYear = curr.getUTCFullYear();
      while (curr.getUTCFullYear() < startYear + 5) {
        if (this.month && !this.month.has(curr.getUTCMonth() + 1)) {
          curr.setUTCMonth(curr.getUTCMonth() + 1, 1);
          curr.setUTCHours(0, 0, 0, 0);
          continue;
        }

        const domSet = this.dom !== null;
        const dowSet = this.dow !== null;
        let matchDay = false;
        if (domSet && dowSet) {
          matchDay = this.dom.has(curr.getUTCDate()) || this.dow.has(curr.getUTCDay());
        } else if (domSet) {
          matchDay = this.dom.has(curr.getUTCDate());
        } else if (dowSet) {
          matchDay = this.dow.has(curr.getUTCDay());
        } else {
          matchDay = true;
        }

        if (!matchDay) {
          curr.setUTCDate(curr.getUTCDate() + 1);
          curr.setUTCHours(0, 0, 0, 0);
          continue;
        }

        if (this.hour && !this.hour.has(curr.getUTCHours())) {
          curr.setUTCHours(curr.getUTCHours() + 1, 0, 0, 0);
          continue;
        }

        if (this.min && !this.min.has(curr.getUTCMinutes())) {
          curr.setUTCMinutes(curr.getUTCMinutes() + 1, 0, 0);
          continue;
        }

        return curr;
      }
      return null;
    }
  }

  class Subprocess {
    constructor(handle, options) {
      this.handle = handle;
      this.pid = null;
      this.options = options || {};
      this._exited_resolve = null;
      this.exited = new Promise(resolve => { this._exited_resolve = resolve; });
      this.exitCode = null;
      this.signalCode = null;
      this.killed = false;

      if (this.options.terminal) {
        this.terminal = new Terminal(this);
        this.stdin = null; this.stdout = null; this.stderr = null;
      } else {
        this.terminal = null;
        this._stdout_controller = null;
        this.stdout = new ReadableStream({ start: (c) => { this._stdout_controller = c; } });
        this._stderr_controller = null;
        this.stderr = new ReadableStream({ start: (c) => { this._stderr_controller = c; } });
        this.stdin = {
          write: (data) => { window.meta._write(this.handle, data); },
          end: () => { window.meta._closeStdin(this.handle); },
          flush: () => {}
        };
      }
    }
    kill(sig) { this.killed = true; window.meta._kill(this.handle, sig || 'SIGTERM'); }
    ref() {} unref() {}
    resourceUsage() { return { maxRSS: 0, cpuTime: { user: 0, system: 0 } }; }
  }

  class Terminal {
    constructor(options_or_proc) {
      if (options_or_proc instanceof Subprocess) {
        this.proc = options_or_proc;
        this.handle = this.proc.handle;
      } else {
        this.options = options_or_proc || {};
        this.handle = "term_" + (++window.meta._handleCounter);
      }
      this.closed = false;
    }
    write(data) { window.meta._write(this.handle, data); }
    resize(c, r) { window.meta._resize(this.handle, c, r); }
    setRawMode(e) {}
    close() { this.closed = true; }
    ref() {} unref() {}
  }

  const meta = async function(path, schedule, title) {
    await window.__meta_cron_register(path, schedule, title);
  };

  meta._processes = {};
  meta._handleCounter = 0;
  meta.Terminal = Terminal;
  meta.spawn = function(cmd, opts) {
    if (!Array.isArray(cmd) && typeof cmd === 'object' && cmd.cmd) { opts = cmd; cmd = cmd.cmd; }
    const handle = "proc_" + (++this._handleCounter);
    const proc = new Subprocess(handle, opts);
    this._processes[handle] = proc;
    (async () => {
      try {
        const res = await window.__meta_spawn(handle, JSON.stringify(cmd), JSON.stringify(opts || {}));
        if (res.error) return console.error("meta.spawn error:", res.error);
        proc.pid = res.pid;
      } catch (e) { console.error("meta.spawn failed:", e); }
    })();
    return proc;
  };

  meta.spawnSync = function(cmd, opts) {
    if (!Array.isArray(cmd) && typeof cmd === 'object' && cmd.cmd) { opts = cmd; cmd = cmd.cmd; }
    return (async () => {
      const res = await window.__meta_spawnSync(JSON.stringify(cmd), JSON.stringify(opts || {}));
      if (res.stdout) res.stdout = b64ToUint8(res.stdout);
      if (res.stderr) res.stderr = b64ToUint8(res.stderr);
      return res;
    })();
  };

  meta.cron = meta;
  meta.cron.parse = function(expr, relativeDate) {
    try {
      return new CronParser(expr).next(relativeDate);
    } catch (e) {
      return null;
    }
  };
  meta.cron.remove = async function(title) {
    await window.__meta_cron_remove(title);
  };

  meta._onData = function(handle, type, data_b64) {
    const proc = this._processes[handle];
    if (!proc) return;
    const encoded = b64ToUint8(data_b64);
    if (type === 'stdout' && proc._stdout_controller) proc._stdout_controller.enqueue(encoded);
    else if (type === 'stderr' && proc._stderr_controller) proc._stderr_controller.enqueue(encoded);
    else if (type === 'terminal' && proc.options.terminal && proc.options.terminal.data) proc.options.terminal.data(proc.terminal, encoded);
  };

  meta._onExit = function(handle, exitCode, signalCode) {
    const proc = this._processes[handle];
    if (!proc) return;
    proc.exitCode = exitCode; proc.signalCode = signalCode;
    if (proc._stdout_controller) try { proc._stdout_controller.close(); } catch(e) {}
    if (proc._stderr_controller) try { proc._stderr_controller.close(); } catch(e) {}
    if (proc.options.onExit) proc.options.onExit(proc, exitCode, signalCode);
    if (proc.options.terminal && proc.options.terminal.exit) proc.options.terminal.exit(proc.terminal, 0, null);
    proc._exited_resolve(exitCode);
    window.__meta_cleanup(handle);
  };

  meta._write = function(h, d) {
    if (h === null) return;
    window.__meta_write(h, typeof d === 'string' ? btoa(d) : uint8ToB64(new Uint8Array(d)));
  };
  meta._closeStdin = function(h) { if (h !== null) window.__meta_closeStdin(h); };
  meta._kill = function(h, s) { if (h !== null) window.__meta_kill(h, s); };
  meta._resize = function(h, c, r) { if (h !== null) window.__meta_resize(h, c, r); };

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

  window.meta = meta;
})();
)js";

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_META_JS_HH
