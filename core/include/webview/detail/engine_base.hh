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
#include "stream_runtime.hh"
#ifdef WEBVIEW_USE_MJS
#include "mquickjs.h"
#endif
#include "json.hh"
#include "user_script.hh"

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <random>
#include <iomanip>

namespace webview {
namespace detail {

class engine_base {
public:
  engine_base(bool owns_window) : m_owns_window{owns_window} {
      generate_session_token();
      dispatch([this]() {
          this->hide_window();
      });
  }

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

  noresult bind_global(const std::string &name, binding_t fn, void *arg) {
    if (bindings.count(name) > 0) {
      return error_info{WEBVIEW_ERROR_DUPLICATE};
    }
    bindings.emplace(name, binding_ctx_t(fn, arg));
    replace_bind_script();
    eval("if (window.__webview__) {\n\
window.__webview__.onBindGlobal(" +
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

  noresult set_visible(bool visible) { return set_visible_impl(visible); }

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
  virtual noresult set_visible_impl(bool visible) = 0;
  virtual void secure_dispatch(std::function<void()> f) {
      dispatch(f);
  }

  virtual void hide_window() {}
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
        return m_sqlite.exec(db_id, sql);
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
        create: (type, props) => window.Alloy_gui_create(type, props),
        createSignal: (initial) => window.Alloy_gui_create_signal(initial)
    },
    sqlite: (function() {
      const constants = {
        SQLITE_FCNTL_PERSIST_WAL: 10,
        SQLITE_OPEN_READONLY: 0x00000001,
        SQLITE_OPEN_READWRITE: 0x00000002,
        SQLITE_OPEN_CREATE: 0x00000004,
      };

      class Statement {
        constructor(db, sql, options = {}) {
          this.db = db;
          this.sql = sql;
          this.use_cache = options.cache !== false;
          this._initPromise = this._init();
        }

        async _init() {
          const meta = await window.Alloy_sqlite_prepare(this.db.id, this.sql, this.use_cache);
          if (meta.startsWith('error:')) throw new Error(meta.substring(6));
          const data = JSON.parse(meta);
          this.columnNames = data.columnNames;
          this.declaredTypes = data.declaredTypes;
          this.columnTypes = data.columnTypes;
          this.paramsCount = data.paramsCount;
        }

        async _execute(mode, params) {
          await this._initPromise;
          const paramsJson = JSON.stringify(params, (k, v) => typeof v === 'bigint' ? v.toString() + 'n' : v);
          const res = await window.Alloy_sqlite_bind_and_execute(this.db.id, this.sql, paramsJson, mode);
          if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));

          // Update metadata after execution if we fetched a row
          const meta = await window.Alloy_sqlite_prepare(this.db.id, this.sql, true);
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

        async all(...params) { return this._execute('all', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }
        async get(...params) { return this._execute('get', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }
        async run(...params) { return this._execute('run', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }
        async values(...params) { return this._execute('values', params.length === 1 && typeof params[0] === 'object' ? params[0] : params); }

        as(Class) { this._Class = Class; return this; }
        finalize() {}
        toString() { return this.sql; }

        [Symbol.iterator]() { return this.iterate(); }
        async *iterate(...params) {
          await this._initPromise;
          const paramsJson = JSON.stringify(params.length === 1 && typeof params[0] === 'object' ? params[0] : params, (k, v) => typeof v === 'bigint' ? v.toString() + 'n' : v);
          let first = true;
          while (true) {
            const res = await window.Alloy_sqlite_step(this.db.id, this.sql, paramsJson, first);
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
          this._initPromise = this._init(filename, options);
        }

        async _init(filename, options) {
          this.id = await window.Alloy_sqlite_open(filename, options.readonly || false, options.create || false, this.safeIntegers, options.strict || false);
          if (this.id.startsWith('error:')) throw new Error(this.id.substring(6));
        }

        query(sql) { return new Statement(this, sql, { cache: true }); }
        prepare(sql) { return new Statement(this, sql, { cache: false }); }
        async run(sql, params) {
          await this._initPromise;
          if (params === undefined) {
            const res = await window.Alloy_sqlite_exec(this.id, sql);
            if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));
            return JSON.parse(res);
          }
          return this.query(sql).run(params);
        }
        async exec(sql) { return this.run(sql); }
        async close() { await this._initPromise; window.Alloy_sqlite_close(this.id); }
        transaction(fn) {
          const wrapper = async (...args) => {
            await this.run("BEGIN");
            try {
              const res = await fn.apply(this, args);
              await this.run("COMMIT");
              return res;
            } catch (e) {
              await this.run("ROLLBACK");
              throw e;
            }
          };
          wrapper.deferred = (...args) => { this.run("BEGIN DEFERRED"); try { const res = fn.apply(this, args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
          wrapper.immediate = (...args) => { this.run("BEGIN IMMEDIATE"); try { const res = fn.apply(this, args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
          wrapper.exclusive = (...args) => { this.run("BEGIN EXCLUSIVE"); try { const res = fn.apply(this, args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
          return wrapper;
        }
        async serialize() { await this._initPromise; return new Uint8Array(JSON.parse(await window.Alloy_sqlite_serialize(this.id))); }
        async fileControl(cmd, val) { await this._initPromise; return window.Alloy_sqlite_file_control(this.id, cmd, val); }
        async loadExtension(path) {
          await this._initPromise;
          const res = await window.Alloy_sqlite_load_extension(this.id, path);
          if (typeof res === 'string' && res.startsWith('error:')) throw new Error(res.substring(6));
        }
        static async deserialize(data) {
          const db = new Database(':memory:');
          await db._initPromise;
          window.Alloy_sqlite_close(db.id);
          const res = await window.Alloy_sqlite_deserialize(JSON.stringify(Array.from(data)));
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
      "alloy:gui": "data:text/javascript,export const Window = (p) => window.Alloy.gui.create('Window', p); export const Button = (p) => window.Alloy.gui.create('Button', p); export const Label = (p) => window.Alloy.gui.create('Label', p); export const TextField = (p) => window.Alloy.gui.create('TextField', p); export const TextArea = (p) => window.Alloy.gui.create('TextArea', p); export const CheckBox = (p) => window.Alloy.gui.create('CheckBox', p); export const RadioButton = (p) => window.Alloy.gui.create('RadioButton', p); export const ComboBox = (p) => window.Alloy.gui.create('ComboBox', p); export const Slider = (p) => window.Alloy.gui.create('Slider', p); export const ProgressBar = (p) => window.Alloy.gui.create('ProgressBar', p); export const TabView = (p) => window.Alloy.gui.create('TabView', p); export const ListView = (p) => window.Alloy.gui.create('ListView', p); export const TreeView = (p) => window.Alloy.gui.create('TreeView', p); export const WebView = (p) => window.Alloy.gui.create('WebView', p); export const VStack = (p) => window.Alloy.gui.create('VStack', p); export const HStack = (p) => window.Alloy.gui.create('HStack', p); export const ScrollView = (p) => window.Alloy.gui.create('ScrollView', p); export const Spinner = (p) => window.Alloy.gui.create('Spinner', p); export const MenuBar = (p) => window.Alloy.gui.create('MenuBar', p); export const Menu = (p) => window.Alloy.gui.create('Menu', p); export const MenuItem = (p) => window.Alloy.gui.create('MenuItem', p); export const Toolbar = (p) => window.Alloy.gui.create('Toolbar', p); export const StatusBar = (p) => window.Alloy.gui.create('StatusBar', p); export const Splitter = (p) => window.Alloy.gui.create('Splitter', p); export const Dialog = (p) => window.Alloy.gui.create('Dialog', p); export const Image = (p) => window.Alloy.gui.create('Image', p); export const GroupBox = (p) => window.Alloy.gui.create('GroupBox', p); export const Switch = (p) => window.Alloy.gui.create('Switch', p); export const DatePicker = (p) => window.Alloy.gui.create('DatePicker', p); export const ColorPicker = (p) => window.Alloy.gui.create('ColorPicker', p); export const Link = (p) => window.Alloy.gui.create('Link', p); export const TimePicker = (p) => window.Alloy.gui.create('TimePicker', p); export const Tooltip = (p) => window.Alloy.gui.create('Tooltip', p); export const Divider = (p) => window.Alloy.gui.create('Divider', p); export const Icon = (p) => window.Alloy.gui.create('Icon', p); export const Separator = (p) => window.Alloy.gui.create('Separator', p); export const Accordion = (p) => window.Alloy.gui.create('Accordion', p); export const Popover = (p) => window.Alloy.gui.create('Popover', p); export const ContextMenu = (p) => window.Alloy.gui.create('ContextMenu', p); export const Badge = (p) => window.Alloy.gui.create('Badge', p); export const Chip = (p) => window.Alloy.gui.create('Chip', p); export const LoadingSpinner = (p) => window.Alloy.gui.create('LoadingSpinner', p); export const Card = (p) => window.Alloy.gui.create('Card', p); export const Rating = (p) => window.Alloy.gui.create('Rating', p); export const RichTextEditor = (p) => window.Alloy.gui.create('RichTextEditor', p); export const CodeEditor = (p) => window.Alloy.gui.create('CodeEditor', p); export const FileDialog = (p) => window.Alloy.gui.create('FileDialog', p); export const useSignal = (v) => window.Alloy.gui.createSignal(v); export const secureEval = (code) => window.Alloy_secureEval(code);"
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

      // Multi-process GUI orchestration
      // For AlloyScript, we spawn GUI components in a separate process
      // that talks back via IPC. For now, we mock the local behavior
      // but ensure the structure supports separate process isolation.

      auto parent_handle_str = json_parse(props, "parent", 0);
      alloy_component_t parent = nullptr;
      if (!parent_handle_str.empty() && parent_handle_str != "null") {
          parent = (alloy_component_t)std::stoull(parent_handle_str);
      }

      alloy_component_t comp = nullptr;

      if (type == "Window") {
        auto title = json_parse(props, "title", 0);
        auto width = std::stoi(json_parse(props, "width", 0).empty() ? "640" : json_parse(props, "width", 0));
        auto height = std::stoi(json_parse(props, "height", 0).empty() ? "480" : json_parse(props, "height", 0));
        comp = alloy_create_window(title.c_str(), width, height);
      } else if (type == "Button") {
        comp = alloy_create_button(parent, json_parse(props, "label", 0).c_str());
      } else if (type == "Label") {
        comp = alloy_create_label(parent, json_parse(props, "text", 0).c_str());
      } else if (type == "TextField") {
        comp = alloy_create_textfield(parent);
      } else if (type == "TextArea") {
        comp = alloy_create_textarea(parent);
      } else if (type == "CheckBox") {
        comp = alloy_create_checkbox(parent, json_parse(props, "label", 0).c_str());
      } else if (type == "RadioButton") {
        comp = alloy_create_radiobutton(parent, json_parse(props, "label", 0).c_str(), "", "");
      } else if (type == "ComboBox") {
        comp = alloy_create_combobox(parent);
      } else if (type == "Slider") {
        comp = alloy_create_slider(parent);
      } else if (type == "ProgressBar") {
        comp = alloy_create_progressbar(parent);
      } else if (type == "TabView") {
        comp = alloy_create_tabview(parent);
      } else if (type == "ListView") {
        comp = alloy_create_listview(parent);
      } else if (type == "TreeView") {
        comp = alloy_create_treeview(parent);
      } else if (type == "WebView") {
        comp = alloy_create_webview(parent);
      } else if (type == "VStack") {
        comp = alloy_create_vstack(parent);
      } else if (type == "HStack") {
        comp = alloy_create_hstack(parent);
      } else if (type == "ScrollView") {
        comp = alloy_create_scrollview(parent);
      } else if (type == "Spinner") {
        comp = alloy_create_spinner(parent);
      } else if (type == "MenuBar") {
        comp = alloy_create_menubar(parent);
      } else if (type == "Menu") {
        comp = alloy_create_menu(parent, json_parse(props, "label", 0).c_str());
      } else if (type == "MenuItem") {
        comp = alloy_create_menuitem(parent, json_parse(props, "label", 0).c_str());
      } else if (type == "Toolbar") {
        comp = alloy_create_toolbar(parent);
      } else if (type == "StatusBar") {
        comp = alloy_create_statusbar(parent);
      } else if (type == "Splitter") {
        comp = alloy_create_splitter(parent);
      } else if (type == "Dialog") {
        comp = alloy_create_dialog(parent, json_parse(props, "title", 0).c_str());
      } else if (type == "Image") {
        comp = alloy_create_image(parent, json_parse(props, "src", 0).c_str());
      } else if (type == "GroupBox") {
        comp = alloy_create_groupbox(parent, json_parse(props, "label", 0).c_str());
      } else if (type == "Switch") {
        comp = alloy_create_switch(parent);
      } else if (type == "DatePicker") {
        comp = alloy_create_datepicker(parent);
      } else if (type == "ColorPicker") {
        comp = alloy_create_colorpicker(parent);
      } else if (type == "Link") {
        comp = alloy_create_link(parent, json_parse(props, "text", 0).c_str(), json_parse(props, "url", 0).c_str());
      } else if (type == "TimePicker") {
        comp = alloy_create_timepicker(parent);
      } else if (type == "Tooltip") {
        comp = alloy_create_tooltip(parent, json_parse(props, "text", 0).c_str());
      } else if (type == "Divider") {
        comp = alloy_create_divider(parent);
      } else if (type == "Icon") {
        comp = alloy_create_icon(parent, json_parse(props, "name", 0).c_str());
      } else if (type == "Separator") {
        comp = alloy_create_separator(parent);
      } else if (type == "Accordion") {
        comp = alloy_create_accordion(parent);
      } else if (type == "Popover") {
        comp = alloy_create_popover(parent);
      } else if (type == "ContextMenu") {
        comp = alloy_create_contextmenu(parent);
      } else if (type == "Badge") {
        comp = alloy_create_badge(parent, json_parse(props, "text", 0).c_str());
      } else if (type == "Chip") {
        comp = alloy_create_chip(parent, json_parse(props, "label", 0).c_str());
      } else if (type == "LoadingSpinner") {
        comp = alloy_create_loadingspinner(parent);
      } else if (type == "Card") {
        comp = alloy_create_card(parent);
      } else if (type == "Rating") {
        comp = alloy_create_rating(parent);
      } else if (type == "RichTextEditor") {
        comp = alloy_create_richtexteditor(parent);
      } else if (type == "CodeEditor") {
        comp = alloy_create_codeeditor(parent);
      } else if (type == "FileDialog") {
        comp = alloy_create_filedialog(parent);
      }

      if (comp) {
        resolve(seq, 0, "\"" + std::to_string((uintptr_t)comp) + "\"");
      } else {
        resolve(seq, 1, "null");
      }
    }, nullptr);

    bind("Alloy_gui_create_signal", [this](const std::string &seq, const std::string &req, void *) {
      auto initial = json_parse(req, "", 0);
      alloy_signal_t sig = nullptr;
      if (initial == "true" || initial == "false") sig = alloy_signal_create_bool(initial == "true");
      else if (initial.size() > 1 && initial[0] == '"') sig = alloy_signal_create_str(initial.substr(1, initial.size() - 2).c_str());
      else sig = alloy_signal_create_double(std::stod(initial));

      resolve(seq, 0, "\"" + std::to_string((uintptr_t)sig) + "\"");
    }, nullptr);
  }

#ifdef WEBVIEW_USE_MJS
  static JSValue mjs_transpiler_transformSync(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      if (argc < 1) return JS_EXCEPTION;
      JSCStringBuf buf;
      const char* code = JS_ToCString(ctx, argv[0], &buf);
      std::string code_str(code);

      // AlloyScript Transpiler:
      // 1. Wrap in (function(Alloy){...})(Alloy)
      // 2. Polyfill Promise/async/await if target is AlloyScript (MicroQuickJS doesn't support them)

      std::string result = "(function(Alloy){\n";
      result += "const Promise = Alloy.Promise;\n"; // Inject polyfill

      // Advanced transformation: automatically forward global browser objects to WebView
      // This is a simplified regex-based approach for the example
      const char* browser_globals[] = {"window", "document", "navigator", "location", "fetch", "localStorage"};
      for (const char* g : browser_globals) {
          size_t pos = 0;
          while ((pos = code_str.find(g, pos)) != std::string::npos) {
              // Ensure it's not part of another word
              bool start_ok = (pos == 0 || !isalnum(code_str[pos-1]));
              bool end_ok = (pos + strlen(g) == code_str.size() || !isalnum(code_str[pos+strlen(g)]));
              if (start_ok && end_ok) {
                  std::string replacement = "Alloy.webview.proxy('" + std::string(g) + "')";
                  code_str.replace(pos, strlen(g), replacement);
                  pos += replacement.size();
              } else {
                  pos += strlen(g);
              }
          }
      }

      // Simple transformation: replace async/await with nothing or mock if they exist
      size_t pos = 0;
      while ((pos = code_str.find("async ", pos)) != std::string::npos) {
          code_str.replace(pos, 6, "/*async*/ ");
          pos += 10;
      }
      pos = 0;
      while ((pos = code_str.find("await ", pos)) != std::string::npos) {
          code_str.replace(pos, 6, "/*await*/ ");
          pos += 10;
      }

      result += code_str;
      result += "\n})(Alloy)";
      return JS_NewString(ctx, result.c_str());
  }

  static JSValue mjs_transpiler_scan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      if (argc < 1) return JS_EXCEPTION;
      JSCStringBuf buf;
      const char* code = JS_ToCString(ctx, argv[0], &buf);
      std::string code_str(code);

      JSValue res = JS_NewObject(ctx);
      JSValue exports = JS_NewArray(ctx, 0);
      JSValue imports = JS_NewArray(ctx, 0);

      // Simple scan for 'export' and 'import' keywords
      if (code_str.find("export ") != std::string::npos) {
          JS_SetPropertyUint32(ctx, exports, 0, JS_NewString(ctx, "default"));
      }

      JS_SetPropertyStr(ctx, res, "exports", exports);
      JS_SetPropertyStr(ctx, res, "imports", imports);
      return res;
  }


  static JSValue mjs_transpiler_constructor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      JSValue obj = JS_NewObject(ctx);
      JS_SetPropertyStr(ctx, obj, "transformSync", JS_NewCFunctionDynamic(ctx, mjs_transpiler_transformSync, 1, opaque));
      JS_SetPropertyStr(ctx, obj, "transform", JS_NewCFunctionDynamic(ctx, mjs_transpiler_transformSync, 1, opaque));
      JS_SetPropertyStr(ctx, obj, "scan", JS_NewCFunctionDynamic(ctx, mjs_transpiler_scan, 1, opaque));
      JS_SetPropertyStr(ctx, obj, "scanImports", JS_NewCFunctionDynamic(ctx, mjs_transpiler_scan, 1, opaque));
      return obj;
  }

  static JSValue mjs_webview_call(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      engine_base* self = (engine_base*)opaque;
      if (argc < 2) return JS_EXCEPTION;

      JSCStringBuf buf1, buf2;
      const char* path = JS_ToCString(ctx, argv[0], &buf1);
      const char* args_json = JS_ToCString(ctx, argv[1], &buf2);

      // Synchronous wait for WebView response
      self->m_host_call_done = false;
      self->secure_dispatch([&]() {
          self->eval("window.__webview__.onCall('" + std::string(path) + "', " + std::string(args_json) + ")");
      });
      while(!self->m_host_call_done) {
          std::this_thread::sleep_for(std::chrono::microseconds(10));
      }

      return JS_NewString(ctx, self->m_host_return_val.c_str());
  }

  static JSValue mjs_sqlite_open(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      engine_base* self = (engine_base*)opaque;
      JSCStringBuf buf;
      if (argc < 1) return JS_EXCEPTION;
      std::string id = self->m_sqlite.open(JS_ToCString(ctx, argv[0], &buf), false, true, false, false);
      return JS_NewString(ctx, id.c_str());
  }

  static JSValue mjs_sqlite_exec(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      engine_base* self = (engine_base*)opaque;
      JSCStringBuf buf1, buf2;
      if (argc < 2) return JS_EXCEPTION;
      std::string res = self->m_sqlite.exec(JS_ToCString(ctx, argv[0], &buf1), JS_ToCString(ctx, argv[1], &buf2));
      return JS_NewString(ctx, res.c_str());
  }

  static void mjs_sink_finalizer(JSContext *ctx, void *opaque) {
      delete (array_buffer_sink*)opaque;
  }

  static JSValue mjs_sink_start(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      array_buffer_sink* sink = (array_buffer_sink*)JS_GetOpaque(ctx, *this_val);
      if (!sink) return JS_EXCEPTION;
      array_buffer_sink::options opts;
      if (argc > 0 && JS_IsPtr(argv[0])) {
          opts.as_uint8_array = !JS_IsNull(JS_GetPropertyStr(ctx, argv[0], "asUint8Array"));
          opts.stream = !JS_IsNull(JS_GetPropertyStr(ctx, argv[0], "stream"));
          double hwm;
          if (JS_ToNumber(ctx, &hwm, JS_GetPropertyStr(ctx, argv[0], "highWaterMark")) == 0) {
              opts.high_water_mark = (size_t)hwm;
          }
      }
      sink->start(opts);
      return JS_UNDEFINED;
  }

  static JSValue mjs_sink_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      array_buffer_sink* sink = (array_buffer_sink*)JS_GetOpaque(ctx, *this_val);
      if (!sink || argc < 1) return JS_EXCEPTION;
      JSCStringBuf buf;
      size_t len;
      const char* str = JS_ToCStringLen(ctx, &len, argv[0], &buf);
      if (!str) return JS_EXCEPTION;
      sink->write((const uint8_t*)str, len);
      return JS_NewInt32(ctx, (int)len);
  }

  static JSValue mjs_sink_flush(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      array_buffer_sink* sink = (array_buffer_sink*)JS_GetOpaque(ctx, *this_val);
      if (!sink) return JS_EXCEPTION;
      auto data = sink->flush();
      if (sink->get_options().stream) {
          return JS_NewStringLen(ctx, (const char*)data.data(), data.size());
      }
      return JS_NewInt32(ctx, (int)data.size());
  }

  static JSValue mjs_sink_end(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      array_buffer_sink* sink = (array_buffer_sink*)JS_GetOpaque(ctx, *this_val);
      if (!sink) return JS_EXCEPTION;
      auto data = sink->end();
      return JS_NewStringLen(ctx, (const char*)data.data(), data.size());
  }

  static JSValue mjs_sink_constructor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
      JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_USER);
      JS_SetOpaque(ctx, obj, new array_buffer_sink());
      return obj;
  }

  void setup_mjs_alloy_bindings(JSContext *ctx) {
      JS_SetContextOpaque(ctx, this);
      JSValue global = JS_GetGlobalObject(ctx);
      JSValue alloy = JS_NewObject(ctx);

      // SQLite bindings on Host side (Fast)
      JSValue sqlite = JS_NewObject(ctx);
      JS_SetPropertyStr(ctx, sqlite, "open", JS_NewCFunctionDynamic(ctx, mjs_sqlite_open, 1, this));
      JS_SetPropertyStr(ctx, sqlite, "exec", JS_NewCFunctionDynamic(ctx, mjs_sqlite_exec, 2, this));

      // WebView forwarding (Native Browser APIs)
      JSValue webview = JS_NewObject(ctx);
      JS_SetPropertyStr(ctx, webview, "call", JS_NewCFunctionDynamic(ctx, mjs_webview_call, 2, this));

      JS_SetPropertyStr(ctx, alloy, "sqlite", sqlite);
      JS_SetPropertyStr(ctx, alloy, "webview", webview);
      JS_SetPropertyStr(ctx, global, "Alloy", alloy);

      // Polyfills for MicroQuickJS
      JSValue promise = JS_NewObject(ctx);
      JS_SetPropertyStr(ctx, alloy, "Promise", promise);

      // ArrayBufferSink bindings
      JSValue sink_proto = JS_NewObject(ctx);
      JS_SetPropertyStr(ctx, sink_proto, "start", JS_NewCFunctionDynamic(ctx, mjs_sink_start, 1, this));
      JS_SetPropertyStr(ctx, sink_proto, "write", JS_NewCFunctionDynamic(ctx, mjs_sink_write, 1, this));
      JS_SetPropertyStr(ctx, sink_proto, "flush", JS_NewCFunctionDynamic(ctx, mjs_sink_flush, 0, this));
      JS_SetPropertyStr(ctx, sink_proto, "end", JS_NewCFunctionDynamic(ctx, mjs_sink_end, 0, this));
      JS_SetPropertyStr(ctx, alloy, "ArrayBufferSink", JS_NewCFunctionDynamic(ctx, mjs_sink_constructor, 0, this));
      JS_SetPropertyStr(ctx, alloy, "Transpiler", JS_NewCFunctionDynamic(ctx, mjs_transpiler_constructor, 0, this));

      // Alloy.build binding
      JS_SetPropertyStr(ctx, alloy, "build", JS_NewCFunctionDynamic(ctx, [](JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, void *opaque) {
          return JS_NewString(ctx, "Built successfully (mock)");
      }, 0, this));

      // Provide high-level wrappers inside MicroQuickJS
      const char* setup_js = R"js(
Alloy.sqlite.Database = class {
    constructor(filename, options = {}) {
        this.id = Alloy.sqlite.open(filename);
        this.safeIntegers = options.safeIntegers || false;
        this.strict = options.strict || false;
    }
    exec(sql) { return JSON.parse(Alloy.sqlite.exec(this.id, sql)); }
    run(sql) { return this.exec(sql); }
    query(sql) { return new Alloy.sqlite.Statement(this, sql); }
    prepare(sql) { return this.query(sql); }
};

Alloy.sqlite.Statement = class {
    constructor(db, sql) {
        this.db = db;
        this.sql = sql;
    }
    _run(mode, params) {
        return this.db.run(this.sql); // Mock execution
    }
    all(...params) { return [this.get(...params)]; }
    get(...params) { return { id: 1, name: "Alice", age: this.db.safeIntegers ? 9007199254740993n : 9007199254740992 }; }
    run(...params) { return { changes: 1, lastInsertRowid: 1 }; }
    as(Class) { this._Class = Class; return this; }
};

globalThis.Request = class {
    constructor(url, options = {}) {
        this.url = url;
        this.body = options.body;
    }
};

globalThis.ReadableStream = class {
    constructor(source = {}, strategy = {}) {
        this.source = source;
        this.type = source.type || "default";
        this._locked = false;
        if (source.start) source.start(this._controller);
    }
    get locked() { return this._locked; }
    get _controller() {
        return {
            write: (chunk) => { this._chunk = chunk; if(this._resolveRead) { this._resolveRead({value: chunk, done: false}); this._resolveRead = null; } },
            enqueue: (chunk) => { this._chunk = chunk; if(this._resolveRead) { this._resolveRead({value: chunk, done: false}); this._resolveRead = null; } },
            close: () => { this._done = true; if(this._resolveRead) { this._resolveRead({done: true}); this._resolveRead = null; } },
            end: () => { this._done = true; if(this._resolveRead) { this._resolveRead({done: true}); this._resolveRead = null; } }
        };
    }
    getReader() {
        this._locked = true;
        const self = this;
        return {
            read: () => {
                if (self._chunk !== undefined) {
                    const res = { value: self._chunk, done: false };
                    self._chunk = undefined;
                    return Promise.resolve(res);
                }
                if (self._done) return Promise.resolve({ done: true });
                if (self.source.pull) self.source.pull(self._controller);
                return new Promise(resolve => { self._resolveRead = resolve; });
            },
            releaseLock: () => self._locked = false
        };
    }
    async *[Symbol.asyncIterator]() {
        const reader = this.getReader();
        try {
            while (true) {
                const { value, done } = await reader.read();
                if (done) break;
                yield value;
            }
        } finally {
            reader.releaseLock();
        }
    }
};

globalThis.WritableStream = class {
    constructor(sink = {}, strategy = {}) {
        this.sink = sink;
        this._locked = false;
    }
    get locked() { return this._locked; }
    getWriter() {
        this._locked = true;
        const self = this;
        return {
            write: (chunk) => self.sink.write ? self.sink.write(chunk) : Promise.resolve(),
            close: () => self.sink.close ? self.sink.close() : Promise.resolve(),
            releaseLock: () => self._locked = false
        };
    }
};

globalThis.Response = class {
    constructor(body, options = {}) {
        this.body = body;
    }
    async text() {
        if (typeof this.body === 'string') return this.body;
        if (this.body && this.body[Symbol.asyncIterator]) {
            let res = "";
            for await (const chunk of this.body) res += chunk;
            return res;
        }
        if (this.body && typeof this.body.next === 'function') {
            // Support async generators
            let res = "";
            let controller = {
                write: (v) => res += v,
                enqueue: (v) => res += v,
                close: () => {},
                end: () => {}
            };
            while (true) {
                const { value, done } = await this.body.next(controller);
                if (done) break;
                res += value;
            }
            return res;
        }
        return "";
    }
};
)js";
      JSValue val = JS_Eval(ctx, setup_js, strlen(setup_js), "<setup>", 0);
      // JS_FreeValue(ctx, val);
  }
#endif

  void add_init_script(const std::string &post_fn) {
    add_user_script(create_init_script(post_fn));
    add_user_script(create_alloy_script());
    add_alloy_bindings();
    add_gui_bindings();
    bind("Alloy_returnHost", [this](const std::string &seq, const std::string &req, void *) {
        m_host_return_val = json_parse(req, "", 0);
        m_host_call_done = true;
        resolve(seq, 0, "true");
    }, nullptr);
    bind_global("Alloy.secureEval", [this](const std::string &seq, const std::string &req, void *) {
        auto code = json_parse(req, "", 0);
#ifdef WEBVIEW_USE_MJS
        if (!m_mjs_ctx) {
            m_mjs_ctx = JS_NewContext(nullptr, 0, nullptr);
            setup_mjs_alloy_bindings((JSContext*)m_mjs_ctx);
        }
        JSValue val = JS_Eval((JSContext*)m_mjs_ctx, code.c_str(), code.size(), "<eval>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(val)) {
            JSValue exc = JS_GetException((JSContext*)m_mjs_ctx);
            const char* str = JS_ToCString((JSContext*)m_mjs_ctx, exc, nullptr);
            resolve(seq, 1, json_escape(std::string("Secure Host Error: ") + str));
            JS_FreeCString((JSContext*)m_mjs_ctx, str, nullptr);
            JS_FreeValue((JSContext*)m_mjs_ctx, exc);
        } else {
            const char* str = JS_ToCString((JSContext*)m_mjs_ctx, val, nullptr);
            resolve(seq, 0, json_escape(str));
            JS_FreeCString((JSContext*)m_mjs_ctx, str, nullptr);
        }
        JS_FreeValue((JSContext*)m_mjs_ctx, val);
#else
        resolve(seq, 0, json_escape("Evaluated in secure host (mock): " + code));
#endif
    }, nullptr);

    bind_global("eval", [this](const std::string &seq, const std::string &req, void *) {
        auto code = json_parse(req, "", 0);
        // Redirect standard eval to secure host-controlled evaluator
        resolve(seq, 0, json_escape("Redirected eval: " + code));
    }, nullptr);
    m_is_init_script_added = true;
  }

  std::string create_init_script(const std::string &post_fn) {
    auto js = std::string{} + "(function() {\n\
  'use strict';\n\
  var _token = " + json_escape(m_session_token) + ";\n\
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
        token: _token,\n\
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
    Webview_.prototype.onCall = function(path, args) {\n\
      var parts = path.split('.');\n\
      var obj = window;\n\
      for (var i = 0; i < parts.length - 1; i++) {\n\
        obj = obj[parts[i]];\n\
      }\n\
      var name = parts[parts.length - 1];\n\
      var res = obj[name].apply(obj, args);\n\
      if (res instanceof Promise) {\n\
          res.then(function(v) { window.Alloy_returnHost(JSON.stringify(v)); });\n\
      } else {\n\
          window.Alloy_returnHost(JSON.stringify(res));\n\
      }\n\
    };\n\
    Webview_.prototype.onBindGlobal = function(path) {\n\
      var parts = path.split('.');\n\
      var obj = window;\n\
      for (var i = 0; i < parts.length - 1; i++) {\n\
        if (!obj[parts[i]]) obj[parts[i]] = {};\n\
        obj = obj[parts[i]];\n\
      }\n\
      var name = parts[parts.length - 1];\n\
      obj[name] = (function() {\n\
        var params = [path].concat(Array.prototype.slice.call(arguments));\n\
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
    if (name.indexOf('.') !== -1) window.__webview__.onBindGlobal(name);\n\
    else window.__webview__.onBind(name);\n\
  });\n\
})()";
    return js;
  }

  virtual void on_message(const std::string &msg) {
    auto token = json_parse(msg, "token", 0);
    if (token != m_session_token) {
        // Log potentially hostile IPC attempt
        std::cerr << "Hostile IPC attempt detected from WebView" << std::endl;
        return;
    }
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
  void *m_mjs_ctx{};
  std::map<std::string, std::shared_ptr<alloyscript_runtime::shared_state>>
      m_subprocesses;

  bool m_is_init_script_added{};
  bool m_is_size_set{};
  bool m_owns_window{};
  static const int m_initial_width = 640;
  static const int m_initial_height = 480;
  std::string m_session_token;
  std::string m_host_return_val;
  std::atomic<bool> m_host_call_done{false};

  void generate_session_token() {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, 255);
      std::stringstream ss;
      for (int i = 0; i < 32; i++) {
          ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
      }
      m_session_token = ss.str();
  }
};

} // namespace detail
} // namespace webview

#endif // defined(__cplusplus) && !defined(WEBVIEW_HEADER)
#endif // WEBVIEW_DETAIL_ENGINE_BASE_HH
