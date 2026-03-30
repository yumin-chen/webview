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
#include "cron.hh"
#include "json.hh"
#include "sqlite.hh"
#include "subprocess.hh"
#include "user_script.hh"

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <string>

namespace webview {
namespace detail {

class engine_base {
public:
  engine_base(bool owns_window) : m_owns_window{owns_window} {}

  virtual ~engine_base() = default;

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

  void add_init_script(const std::string &post_fn) {
    add_user_script(create_alloy_script());
    add_user_script(create_init_script(post_fn));
    m_is_init_script_added = true;

    bind("__alloy_spawn_sync", [this](const std::string &req) -> std::string {
      auto cmd_json = json_parse(req, "", 0);
      auto opts_json = json_parse(req, "", 1);

      std::vector<std::string> cmd;
      if (cmd_json[0] == '[') {
        for (int i = 0;; ++i) {
          auto arg = json_parse(cmd_json, "", i);
          if (arg.empty() && i > 0)
            break;
          if (!arg.empty())
            cmd.push_back(arg);
          else if (i == 0)
            break;
        }
      } else if (cmd_json[0] == '"') {
        cmd.push_back(json_parse(req, "", 0));
      }

      subprocess::options opts;
      opts.cmd = cmd;
      opts.cwd = json_parse(opts_json, "cwd", 0);

      std::string stdout_data, stderr_data;
      int exit_code = -1;
      bool finished = false;

      subprocess proc(opts);
      proc.spawn(
          [&](const std::string &data, bool is_stderr) {
            if (is_stderr)
              stderr_data += data;
            else
              stdout_data += data;
          },
          [&](int code) {
            exit_code = code;
            finished = true;
          });

      while (!finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      return "{\"stdout\":" + json_escape(stdout_data) +
             ",\"stderr\":" + json_escape(stderr_data) +
             ",\"exitCode\":" + std::to_string(exit_code) +
             ",\"success\":" + (exit_code == 0 ? "true" : "false") + "}";
    });

    bind("__alloy_spawn_bridge", [this](const std::string &req) -> std::string {
      auto cmd_json = json_parse(req, "", 0);
      auto opts_json = json_parse(req, "", 1);

      std::vector<std::string> cmd;
      if (cmd_json[0] == '[') {
        for (int i = 0;; ++i) {
          auto arg = json_parse(cmd_json, "", i);
          if (arg.empty() && i > 0)
            break;
          if (!arg.empty()) {
            cmd.push_back(arg);
          } else if (i == 0) {
            break;
          }
        }
      } else if (cmd_json[0] == '"') {
        cmd.push_back(json_parse(req, "", 0));
      }

      subprocess::options opts;
      opts.cmd = cmd;
      opts.cwd = json_parse(opts_json, "cwd", 0);
      auto term_json = json_parse(opts_json, "terminal", 0);
      if (!term_json.empty() && term_json != "undefined" &&
          term_json != "null") {
        opts.use_terminal = true;
        auto cols = json_parse(term_json, "cols", 0);
        auto rows = json_parse(term_json, "rows", 0);
        if (!cols.empty())
          opts.terminal.cols = std::stoi(cols);
        if (!rows.empty())
          opts.terminal.rows = std::stoi(rows);
      }

      auto env_json = json_parse(opts_json, "env", 0);
      if (!env_json.empty() && env_json[0] == '{') {
        // Simple manual parsing for some env vars
        // This is tricky without a real JSON parser.
        // For now, let's just support passing a few known ones or keep it empty.
      }

      auto proc = std::make_shared<subprocess>(opts);
      auto proc_id = std::to_string(reinterpret_cast<uintptr_t>(proc.get()));
      m_subprocesses[proc_id] = proc;

      bool success = proc->spawn(
          [this, proc_id](const std::string &data, bool is_stderr) {
            std::string js = "window.Alloy.__onData(" + json_escape(proc_id) +
                             ", " + json_escape(data) + ", " +
                             (is_stderr ? "true" : "false") + ")";
            dispatch([this, js] { eval(js); });
          },
          [this, proc_id](int exit_code) {
            std::string js = "window.Alloy.__onExit(" + json_escape(proc_id) +
                             ", " + std::to_string(exit_code) + ")";
            dispatch([this, js] { eval(js); });
          });

      if (success) {
        return "{\"id\":" + json_escape(proc_id) +
               ",\"pid\":" + std::to_string(proc->get_pid()) + "}";
      } else {
        return "{\"error\":\"Failed to spawn\"}";
      }
    });

    bind("__alloy_kill", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->kill();
        return "true";
      }
      return "false";
    });

    bind("__alloy_stdin_write", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto data = json_parse(req, "", 1);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->write_stdin(data);
        return "true";
      }
      return "false";
    });

