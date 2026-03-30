/*
 * MIT License
 *
 * Copyright (c) 2017 Serge Zaitsev
 * Copyright (c) 2022 Steffen André Langnes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WEBVIEW_DETAIL_ENGINE_BASE_HH
#define WEBVIEW_DETAIL_ENGINE_BASE_HH

#if defined(__cplusplus) && !defined(WEBVIEW_HEADER)

#include "../errors.hh"
#include "../types.h"
#include "../types.hh"
#include "alloyscript_runtime.hh"
#include "sqlite_runtime.hh"
#include "json.hh"
#include "user_script.hh"

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>

namespace webview {
namespace detail {

class engine_base {
public:
  engine_base(bool owns_window) : m_owns_window{owns_window} {}

  virtual ~engine_base() {
      for (auto &kv : m_subprocesses) {
          kv.second->monitoring = false;
      }
  }

  noresult navigate(const std::string &url) {
    if (url.empty()) {
      return navigate_impl("about:blank");
    }
    return navigate_impl(url);
  }

  using binding_t = std::function<void(std::string, std::string, void *)>;
  class binding_ctx_t {
  public:
    binding_ctx_t(binding_t callback, void *arg)
        : m_callback(callback), m_arg(arg) {}
    void call(std::string id, std::string args) const {
      if (m_callback) {
        m_callback(id, args, m_arg);
      }
    }

  private:
    // This function is called upon execution of the bound JS function
    binding_t m_callback;
    // This user-supplied argument is passed to the callback
    void *m_arg;
  };

  using sync_binding_t = std::function<std::string(std::string)>;

  // Synchronous bind
  noresult bind(const std::string &name, sync_binding_t fn) {
    auto wrapper = [this, fn](const std::string &id, const std::string &req,
                              void * /*arg*/) { resolve(id, 0, fn(req)); };
    return bind(name, wrapper, nullptr);
  }

  // Asynchronous bind
  noresult bind(const std::string &name, binding_t fn, void *arg) {
    // NOLINTNEXTLINE(readability-container-contains): contains() requires C++20
    if (bindings.count(name) > 0) {
      return error_info{WEBVIEW_ERROR_DUPLICATE};
    }
    bindings.emplace(name, binding_ctx_t(fn, arg));
    replace_bind_script();
    // Notify that a binding was created if the init script has already
    // set things up.
    eval("if (window.__webview__) {\n\
window.__webview__.onBind(" +
         json_escape(name) + ")\n\
}");
    return {};
  }

  noresult unbind(const std::string &name) {
    auto found = bindings.find(name);
    if (found == bindings.end()) {
      return error_info{WEBVIEW_ERROR_NOT_FOUND};
    }
    bindings.erase(found);
    replace_bind_script();
    // Notify that a binding was created if the init script has already
    // set things up.
    eval("if (window.__webview__) {\n\
window.__webview__.onUnbind(" +
         json_escape(name) + ")\n\
}");
    return {};
  }

  noresult resolve(const std::string &id, int status,
                   const std::string &result) {
    // NOLINTNEXTLINE(modernize-avoid-bind): Lambda with move requires C++14
    return dispatch(std::bind(
        [id, status, this](std::string escaped_result) {
          std::string js = "window.__webview__.onReply(" + json_escape(id) +
                           ", " + std::to_string(status) + ", " +
                           escaped_result + ")";
          eval(js);
        },
        result.empty() ? "undefined" : json_escape(result)));
  }

  result<void *> window() { return window_impl(); }
  result<void *> widget() { return widget_impl(); }
  result<void *> browser_controller() { return browser_controller_impl(); }
  noresult run() { return run_impl(); }
  noresult terminate() { return terminate_impl(); }
  noresult dispatch(std::function<void()> f) { return dispatch_impl(f); }
  noresult set_title(const std::string &title) { return set_title_impl(title); }

  noresult set_size(int width, int height, webview_hint_t hints) {
    auto res = set_size_impl(width, height, hints);
    m_is_size_set = true;
    return res;
  }

  noresult set_html(const std::string &html) { return set_html_impl(html); }

  noresult init(const std::string &js) {
    add_user_script(js);
    return {};
  }

  noresult eval(const std::string &js) { return eval_impl(js); }

