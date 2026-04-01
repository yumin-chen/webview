
// AlloyScript Runtime
(function() {
  'use strict';

  function b64ToUint8(b64: string): Uint8Array {
    const bin = atob(b64);
    const bytes = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
    return bytes;
  }

  function uint8ToB64(uint8: Uint8Array): string {
    let bin = '';
    const len = uint8.byteLength;
    const chunk = 8192;
    for (let i = 0; i < len; i += chunk) {
      bin += String.fromCharCode.apply(null, Array.from(uint8.subarray(i, i + chunk)));
    }
    return btoa(bin);
  }

  // --- Cron ---
  const CRON_MONTHS = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"];
  const CRON_DAYS = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
  const CRON_NICKNAMES: Record<string, string> = {
    "@yearly": "0 0 1 1 *", "@annually": "0 0 1 1 *", "@monthly": "0 0 1 * *",
    "@weekly": "0 0 * * 0", "@daily": "0 0 * * *", "@midnight": "0 0 * * *", "@hourly": "0 * * * *"
  };

  class CronParser {
    min: Set<number> | null; hour: Set<number> | null; dom: Set<number> | null; month: Set<number> | null; dow: Set<number> | null;
    constructor(expr: string) {
      expr = CRON_NICKNAMES[expr] || expr;
      const fields = expr.split(/\s+/);
      if (fields.length !== 5) throw new Error("Invalid cron expression");
      this.min = this.parseField(fields[0], 0, 59);
      this.hour = this.parseField(fields[1], 0, 23);
      this.dom = this.parseField(fields[2], 1, 31);
      this.month = this.parseField(fields[3], 1, 12, CRON_MONTHS);
      this.dow = this.parseField(fields[4], 0, 7, CRON_DAYS);
      if (this.dow && this.dow.has(7)) this.dow.add(0);
    }
    private parseField(field: string, min: number, max: number, names?: string[]): Set<number> | null {
      if (field === '*') return null;
      const result = new Set<number>();
      for (const part of field.split(',')) {
        let [range, stepStr] = part.split('/');
        const step = stepStr ? parseInt(stepStr, 10) : 1;
        let start: number, end: number;
        if (range === '*') { start = min; end = max; }
        else if (range.includes('-')) {
          let [s, e] = range.split('-');
          start = this.parseValue(s, names); end = this.parseValue(e, names);
        } else { start = end = this.parseValue(range, names); }
        for (let i = start; i <= end; i += step) result.add(i);
      }
      return result;
    }
    private parseValue(val: string, names?: string[]): number {
      if (names) {
        const idx = names.indexOf(val.toUpperCase().substring(0, 3));
        if (idx !== -1) return names === CRON_DAYS ? idx : idx + 1;
      }
      return parseInt(val, 10);
    }
    next(from?: Date | number): Date | null {
      let curr = new Date(from || Date.now());
      curr.setUTCSeconds(0, 0); curr.setUTCMinutes(curr.getUTCMinutes() + 1);
      const startYear = curr.getUTCFullYear();
      while (curr.getUTCFullYear() < startYear + 5) {
        if (this.month && !this.month.has(curr.getUTCMonth() + 1)) {
          curr.setUTCMonth(curr.getUTCMonth() + 1, 1); curr.setUTCHours(0, 0, 0, 0); continue;
        }
        const domSet = this.dom !== null, dowSet = this.dow !== null;
        let matchDay = false;
        if (domSet && dowSet) matchDay = this.dom!.has(curr.getUTCDate()) || this.dow!.has(curr.getUTCDay());
        else if (domSet) matchDay = this.dom!.has(curr.getUTCDate());
        else if (dowSet) matchDay = this.dow!.has(curr.getUTCDay());
        else matchDay = true;
        if (!matchDay) { curr.setUTCDate(curr.getUTCDate() + 1); curr.setUTCHours(0, 0, 0, 0); continue; }
        if (this.hour && !this.hour.has(curr.getUTCHours())) { curr.setUTCHours(curr.getUTCHours() + 1, 0, 0, 0); continue; }
        if (this.min && !this.min.has(curr.getUTCMinutes())) { curr.setUTCMinutes(curr.getUTCMinutes() + 1, 0, 0); continue; }
        return curr;
      }
      return null;
    }
  }

  // --- Subprocess ---
  class Subprocess {
    handle: string; pid: number | null = null; options: any; exited: Promise<number>;
    private _exited_resolve!: (code: number) => void;
    exitCode: number | null = null; signalCode: string | null = null; killed = false;
    terminal: Terminal | null = null; stdout: any = null; stderr: any = null; stdin: any = null;
    private _stdout_controller: ReadableStreamDefaultController | null = null;
    private _stderr_controller: ReadableStreamDefaultController | null = null;

    constructor(handle: string, options: any) {
      this.handle = handle; this.options = options || {};
      this.exited = new Promise(resolve => { this._exited_resolve = resolve; });
      if (this.options.terminal) {
        this.terminal = new Terminal(this);
      } else {
        this.stdout = new ReadableStream({ start: (c) => { this._stdout_controller = c; } });
        this.stderr = new ReadableStream({ start: (c) => { this._stderr_controller = c; } });
        this.stdin = {
          write: (data: any) => { (window as any).Alloy._write(this.handle, data); },
          end: () => { (window as any).Alloy._closeStdin(this.handle); },
          flush: () => {}
        };
      }
    }
    kill(sig?: string | number) { this.killed = true; (window as any).Alloy._kill(this.handle, sig || 'SIGTERM'); }
    ref() {} unref() {}
    resourceUsage() { return { maxRSS: 0, cpuTime: { user: 0, system: 0, total: 0 }, contextSwitches: { voluntary: 0, involuntary: 0 }, ops: { in: 0, out: 0 } }; }
    disconnect() {}
    send(msg: any) {}

    _onData(type: string, encodedData: string) {
        const data = b64ToUint8(encodedData);
        if (type === 'stdout' && this._stdout_controller) this._stdout_controller.enqueue(data);
        else if (type === 'stderr' && this._stderr_controller) this._stderr_controller.enqueue(data);
        else if (type === 'terminal' && this.options.terminal?.data) this.options.terminal.data(this.terminal, data);
    }
    _onExit(exitCode: number, signalCode: number) {
        this.exitCode = exitCode; this.signalCode = signalCode ? "SIG" + signalCode : null;
        if (this._stdout_controller) try { this._stdout_controller.close(); } catch(e) {}
        if (this._stderr_controller) try { this._stderr_controller.close(); } catch(e) {}
        if (this.options.onExit) this.options.onExit(this, exitCode, this.signalCode);
        if (this.options.terminal?.exit) this.options.terminal.exit(this.terminal, 0, null);
        this._exited_resolve(exitCode);
    }
  }

  class Terminal {
    proc?: Subprocess; handle: string; options?: any; closed = false;
    constructor(options_or_proc: any) {
      if (options_or_proc instanceof Subprocess) { this.proc = options_or_proc; this.handle = this.proc.handle; }
      else { this.options = options_or_proc || {}; this.handle = "term_" + (++(window as any).Alloy._handleCounter); }
    }
    write(data: any) { (window as any).Alloy._write(this.handle, data); }
    resize(c: number, r: number) { (window as any).Alloy._resize(this.handle, c, r); }
    setRawMode(e: boolean) {} close() { this.closed = true; } ref() {} unref() {}
  }

  // --- GUI ---
  class NativeComponent {
    handle: string; type: string; props: any; children: NativeComponent[] = [];
    constructor(type: string, props: any) {
      this.type = type; this.props = props || {};
      this.handle = "gui_" + (++(window as any).Alloy._handleCounter);
      (window as any).Alloy._widgets[this.handle] = this;
      (window as any).__alloy_gui_create(this.handle, this.type, JSON.stringify(this.props));
    }
    append(child: NativeComponent) {
      this.children.push(child);
      (window as any).__alloy_gui_append(this.handle, child.handle);
    }
    setText(text: string) { (window as any).__alloy_gui_set_text(this.handle, text); }
    setValue(value: any) { (window as any).__alloy_gui_set_value(this.handle, String(value)); }
    addEventListener(event: string, handler: Function) {
      if (!this.props.handlers) this.props.handlers = {};
      this.props.handlers[event] = handler;
    }
    _trigger(event: string) {
      if (this.props.handlers && this.props.handlers[event]) this.props.handlers[event]();
    }
  }

  const Alloy: any = async function(path: string, schedule: string, title: string) {
    await (window as any).__alloy_cron_register(path, schedule, title);
  };

  Alloy._processes = {} as Record<string, Subprocess>;
  Alloy._widgets = {} as Record<string, NativeComponent>;
  Alloy._handleCounter = 0;
  Alloy.Terminal = Terminal;
  Alloy.file = (path: string) => ({ type: "file", path: path });

  Alloy.spawn = function(cmd: any, opts: any) {
    let command = Array.isArray(cmd) ? cmd : (cmd.cmd || []);
    let options = Array.isArray(cmd) ? (opts || {}) : (cmd || {});
    const handle = "proc_" + (++this._handleCounter);
    const proc = new Subprocess(handle, options);
    this._processes[handle] = proc;
    (async () => {
      try {
        const res_json = await (globalThis as any).__alloy_spawn(handle, JSON.stringify(command), JSON.stringify(options || {}));
        const res = JSON.parse(res_json);
        if (res.error) return console.error("Alloy.spawn error:", res.error);
        proc.pid = res.pid;
      } catch (e) { console.error("Alloy.spawn failed:", e); }
    })();
    return proc;
  };

  Alloy.spawnSync = async function(cmd: any, opts: any) {
    let command = Array.isArray(cmd) ? cmd : (cmd.cmd || []);
    let options = Array.isArray(cmd) ? (opts || {}) : (cmd || {});
    const res_raw = await (globalThis as any).__alloy_spawnSync(JSON.stringify(command), JSON.stringify(options || {}));
    const res = typeof res_raw === 'string' ? JSON.parse(res_raw) : res_raw;
    if (res.stdout) res.stdout = b64ToUint8(res.stdout);
    if (res.stderr) res.stderr = b64ToUint8(res.stderr);
    return res;
  };

  Alloy.cron = Alloy;
  Alloy.cron.parse = function(expr: string, relativeDate?: Date | number) {
    try { return new CronParser(expr).next(relativeDate); } catch (e) { return null; }
  };
  Alloy.cron.remove = async function(title: string) { await (globalThis as any).__alloy_cron_remove(title); };

  Alloy.gui = {
    Window: (props: any) => new NativeComponent("Window", props),
    Button: (props: any) => new NativeComponent("Button", props),
    Label: (props: any) => new NativeComponent("Label", props),
    VStack: (props: any) => new NativeComponent("VStack", props),
    HStack: (props: any) => new NativeComponent("HStack", props),
    TextField: (props: any) => new NativeComponent("TextField", props),
    TextArea: (props: any) => new NativeComponent("TextArea", props),
    CheckBox: (props: any) => new NativeComponent("CheckBox", props),
    Slider: (props: any) => new NativeComponent("Slider", props),
    ProgressBar: (props: any) => new NativeComponent("ProgressBar", props),
    Switch: (props: any) => new NativeComponent("Switch", props),
    _onEvent: function(handle: string, event: string) {
      const comp = Alloy._widgets[handle];
      if (comp) comp._trigger(event);
    }
  };

  Alloy._onData = function(handle: string, type: string, data_b64: string) {
    const proc = this._processes[handle];
    if (proc) proc._onData(type, data_b64);
  };
  Alloy._onExit = function(handle: string, exitCode: number, signalCode: number) {
    const proc = this._processes[handle];
    if (proc) { proc._onExit(exitCode, signalCode); delete this._processes[handle]; (window as any).__alloy_cleanup(handle); }
  };
  Alloy._write = function(h: string, d: any) {
    if (h === null) return;
    let b64;
    if (typeof d === 'string') b64 = btoa(d); else b64 = uint8ToB64(new Uint8Array(d));
    (window as any).__alloy_write(h, b64);
  };
  Alloy._closeStdin = function(h: string) { if (h !== null) (window as any).__alloy_closeStdin(h); };
  Alloy._kill = function(h: string, s: string) { if (h !== null) (window as any).__alloy_kill(h, s); };
  Alloy._resize = function(h: string, c: number, r: number) { if (h !== null) (window as any).__alloy_resize(h, c, r); };

  if (!ReadableStream.prototype.hasOwnProperty('text')) {
    (ReadableStream.prototype as any).text = async function() {
      const reader = this.getReader(); let decoder = new TextDecoder(); let result = '';
      while (true) {
        const { done, value } = await reader.read(); if (done) break;
        result += decoder.decode(value, { stream: true });
      }
      result += decoder.decode(); return result;
    };
  }

  (globalThis as any).Alloy = Alloy;
})();