    bind("__alloy_stdin_close", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->close_stdin();
        return "true";
      }
      return "false";
    });

    bind("__alloy_send", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto message = json_parse(req, "", 1);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        // Simple IPC via stdin for now as a fallback
        it->second->write_stdin(message + "\n");
        return "true";
      }
      return "false";
    });

    bind("__alloy_cleanup", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      m_subprocesses.erase(proc_id);
      return "true";
    });

    bind("__alloy_cron_register", [this](const std::string &req) -> std::string {
      auto path = json_parse(req, "", 0);
      auto schedule = json_parse(req, "", 1);
      auto title = json_parse(req, "", 2);
      return cron_manager::register_job(path, schedule, title) ? "true" : "false";
    });

    bind("__alloy_cron_remove", [this](const std::string &req) -> std::string {
      auto title = json_parse(req, "", 0);
      return cron_manager::remove_job(title) ? "true" : "false";
    });

    bind("__alloy_sqlite_open", [this](const std::string &req) -> std::string {
      auto filename = json_parse(req, "", 0);
      try {
        auto db = std::make_shared<sqlite_db>(filename);
        auto id = std::to_string(reinterpret_cast<uintptr_t>(db.get()));
        m_sqlite_dbs[id] = db;
        return id;
      } catch (const std::exception &e) {
        return "{\"error\":" + json_escape(e.what()) + "}";
      }
    });

    bind("__alloy_sqlite_query", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto sql = json_parse(req, "", 1);
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        try {
          auto stmt = std::make_shared<sqlite_stmt>(it->second->get_native(), sql);
          auto id = std::to_string(reinterpret_cast<uintptr_t>(stmt.get()));
          m_sqlite_stmts[id] = stmt;
          return id;
        } catch (const std::exception &e) {
          return "{\"error\":" + json_escape(e.what()) + "}";
        }
      }
      return "{\"error\":\"DB not found\"}";
    });

    bind("__alloy_sqlite_step", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        return it->second->step();
      }
      return "";
    });

    bind("__alloy_sqlite_reset", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        it->second->reset();
        return "true";
      }
      return "false";
    });

    bind("__alloy_sqlite_bind", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto index_str = json_parse(req, "", 1);
      auto val = json_parse(req, "", 2);
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        int index = std::stoi(index_str);
        if (val == "null") it->second->bind_null(index);
        else it->second->bind_text(index, val);
        return "true";
      }
      return "false";
    });

    bind("__alloy_sqlite_serialize", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        auto data = it->second->serialize();
        // Return as hex or something similar for simple bridge
        std::string hex = "";
        for (auto b : data) {
          char buf[3];
          sprintf(buf, "%02x", b);
          hex += buf;
        }
        return hex;
      }
      return "";
    });
  }

  std::string create_alloy_script() {
    return R"js(
(function() {
  'use strict';
  if (window.Alloy) return;

  class Subprocess {
    constructor(id, pid) {
      this.id = id;
      this.pid = pid;
      this.exitCode = null;
      this.killed = false;
      this._exitedPromise = new Promise(resolve => {
        this._resolveExited = resolve;
      });

      this._stdoutController = null;
      this.stdout = new ReadableStream({
        start: (controller) => { this._stdoutController = controller; }
      });

      this._stderrController = null;
      this.stderr = new ReadableStream({
        start: (controller) => { this._stderrController = controller; }
      });

      this.stdin = {
        write: (data) => window.__alloy_stdin_write(this.id, data),
        end: () => window.__alloy_stdin_close(this.id),
        flush: () => {}
      };
    }
    get exited() { return this._exitedPromise; }
    kill(sig) {
      this.killed = true;
      return window.__alloy_kill(this.id, sig);
    }
    send(message) {
      return window.__alloy_send(this.id, JSON.stringify(message));
    }
  }

  const subprocesses = {};

  window.Alloy = {
    cron: (function() {
      const cron = async function(path, schedule, title) {
        return window.__alloy_cron_register(path, schedule, title);
      };
      cron.remove = async function(title) {
        return window.__alloy_cron_remove(title);
      };
      cron.parse = function(expression, relativeDate) {
        // Basic parser stub for illustration
        return new Date();
      };
      return cron;
    })(),
    spawn: function(cmd, opts) {
        const arg = (Array.isArray(cmd)) ? cmd : (cmd && cmd.cmd ? cmd : [cmd]);
        const options = (Array.isArray(cmd)) ? opts : cmd;
        // __alloy_spawn is still async due to webview::bind,
        // but we can make it look sync if we use a sync binding that returns immediately
        // and does the spawning in background.
        const res = JSON.parse(window.__alloy_spawn_bridge(arg, options));
      if (res.error) throw new Error(res.error);
      const proc = new Subprocess(res.id, res.pid);
      subprocesses[res.id] = proc;
      if (options && options.ipc) {
        proc._ipcHandler = options.ipc;
      }
      return proc;
    },
    spawnSync: function(cmd, opts) {
        const arg = (Array.isArray(cmd)) ? cmd : (cmd && cmd.cmd ? cmd : [cmd]);
        const options = (Array.isArray(cmd)) ? opts : cmd;
        const res = window.__alloy_spawn_sync(arg, options);
        return JSON.parse(res);
    },
    __onData: function(id, data, isStderr) {
      const proc = subprocesses[id];
      if (proc) {
        const encoder = new TextEncoder();
        const uint8 = encoder.encode(data);
        if (isStderr) {
          if (proc._stderrController) proc._stderrController.enqueue(uint8);
        } else {
          if (proc._stdoutController) proc._stdoutController.enqueue(uint8);
        }
      }
    },
    __onExit: function(id, exitCode) {
      const proc = subprocesses[id];
      if (proc) {
        proc.exitCode = exitCode;
        if (proc._stdoutController) proc._stdoutController.close();
        if (proc._stderrController) proc._stderrController.close();
        proc._resolveExited(exitCode);
        delete subprocesses[id];
        window.__alloy_cleanup(id);
      }
    },
    __onMessage: function(id, message) {
      const proc = subprocesses[id];
      if (proc && proc._ipcHandler) {
        proc._ipcHandler(JSON.parse(message), proc);
      }
    }
  };
})();
)js";
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
  std::map<std::string, std::shared_ptr<subprocess>> m_subprocesses;
  std::map<std::string, std::shared_ptr<sqlite_db>> m_sqlite_dbs;
  std::map<std::string, std::shared_ptr<sqlite_stmt>> m_sqlite_stmts;
  user_script *m_bind_script{};
  std::list<user_script> m_user_scripts;

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