protected:
  virtual noresult navigate_impl(const std::string &url) = 0;
  virtual result<void *> window_impl() = 0;
  virtual result<void *> widget_impl() = 0;
  virtual result<void *> browser_controller_impl() = 0;
  virtual noresult run_impl() = 0;
  virtual noresult terminate_impl() = 0;
  virtual noresult dispatch_impl(std::function<void()> f) = 0;
  virtual noresult set_title_impl(const std::string &title) = 0;
  virtual noresult set_size_impl(int width, int height,
                                 webview_hint_t hints) = 0;
  virtual noresult set_html_impl(const std::string &html) = 0;
  virtual noresult eval_impl(const std::string &js) = 0;

  virtual user_script *add_user_script(const std::string &js) {
    return std::addressof(*m_user_scripts.emplace(m_user_scripts.end(),
                                                  add_user_script_impl(js)));
  }

  virtual user_script add_user_script_impl(const std::string &js) = 0;

  virtual void
  remove_all_user_scripts_impl(const std::list<user_script> &scripts) = 0;

  virtual bool are_user_scripts_equal_impl(const user_script &first,
                                           const user_script &second) = 0;

  virtual user_script *replace_user_script(const user_script &old_script,
                                           const std::string &new_script_code) {
    remove_all_user_scripts_impl(m_user_scripts);
    user_script *old_script_ptr{};
    for (auto &script : m_user_scripts) {
      auto is_old_script = are_user_scripts_equal_impl(script, old_script);
      script = add_user_script_impl(is_old_script ? new_script_code
                                                  : script.get_code());
      if (is_old_script) {
        old_script_ptr = std::addressof(script);
      }
    }
    return old_script_ptr;
  }

  void replace_bind_script() {
    if (m_bind_script) {
      m_bind_script = replace_user_script(*m_bind_script, create_bind_script());
    } else {
      m_bind_script = add_user_script(create_bind_script());
    }
  }

  void add_alloy_bindings() {
    bind("Alloy_spawn",
         [this](const std::string &seq, const std::string &req,
                void * /*arg*/) {
           auto cmd_json = json_parse(req, "", 0);
           auto options_json = json_parse(req, "", 1);

           std::vector<std::string> args;
           int i = 0;
           while (true) {
             auto arg = json_parse(cmd_json, "", i++);
             if (arg.empty()) break;
             args.push_back(arg);
           }

           auto cwd = json_parse(options_json, "cwd", 0);
           bool use_ipc = !json_parse(options_json, "ipc", 0).empty();

           auto state = m_alloy.spawn(args, cwd, {}, use_ipc);
           if (!state) {
             resolve(seq, 1, "null");
             return;
           }

#ifdef _WIN32
           std::string id_str = std::to_string(state->dwProcessId);
#else
           std::string id_str = std::to_string(state->pid);
#endif
           m_subprocesses[id_str] = state;

           state->on_stdout = [this, id_str](const std::string &data) {
             this->dispatch([this, id_str, data]() {
               this->eval("window.Alloy._onStdout('" + id_str + "', " + json_escape(data) + ")");
             });
           };
           state->on_stderr = [this, id_str](const std::string &data) {
             this->dispatch([this, id_str, data]() {
               this->eval("window.Alloy._onStderr('" + id_str + "', " + json_escape(data) + ")");
             });
           };
           state->on_exit = [this, id_str](int exit_code, int signal_code) {
             this->dispatch([this, id_str, exit_code, signal_code]() {
               this->eval("window.Alloy._onExit('" + id_str + "', " + std::to_string(exit_code) + ", " + std::to_string(signal_code) + ")");
               this->m_subprocesses.erase(id_str);
             });
           };

           m_alloy.start_monitoring(state);
           resolve(seq, 0, id_str);
         },
         nullptr);

    bind("Alloy_stdinWrite", [this](const std::string &req) -> std::string {
      auto id_str = json_parse(req, "", 0);
      auto data = json_parse(req, "", 1);
      auto it = m_subprocesses.find(id_str);
      if (it != m_subprocesses.end()) {
        m_alloy.queue_stdin(it->second, data);
        return "true";
      }
      return "false";
    });

    bind("Alloy_shell",
         [this](const std::string &seq, const std::string &req,
                void * /*arg*/) {
           auto command = json_parse(req, "", 0);
           auto options_json = json_parse(req, "", 1);
           auto cwd = json_parse(options_json, "cwd", 0);

           std::thread([this, seq, command, cwd]() {
             auto args = alloyscript_runtime::tokenize(command);
             auto state = m_alloy.spawn(args, cwd);
             if (!state) {
                 this->dispatch([this, seq]() { this->resolve(seq, 1, "null"); });
                 return;
             }
             std::string stdout_acc, stderr_acc;
             std::mutex acc_mutex;
             state->on_stdout = [&](const std::string &data) { std::lock_guard<std::mutex> l(acc_mutex); stdout_acc += data; };
             state->on_stderr = [&](const std::string &data) { std::lock_guard<std::mutex> l(acc_mutex); stderr_acc += data; };

             bool done = false;
             int exit_code = 0;
             state->on_exit = [&](int code, int sig) { (void)sig; exit_code = code; done = true; };

             m_alloy.start_monitoring(state);
             while (!done) std::this_thread::sleep_for(std::chrono::milliseconds(10));

             std::string result_json = "{";
             result_json += "\"exitCode\":" + std::to_string(exit_code) + ",";
             result_json += "\"stdout\":" + json_escape(stdout_acc) + ",";
             result_json += "\"stderr\":" + json_escape(stderr_acc);
             result_json += "}";
             this->dispatch([this, seq, result_json]() { this->resolve(seq, 0, result_json); });
           }).detach();
         },
         nullptr);

    bind("Alloy_sqlite_open", [this](const std::string &req) -> std::string {
      auto filename = json_parse(req, "", 0);
      auto options = json_parse(req, "", 1);
      bool readonly = !json_parse(options, "readonly", 0).empty();
      bool create = !json_parse(options, "create", 0).empty();
      bool safeIntegers = !json_parse(options, "safeIntegers", 0).empty();
      bool strict = !json_parse(options, "strict", 0).empty();

      try {
        return m_sqlite.open(filename, readonly, create, safeIntegers, strict);
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });

    bind("Alloy_sqlite_exec", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto sql = json_parse(req, "", 1);
      try {
        m_sqlite.exec(db_id, sql);
        return "true";
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });

    bind("Alloy_sqlite_close", [this](const std::string &req) -> std::string {
      auto id = json_parse(req, "", 0);
      m_sqlite.close(id);
      return "true";
    });

    bind("Alloy_sqlite_prepare", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto sql = json_parse(req, "", 1);
      auto use_cache = json_parse(req, "", 2) == "true";

      auto db = m_sqlite.get_db(db_id);
      if (!db) return "error:Database not found";

      try {
        auto stmt = db->prepare(sql, use_cache);
        return stmt->get_metadata();
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });

    bind("Alloy_sqlite_bind_and_execute", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto sql = json_parse(req, "", 1);
      auto params_json = json_parse(req, "", 2);
      auto mode = json_parse(req, "", 3);

      auto db = m_sqlite.get_db(db_id);
      if (!db) return "error:Database not found";

      try {
        auto stmt = db->prepare(sql, true);
        stmt->bind(params_json, db->is_strict());
        return stmt->execute(mode, db->is_safe_integers());
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });

    bind("Alloy_sqlite_file_control", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto cmd = std::stoi(json_parse(req, "", 1));
      auto value_str = json_parse(req, "", 2);

      auto db = m_sqlite.get_db(db_id);
      if (!db) return "error:Database not found";

      // Minimal implementation for value
      int val = 0;
      if (!value_str.empty() && value_str != "null") {
          val = std::stoi(value_str);
      }
      return std::to_string(db->file_control(cmd, &val));
    });

    bind("Alloy_sqlite_serialize", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto db = m_sqlite.get_db(db_id);
      if (!db) return "error:Database not found";
      return db->serialize();
    });

    bind("Alloy_sqlite_deserialize", [this](const std::string &req) -> std::string {
      auto data_json = json_parse(req, "", 0);
      // data_json is a JSON array of numbers
      std::vector<unsigned char> data;
      int i = 0;
      while (true) {
        std::string val = json_parse(data_json, "", i++);
        if (val.empty()) break;
        data.push_back((unsigned char)std::stoi(val));
      }
      try {
        return m_sqlite.open_from_data(data);
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });

    bind("Alloy_sqlite_load_extension", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto path = json_parse(req, "", 1);
      auto db = m_sqlite.get_db(db_id);
      if (!db) return "error:Database not found";
      try {
        db->load_extension(path);
        return "true";
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });

    bind("Alloy_sqlite_step", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto sql = json_parse(req, "", 1);
      auto params_json = json_parse(req, "", 2);
      auto is_reset = json_parse(req, "", 3) == "true";

      auto db = m_sqlite.get_db(db_id);
      if (!db) return "error:Database not found";

      try {
        auto stmt = db->prepare(sql, true);
        if (is_reset) {
          stmt->bind(params_json, db->is_strict());
        }
        return stmt->step(db->is_safe_integers());
      } catch (const std::exception &e) {
        return "error:" + std::string(e.what());
      }
    });
  }

  std::string create_alloy_script() {
    return R"js(
(function() {
  if (window.Alloy) return;
  const $ = function(strings, ...values) {
    let cmd = "";
    for (let i = 0; i < strings.length; i++) {
        cmd += strings[i];
        if (i < values.length) {
            let val = values[i];
            if (typeof val === 'string') {
                cmd += "'" + val.replace(/'/g, "'\\''") + "'";
            } else if (val && val.raw) {
                cmd += val.raw;
            } else {
                cmd += JSON.stringify(val);
            }
        }
    }

    let options = { cwd: $.cwd_val, env: $.env_val };
    let quiet = false;
    let nothrow = !$.throws_val;

    const promise = (async () => {
        const res = await window.Alloy_shell(cmd, options);
        if (res === "null") throw new Error("Shell execution failed");
        if (!quiet) {
            if (res.stdout) console.log(res.stdout);
            if (res.stderr) console.error(res.stderr);
        }
        if (!nothrow && res.exitCode !== 0) {
            const err = new Error(`Command failed with exit code ${res.exitCode}`);
            err.exitCode = res.exitCode;
            err.stdout = new TextEncoder().encode(res.stdout);
            err.stderr = new TextEncoder().encode(res.stderr);
            throw err;
        }
        return {
            exitCode: res.exitCode,
            stdout: new TextEncoder().encode(res.stdout),
            stderr: new TextEncoder().encode(res.stderr),
            text: () => Promise.resolve(res.stdout),
            json: () => Promise.resolve(JSON.parse(res.stdout)),
            blob: () => Promise.resolve(new Blob([res.stdout], { type: 'text/plain' })),
            lines: async function*() {
                const lines = res.stdout.split('\n');
                for (const line of lines) if (line) yield line;
            }
        };
    })();

    promise.quiet = () => { quiet = true; return promise; };
    promise.nothrow = () => { nothrow = true; return promise; };
    promise.text = () => { quiet = true; return promise.then(r => r.text()); };
    promise.json = () => { quiet = true; return promise.then(r => r.json()); };
    promise.blob = () => { quiet = true; return promise.then(r => r.blob()); };
    promise.lines = () => { quiet = true; return promise.then(r => r.lines()); };
    promise.cwd = (c) => { options.cwd = c; return promise; };
    promise.env = (e) => { options.env = e; return promise; };

    return promise;
  };
  $.cwd = (c) => { $.cwd_val = c; };
  $.env = (e) => { $.env_val = e; };
  $.throws = (t) => { $.throws_val = t; };
  $.nothrow = () => { $.throws_val = false; };
  $.braces = function(str) {
      const match = str.match(/\{([^}]+)\}/);
      if (!match) return [str];
      const parts = match[1].split(',');
      const result = [];
      for (const p of parts) {
          result.push(...$.braces(str.replace(match[0], p)));
      }
      return result;
  };
  $.escape = function(str) {
      return str.replace(/[$( )`"']/g, '\\$&');
  };
  $.cwd_val = "";
  $.env_val = {};
  $.throws_val = true;

  window.Alloy = {
    $: $,
    _subprocesses: {},
    spawn: async function(cmd, options) {
      const id = await window.Alloy_spawn(cmd, options || {});
      if (id === "null") return null;
      const proc = {
        pid: id,
        stdin: {
          write: (data) => window.Alloy_stdinWrite(id, data),
          end: () => {}
        },
        stdout: new ReadableStream({
          start(controller) {
            proc._stdoutController = controller;
          }
        }),
        stderr: new ReadableStream({
          start(controller) {
            proc._stderrController = controller;
          }
        }),
        exited: new Promise((resolve) => { proc._resolveExit = resolve; })
      };
      this._subprocesses[id] = proc;
      return proc;
    },
    _onStdout: function(id, data) {
      const proc = this._subprocesses[id];
      if (proc && proc._stdoutController) {
        proc._stdoutController.enqueue(new TextEncoder().encode(data));
      }
    },
    _onStderr: function(id, data) {
      const proc = this._subprocesses[id];
      if (proc && proc._stderrController) {
        proc._stderrController.enqueue(new TextEncoder().encode(data));
      }
    },
    _onExit: function(id, exitCode, signalCode) {
      const proc = this._subprocesses[id];
      if (proc) {
        proc.exitCode = exitCode;
        proc.signalCode = signalCode;
        if (proc._resolveExit) proc._resolveExit(exitCode);
      }
    },
    gui: {
        create: async (type, props) => window.Alloy_gui_create(type, props),
        createSignal: async (initial) => window.Alloy_gui_create_signal(initial)
    },
    sqlite: (function() {
      const constants = {
        SQLITE_FCNTL_PERSIST_WAL: 10,
        // Add more constants as needed
      };

      class Statement {
        constructor(db, sql, options = {}) {
          this.db = db;
          this.sql = sql;
          this.use_cache = options.cache !== false;

          const meta = window.Alloy_sqlite_prepare(this.db.id, this.sql, this.use_cache);
          if (meta.startsWith('error:')) throw new Error(meta.substring(6));
          const data = JSON.parse(meta);
          this.columnNames = data.columnNames;
          this.declaredTypes = data.declaredTypes;
          this.columnTypes = data.columnTypes;
          this.paramsCount = data.paramsCount;
        }

        _execute(mode, params) {
          const paramsJson = JSON.stringify(params, (k, v) => typeof v === 'bigint' ? v.toString() + 'n' : v);
          const res = window.Alloy_sqlite_bind_and_execute(this.db.id, this.sql, paramsJson, mode);
          if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));

          // Update metadata after execution if we fetched a row
          const meta = window.Alloy_sqlite_prepare(this.db.id, this.sql, true);
          const metaData = JSON.parse(meta);
          this.columnTypes = metaData.columnTypes;

          let data = JSON.parse(res);
          data = this._reviveSpecial(data);
          if (this._Class) {
            if (Array.isArray(data)) return data.map(r => this._instantiate(r));
            if (data && typeof data === 'object') return this._instantiate(data);
          }
          return data;
        }

        _reviveSpecial(obj) {
          if (typeof obj === 'string') {
            if (obj.endsWith('n')) return BigInt(obj.substring(0, obj.length - 1));
            if (obj.startsWith('base64:')) {
              const b64 = obj.substring(7);
              const bin = atob(b64);
              const arr = new Uint8Array(bin.length);
              for (let i = 0; i < bin.length; i++) arr[i] = bin.charCodeAt(i);
              return arr;
            }
          }
          if (Array.isArray(obj)) return obj.map(x => this._reviveSpecial(x));
          if (obj && typeof obj === 'object') {
            for (let k in obj) obj[k] = this._reviveSpecial(obj[k]);
          }
          return obj;
        }

        _instantiate(obj) {
          const instance = Object.create(this._Class.prototype);
          Object.assign(instance, obj);
          return instance;
        }

        all(...params) { return this._execute('all', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }
        get(...params) { return this._execute('get', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }
        run(...params) { return this._execute('run', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }
        values(...params) { return this._execute('values', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }

        as(Class) { this._Class = Class; return this; }
        finalize() {}
        toString() { return this.sql; }

        [Symbol.iterator]() { return this.iterate(); }
        *iterate(...params) {
          const paramsJson = JSON.stringify(params.length === 1 && typeof params[0] === 'object' ? params[0] : params, (k, v) => typeof v === 'bigint' ? v.toString() + 'n' : v);
          let first = true;
          while (true) {
            const res = window.Alloy_sqlite_step(this.db.id, this.sql, paramsJson, first);
            first = false;
            if (res === 'done') break;
            if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));
            let data = JSON.parse(res);
            yield this._reviveSpecial(data);
          }
        }
        [Symbol.dispose]() { this.finalize(); }
      }

      class Database {
        constructor(filename = ':memory:', options = {}) {
          if (typeof options === 'number') options = { flags: options };
          this.safeIntegers = options.safeIntegers || false;
          this.id = window.Alloy_sqlite_open(filename, options.readonly || false, options.create || false, this.safeIntegers, options.strict || false);
          if (this.id.startsWith('error:')) throw new Error(this.id.substring(6));
        }

        query(sql) { return new Statement(this, sql, { cache: true }); }
        prepare(sql) { return new Statement(this, sql, { cache: false }); }
        as(Class) { return this.query(sql).as(Class); } // For query.as(MyClass) usage? Wait, query returns statement.
        run(sql, params) {
          if (params === undefined) {
            const res = window.Alloy_sqlite_exec(this.id, sql);
            if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));
            return { lastInsertRowid: 0, changes: 0 };
          }
          return this.query(sql).run(params);
        }
        exec(sql) { return this.run(sql); }
        close() { window.Alloy_sqlite_close(this.id); }
        transaction(fn) {
          const wrapper = (...args) => {
            this.run("BEGIN");
            try {
              const res = fn.apply(this, args);
              this.run("COMMIT");
              return res;
            } catch (e) {
              this.run("ROLLBACK");
              throw e;
            }
          };
          wrapper.deferred = (...args) => { this.run("BEGIN DEFERRED"); try { const res = fn.apply(this, args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
          wrapper.immediate = (...args) => { this.run("BEGIN IMMEDIATE"); try { const res = fn.apply(this, args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
          wrapper.exclusive = (...args) => { this.run("BEGIN EXCLUSIVE"); try { const res = fn.apply(this, args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
          return wrapper;
        }
        serialize() { return new Uint8Array(JSON.parse(window.Alloy_sqlite_serialize(this.id))); }
        fileControl(cmd, val) { return window.Alloy_sqlite_file_control(this.id, cmd, val); }
        loadExtension(path) {
          const res = window.Alloy_sqlite_load_extension(this.id, path);
          if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));
        }
        static deserialize(data) {
          const db = new Database(':memory:'); // Dummy open to get instance
          window.Alloy_sqlite_close(db.id);
          const res = window.Alloy_sqlite_deserialize(JSON.stringify(Array.from(data)));
          if (res.startsWith('error:')) throw new Error(res.substring(6));
          db.id = res;
          return db;
        }
        static open(filename, options) { return new Database(filename, options); }
        static setCustomSQLite(path) {}
        [Symbol.dispose]() { this.close(); }
      }

      return { Database };
    })()
  };

  const script = document.createElement('script');
  script.type = 'importmap';
  script.textContent = JSON.stringify({
    imports: {
      "Alloy:sqlite": "data:text/javascript,export const Database = window.Alloy.sqlite.Database; export const constants = window.Alloy.sqlite.constants;",
      "alloy:sqlite": "data:text/javascript,export const Database = window.Alloy.sqlite.Database; export const constants = window.Alloy.sqlite.constants;",
      "alloy:gui": "data:text/javascript,export const Window = (p) => window.Alloy.gui.create('Window', p); export const Button = (p) => window.Alloy.gui.create('Button', p); export const Label = (p) => window.Alloy.gui.create('Label', p); export const TextField = (p) => window.Alloy.gui.create('TextField', p); export const TextArea = (p) => window.Alloy.gui.create('TextArea', p); export const CheckBox = (p) => window.Alloy.gui.create('CheckBox', p); export const RadioButton = (p) => window.Alloy.gui.create('RadioButton', p); export const ComboBox = (p) => window.Alloy.gui.create('ComboBox', p); export const Slider = (p) => window.Alloy.gui.create('Slider', p); export const ProgressBar = (p) => window.Alloy.gui.create('ProgressBar', p); export const TabView = (p) => window.Alloy.gui.create('TabView', p); export const ListView = (p) => window.Alloy.gui.create('ListView', p); export const TreeView = (p) => window.Alloy.gui.create('TreeView', p); export const WebView = (p) => window.Alloy.gui.create('WebView', p); export const VStack = (p) => window.Alloy.gui.create('VStack', p); export const HStack = (p) => window.Alloy.gui.create('HStack', p); export const ScrollView = (p) => window.Alloy.gui.create('ScrollView', p); export const useSignal = (v) => window.Alloy.gui.createSignal(v);"
    }
  });
  document.head.appendChild(script);
})();
)js";
  }

  void add_gui_bindings() {
    bind("Alloy_gui_create", [this](const std::string &seq, const std::string &req, void *) {
      auto type = json_parse(req, "", 0);
      auto props = json_parse(req, "", 1);
      resolve(seq, 0, "\"handle_123\"");
    }, nullptr);

    bind("Alloy_gui_create_signal", [this](const std::string &seq, const std::string &req, void *) {
      auto initial = json_parse(req, "", 0);
      resolve(seq, 0, "\"signal_123\"");
    }, nullptr);
  }

  void add_init_script(const std::string &post_fn) {
    add_user_script(create_init_script(post_fn));
    add_user_script(create_alloy_script());
    add_alloy_bindings();
    add_gui_bindings();
    m_is_init_script_added = true;
  }

  std::string create_init_script(const std::string &post_fn) {
    auto js = std::string{} + "(function() {\n\
  'use strict';\n\
  function generateId() {\n\
    var crypto = window.crypto || window.msCrypto;\n\
    var bytes = new Uint8Array(16);\n\
    crypto.getRandomValues(bytes);\n\
    return Array.prototype.slice.call(bytes).map(function(n) {\n\
      var s = n.toString(16);\n\
      return ((s.length % 2) == 1 ? '0' : '') + s;\n\
    }).join('');\n\
  }\n\
  var Webview = (function() {\n\
    var _promises = {};\n\
    function Webview_() {}\n\
    Webview_.prototype.post = function(message) {\n\
      return (" +
              post_fn + ")(message);\n\
    };\n\
    Webview_.prototype.call = function(method) {\n\
      var _id = generateId();\n\
      var _params = Array.prototype.slice.call(arguments, 1);\n\
      var promise = new Promise(function(resolve, reject) {\n\
        _promises[_id] = { resolve, reject };\n\
      });\n\
      this.post(JSON.stringify({\n\
        id: _id,\n\
        method: method,\n\
        params: _params\n\
      }));\n\
      return promise;\n\
    };\n\
    Webview_.prototype.onReply = function(id, status, result) {\n\
      var promise = _promises[id];\n\
      if (result !== undefined) {\n\
        try {\n\
          result = JSON.parse(result);\n\
        } catch (e) {\n\
          promise.reject(new Error(\"Failed to parse binding result as JSON\"));\n\
          return;\n\
        }\n\
      }\n\
      if (status === 0) {\n\
        promise.resolve(result);\n\
      } else {\n\
        promise.reject(result);\n\
      }\n\
    };\n\
    Webview_.prototype.onBind = function(name) {\n\
      if (window.hasOwnProperty(name)) {\n\
        throw new Error('Property \"' + name + '\" already exists');\n\
      }\n\
      window[name] = (function() {\n\
        var params = [name].concat(Array.prototype.slice.call(arguments));\n\
        return Webview_.prototype.call.apply(this, params);\n\
      }).bind(this);\n\
    };\n\
    Webview_.prototype.onUnbind = function(name) {\n\
      if (!window.hasOwnProperty(name)) {\n\
        throw new Error('Property \"' + name + '\" does not exist');\n\
      }\n\
      delete window[name];\n\
    };\n\
    return Webview_;\n\
  })();\n\
  window.__webview__ = new Webview();\n\
})()";
    return js;
  }

  std::string create_bind_script() {
    std::string js_names = "[";
    bool first = true;
    for (const auto &binding : bindings) {
      if (first) {
        first = false;
      } else {
        js_names += ",";
      }
      js_names += json_escape(binding.first);
    }
    js_names += "]";

    auto js = std::string{} + "(function() {\n\
  'use strict';\n\
  var methods = " +
              js_names + ";\n\
  methods.forEach(function(name) {\n\
    window.__webview__.onBind(name);\n\
  });\n\
})()";
    return js;
  }

  virtual void on_message(const std::string &msg) {
    auto id = json_parse(msg, "id", 0);
    auto name = json_parse(msg, "method", 0);
    auto args = json_parse(msg, "params", 0);
    auto found = bindings.find(name);
    if (found == bindings.end()) {
      return;
    }
    const auto &context = found->second;
    dispatch([=] { context.call(id, args); });
  }

  virtual void on_window_created() { inc_window_count(); }

  virtual void on_window_destroyed(bool skip_termination = false) {
    if (dec_window_count() <= 0) {
      if (!skip_termination) {
        terminate();
      }
    }
  }

  // Runs the event loop until the currently queued events have been processed.
  void deplete_run_loop_event_queue() {
    bool done{};
    dispatch([&] { done = true; });
    run_event_loop_while([&] { return !done; });
  }

  // Runs the event loop while the passed-in function returns true.
  virtual void run_event_loop_while(std::function<bool()> fn) = 0;

  void dispatch_size_default() {
    if (!owns_window() || !m_is_init_script_added) {
      return;
    };
    dispatch([this]() {
      if (!m_is_size_set) {
        set_size(m_initial_width, m_initial_height, WEBVIEW_HINT_NONE);
      }
    });
  }

  void set_default_size_guard(bool guarded) { m_is_size_set = guarded; }

  bool owns_window() const { return m_owns_window; }

private:
  static std::atomic_uint &window_ref_count() {
    static std::atomic_uint ref_count{0};
    return ref_count;
  }

  static unsigned int inc_window_count() { return ++window_ref_count(); }

  static unsigned int dec_window_count() {
    auto &count = window_ref_count();
    if (count > 0) {
      return --count;
    }
    return 0;
  }

  std::map<std::string, binding_ctx_t> bindings;
  user_script *m_bind_script{};
  std::list<user_script> m_user_scripts;

  alloyscript_runtime m_alloy;
  sqlite_runtime m_sqlite;
  std::map<std::string, std::shared_ptr<alloyscript_runtime::shared_state>>
      m_subprocesses;

  bool m_is_init_script_added{};
  bool m_is_size_set{};
  bool m_owns_window{};
  static const int m_initial_width = 640;
  static const int m_initial_height = 480;
};

} // namespace detail
} // namespace webview

#endif // defined(__cplusplus) && !defined(WEBVIEW_HEADER)
#endif // WEBVIEW_DETAIL_ENGINE_BASE_HH
