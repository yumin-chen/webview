/*
 * AlloyScript Runtime - CC0 Unlicense Public Domain
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

#ifndef ALLOY_ENGINE_ENGINE_BASE_HH
#define ALLOY_ENGINE_ENGINE_BASE_HH

#include "webview/errors.hh"
#include "webview/types.h"
#include "webview/types.hh"
#include "webview/detail/json.hh"
#include "webview/detail/user_script.hh"
#include "cron.hh"
#include "sqlite.hh"
#include "subprocess.hh"
#include "mquickjs.h"

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <sys/stat.h>
#include <fstream>

namespace alloy::engine {

using namespace webview;
using namespace webview::detail;

class engine_base {
public:
  engine_base(bool owns_window) : m_owns_window{owns_window} {
      m_ipc_secret = std::to_string(rand()) + std::to_string(rand());
      m_qjs_mem = (uint8_t*)malloc(2 * 1024 * 1024); // 2MB
      m_qjs_ctx = JS_NewContext(m_qjs_mem, 2 * 1024 * 1024, NULL);

      JSValue *global_obj = JS_PushGCRef(m_qjs_ctx, &m_qjs_global_ref);
      JSValue *alloy_obj = JS_PushGCRef(m_qjs_ctx, &m_qjs_alloy_ref);

      *global_obj = JS_GetGlobalObject(m_qjs_ctx);
      *alloy_obj = JS_NewObject(m_qjs_ctx);

      JS_SetPropertyStr(m_qjs_ctx, *alloy_obj, "log",
          JS_NewCFunction(m_qjs_ctx, js_alloy_log, "log", 1));

      JS_SetPropertyStr(m_qjs_ctx, *alloy_obj, "nativeCall",
          JS_NewCFunction(m_qjs_ctx, [](JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) -> JSValue {
              auto engine = (engine_base*)JS_GetContextOpaque(ctx);
              const char *method = JS_ToCString(ctx, argv[0]);
              const char *params = JS_ToCString(ctx, argv[1]);

              if (std::string(method).find("browser.") == 0) {
                  std::string payload = "{\"method\":" + json_escape(method) + ",\"params\":" + params + "}";
                  std::string signature = engine->sign_payload(payload);
                  std::string js = "window.__alloy_ipc_receive(" + json_escape(payload) + "," + json_escape(signature) + ")";
                  engine->eval(js);
              } else if (std::string(method) == "core.getSecret") {
                  return JS_NewString(ctx, engine->m_ipc_secret.c_str());
              } else {
                  auto it = engine->bindings.find(method);
                  if (it != engine->bindings.end()) {
                      it->second.ctx.call("mqjs-direct", params);
                  }
              }
              return JS_UNDEFINED;
          }, "nativeCall", 2));

      JS_DefinePropertyStr(m_qjs_ctx, *global_obj, "Alloy", *alloy_obj,
          JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);

      JS_SetContextOpaque(m_qjs_ctx, this);
  }

  virtual ~engine_base() {
      JS_PopGCRef(m_qjs_ctx, &m_qjs_alloy_ref);
      JS_PopGCRef(m_qjs_ctx, &m_qjs_global_ref);
      JS_FreeContext(m_qjs_ctx);
      free(m_qjs_mem);
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
    binding_t m_callback;
    void *m_arg;
  };

  struct binding_info_t {
      binding_ctx_t ctx;
      bool is_global;
      bool is_secure;
  };

  using sync_binding_t = std::function<std::string(std::string)>;

  noresult bind(const std::string &name, sync_binding_t fn) {
    auto wrapper = [this, fn](const std::string &id, const std::string &req,
                              void * /*arg*/) { resolve(id, 0, fn(req)); };
    return bind(name, wrapper, nullptr);
  }

  noresult bind(const std::string &name, binding_t fn, void *arg) {
    if (bindings.count(name) > 0) {
      return error_info{WEBVIEW_ERROR_DUPLICATE};
    }
    bindings.emplace(name, binding_info_t{binding_ctx_t(fn, arg), false, false});
    replace_bind_script();
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
    bindings.emplace(name, binding_info_t{binding_ctx_t(fn, arg), true, false});
    replace_bind_script();
    eval("if (window.__webview__) {\n\
window.__webview__.onBindGlobal(" +
         json_escape(name) + ")\n\
}");

    JSValue *global_obj = JS_PushGCRef(m_qjs_ctx, &m_qjs_global_ref);
    *global_obj = JS_GetGlobalObject(m_qjs_ctx);
    JS_SetPropertyStr(m_qjs_ctx, *global_obj, name.c_str(),
        JS_NewCFunction(m_qjs_ctx, [](JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) -> JSValue {
            auto engine = (engine_base*)JS_GetContextOpaque(ctx);
            const char* name = JS_GetCFunctionName(ctx, this_val);
            auto it = engine->bindings.find(name);
            if (it != engine->bindings.end()) {
                std::string req = "[";
                for (int i = 0; i < argc; i++) {
                    if (i > 0) req += ",";
                    const char *str = JS_ToCString(ctx, argv[i]);
                    req += json_escape(str ? str : "");
                }
                req += "]";
                it->second.ctx.call("mqjs-" + std::to_string(rand()), req);
            }
            return JS_UNDEFINED;
        }, name.c_str(), 1));
    JS_PopGCRef(m_qjs_ctx, &m_qjs_global_ref);
    return {};
  }

  noresult bind_global_secure(const std::string &name, binding_t fn, void *arg) {
    if (bindings.count(name) > 0) {
      return error_info{WEBVIEW_ERROR_DUPLICATE};
    }
    bindings.emplace(name, binding_info_t{binding_ctx_t(fn, arg), true, true});
    replace_bind_script();
    return {};
  }

  noresult unbind(const std::string &name) {
    auto found = bindings.find(name);
    if (found == bindings.end()) {
      return error_info{WEBVIEW_ERROR_NOT_FOUND};
    }
    bindings.erase(found);
    replace_bind_script();
    eval("if (window.__webview__) {\n\
window.__webview__.onUnbind(" +
         json_escape(name) + ")\n\
}");
    return {};
  }

  noresult resolve(const std::string &id, int status,
                   const std::string &result) {
    return dispatch([id, status, this, result] {
          std::string js = "window.__webview__.onReply(" + json_escape(id) +
                           ", " + std::to_string(status) + ", " +
                           (result.empty() ? "undefined" : json_escape(result)) + ")";
          eval(js);
        });
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

  void add_init_scripts(const std::string &post_fn) {
    add_user_script(create_alloy_script());
    add_user_script(create_init_script(post_fn));
    m_is_init_script_added = true;

    bind_global_secure("__alloy_spawn_sync", [this](const std::string &req) -> std::string {
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
      auto timeout_str = json_parse(opts_json, "timeout", 0);
      if (!timeout_str.empty()) opts.timeout = std::stoi(timeout_str);

      std::string stdout_data, stderr_data;
      std::mutex sync_mutex;
      std::condition_variable sync_cv;
      int exit_code = -1;
      bool finished = false;
      std::string signal_name = "";
      resource_usage usage{};

      subprocess proc(opts);
      proc.spawn(
          {
          [&](const std::string &data, bool is_stderr) {
            std::lock_guard<std::mutex> lock(sync_mutex);
            if (is_stderr) stderr_data += data;
            else stdout_data += data;
          },
          [&](int code, const std::string& sig) {
            std::lock_guard<std::mutex> lock(sync_mutex);
            exit_code = code;
            signal_name = sig;
            finished = true;
            sync_cv.notify_one();
          }
          });

      std::unique_lock<std::mutex> lock(sync_mutex);
      sync_cv.wait(lock, [&]{ return finished; });
      usage = proc.get_resource_usage();

      auto format_usage = [](const resource_usage& ru) {
          return "{\"maxRSS\":" + std::to_string(ru.max_rss) +
                 ",\"cpuTime\":{\"user\":" + std::to_string(ru.cpu_time.user) +
                 ",\"system\":" + std::to_string(ru.cpu_time.system) +
                 ",\"total\":" + std::to_string(ru.cpu_time.total) + "}}";
      };

      return "{\"stdout\":" + json_escape(stdout_data) +
             ",\"stderr\":" + json_escape(stderr_data) +
             ",\"exitCode\":" + std::to_string(exit_code) +
             ",\"signalCode\":" + json_escape(signal_name) +
             ",\"success\":" + (exit_code == 0 ? "true" : "false") +
             ",\"resourceUsage\":" + format_usage(usage) + "}";
    });

    bind_global_secure("__alloy_terminal_resize", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto cols = std::stoi(json_parse(req, "", 1));
      auto rows = std::stoi(json_parse(req, "", 2));
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->resize_terminal(cols, rows);
        return "true";
      }
      return "false";
    });

    bind_global_secure("__alloy_terminal_raw", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto enabled = json_parse(req, "", 1) == "true";
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->set_raw_mode(enabled);
        return "true";
      }
      return "false";
    });

    bind_global_secure("__alloy_spawn_bridge", [this](const std::string &req) -> std::string {
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
      auto timeout_str = json_parse(opts_json, "timeout", 0);
      if (!timeout_str.empty() && timeout_str != "undefined" && timeout_str != "null")
          opts.timeout = std::stoi(timeout_str);
      auto kill_sig_str = json_parse(opts_json, "killSignal", 0);
      if (!kill_sig_str.empty() && kill_sig_str != "undefined" && kill_sig_str != "null") {
          if (kill_sig_str[0] == '"') {
              std::string sig_name = json_parse(opts_json, "killSignal", 0);
              if (sig_name == "SIGKILL") opts.kill_signal = 9;
              else opts.kill_signal = 15;
          } else opts.kill_signal = std::stoi(kill_sig_str);
      }

      auto term_json = json_parse(opts_json, "terminal", 0);
      if (!term_json.empty() && term_json != "undefined" &&
          term_json != "null") {
        opts.use_terminal = true;
        auto cols = json_parse(term_json, "cols", 0);
        auto rows = json_parse(term_json, "rows", 0);
        if (!cols.empty() && cols != "undefined" && cols != "null")
          opts.terminal.cols = std::stoi(cols);
        if (!rows.empty() && rows != "undefined" && rows != "null")
          opts.terminal.rows = std::stoi(rows);
      }

      auto env_json = json_parse(opts_json, "env", 0);
      if (!env_json.empty() && env_json[0] == '{') {
          size_t pos = 1;
          while (pos < env_json.size() - 1) {
              auto key = json_parse(env_json.substr(pos), "", 1);
              auto val = json_parse(env_json.substr(pos), key, 0);
              if (!key.empty()) opts.env[key] = val;
              pos = env_json.find(',', pos);
              if (pos == std::string::npos) break;
              pos++;
          }
      }

      auto proc = std::make_shared<subprocess>(opts);
      auto proc_id = std::to_string(reinterpret_cast<uintptr_t>(proc.get()));
      m_subprocesses[proc_id] = proc;

      bool success = proc->spawn(
          {
          [this, proc_id](const std::string &data, bool is_stderr) {
            std::string js = "window.Alloy.__onData(" + json_escape(proc_id) +
                             ", " + json_escape(data) + ", " +
                             (is_stderr ? "true" : "false") + ")";
            dispatch([this, js] { eval(js); });
          },
          [this, proc_id](int exit_code, const std::string& signal_name) {
            std::string js = "window.Alloy.__onExit(" + json_escape(proc_id) +
                             ", " + std::to_string(exit_code) + ", " + json_escape(signal_name) + ")";
            dispatch([this, js] { eval(js); });
          }
          });

      if (success) {
        return "{\"id\":" + json_escape(proc_id) +
               ",\"pid\":" + std::to_string(proc->get_pid()) + "}";
      } else {
        return "{\"error\":\"Failed to spawn\"}";
      }
    });

    bind_global_secure("__alloy_kill", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto sig_str = json_parse(req, "", 1);
      int sig = 15;
      if (!sig_str.empty()) {
          if (sig_str[0] == '"') {
              std::string sig_name = json_parse(req, "", 1);
              if (sig_name == "SIGKILL") sig = 9;
              else sig = 15;
          } else sig = std::stoi(sig_str);
      }
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->kill(sig);
        return "true";
      }
      return "false";
    });

    bind_global_secure("__alloy_stdin_write", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto data = json_parse(req, "", 1);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->write_stdin(data);
        return "true";
      }
      return "false";
    });

    bind_global_secure("__alloy_stdin_close", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->close_stdin();
        return "true";
      }
      return "false";
    });

    bind_global_secure("__alloy_cron_register", [this](const std::string &req) -> std::string {
      auto path = json_parse(req, "", 0);
      auto schedule = json_parse(req, "", 1);
      auto title = json_parse(req, "", 2);
      return cron_manager::register_job(path, schedule, title) ? "true" : "false";
    });

    bind_global_secure("__alloy_sqlite_open", [this](const std::string &req) -> std::string {
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

    bind_global_secure("__alloy_sqlite_query", [this](const std::string &req) -> std::string {
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

    bind_global_secure("__alloy_sqlite_step", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto safe_int = json_parse(req, "", 1) == "true";
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        return it->second->step(safe_int);
      }
      return "";
    });

    bind_global_secure("__alloy_sqlite_close", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      m_sqlite_dbs.erase(db_id);
      return "true";
    });

    bind_global_secure("__alloy_get_resource_usage", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
          auto ru = it->second->get_resource_usage();
          return "{\"maxRSS\":" + std::to_string(ru.max_rss) +
                 ",\"cpuTime\":{\"user\":" + std::to_string(ru.cpu_time.user) +
                 ",\"system\":" + std::to_string(ru.cpu_time.system) +
                 ",\"total\":" + std::to_string(ru.cpu_time.total) + "}}";
      }
      return "null";
    });

    bind_global("eval", [this](const std::string &req) -> std::string {
      auto secret = json_parse(req, "", 0);
      if (secret != m_ipc_secret) return "{\"error\":\"Unauthorized IPC access\"}";
      auto js = json_parse(req, "", 1);
      return this->secure_eval_internal(js);
    });

    bind_global_secure("__alloy_file_exists", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0) ? "true" : "false";
    });

    bind_global_secure("__alloy_file_read", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return "";
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    });

    bind_global_secure("__alloy_file_write", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        auto data = json_parse(req, "", 1);
        std::ofstream f(path, std::ios::binary);
        if (!f.is_open()) return "0";
        f.write(data.c_str(), data.size());
        f.close();
        return std::to_string(data.size());
    });

    bind_global_secure("__alloy_get_env", [this](const std::string &req) -> std::string {
        auto key = json_parse(req, "", 0);
        const char* val = getenv(key.c_str());
        return val ? val : "";
    });

    bind_global_secure("__alloy_set_env", [this](const std::string &req) -> std::string {
        auto key = json_parse(req, "", 0);
        auto val = json_parse(req, "", 1);
#ifdef _WIN32
        _putenv_s(key.c_str(), val.c_str());
#else
        setenv(key.c_str(), val.c_str(), 1);
#endif
        return "true";
    });

    bind_global("__alloy_ipc_receive", [this](const std::string &req) -> std::string {
        auto payload = json_parse(req, "", 0);
        auto signature = json_parse(req, "", 1);

        if (signature != sign_payload(payload)) {
            return "{\"error\":\"Invalid IPC signature\"}";
        }

        auto method = json_parse(payload, "method", 0);
        auto params = json_parse(payload, "params", 0);

        if (method == "browser.ipc_message") {
            std::string js = "Alloy.__onMessage(" + params + ")";
            this->secure_eval_internal(js);
        }
        return "";
    });

    bind_global_secure("__alloy_send", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto message = json_parse(req, "", 1);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->write_stdin(message + "\n");
        return "true";
      }
      return "false";
    });

    bind_global_secure("__alloy_cleanup", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      m_subprocesses.erase(proc_id);
      return "true";
    });
  }

  std::string sign_payload(const std::string& payload) {
      return std::to_string(payload.length() ^ 42);
  }

  virtual void on_message(const std::string &msg) {
    auto id = json_parse(msg, "id", 0);
    auto name = json_parse(msg, "method", 0);
    auto args = json_parse(msg, "params", 0);

    auto found = bindings.find(name);
    if (found == bindings.end()) {
      return;
    }

    if (found->second.is_secure) {
        auto secret = json_parse(msg, "secret", 0);
        if (secret != m_ipc_secret) {
            resolve(id, 1, "{\"error\":\"Unauthorized secure binding access\"}");
            return;
        }
    }

    const auto &context = found->second.ctx;
    dispatch([=] { context.call(id, args); });
  }

  std::string create_alloy_script() {
    return R"js(
(function() {
  'use strict';
  if (window.Alloy) return;

  class Subprocess {
    constructor(id, pid, options) {
      this.id = id;
      this.pid = pid;
      this.exitCode = null;
      this.signalCode = null;
      this.killed = false;
      this._exitedPromise = new Promise(resolve => {
        this._resolveExited = resolve;
      });

      this._stdoutController = null;
      this.stdout = (options && options.stdout === "ignore") ? null : new ReadableStream({
        start: (controller) => { this._stdoutController = controller; }
      });

      this._stderrController = null;
      this.stderr = (options && options.stderr === "ignore") ? null : new ReadableStream({
        start: (controller) => { this._stderrController = controller; }
      });

      this.stdin = (options && options.stdin === "pipe") ? new FileSink(null, { id: this.id }) : null;

      if (options && options.terminal) {
          this.terminal = {
              write: (data) => window.__alloy_stdin_write(this.id, data),
              resize: (cols, rows) => window.__alloy_terminal_resize(this.id, cols, rows),
              setRawMode: (enabled) => window.__alloy_terminal_raw(this.id, enabled),
              close: () => window.__alloy_stdin_close(this.id),
              ref: () => {},
              unref: () => {}
          };
      }
    }
    get exited() { return this._exitedPromise; }
    kill(sig) {
      this.killed = true;
      return window.__alloy_kill(this.id, sig);
    }
    ref() {}
    unref() {}
    send(message) {
        window.__alloy_send(this.id, JSON.stringify(message));
    }
    disconnect() {
        window.__alloy_stdin_close(this.id);
    }
    resourceUsage() {
        return JSON.parse(window.__alloy_get_resource_usage(this.id));
    }
    async [Symbol.asyncDispose]() {
        this.kill();
    }
  }

  class AlloyFile {
    constructor(path) { this.path = path; }
    toString() { return this.path; }
    async text() { return window.__alloy_file_read(this.path); }
    async size() { return parseInt(window.__alloy_file_size(this.path)); }
  }

  class FileSink {
    constructor(path, options) {
        this.path = path;
        this.id = options ? options.id : null;
    }
    write(data) {
        if (this.id) return window.__alloy_stdin_write(this.id, data);
        return window.__alloy_file_write(this.path, data);
    }
    flush() {}
    end() {
        if (this.id) window.__alloy_stdin_close(this.id);
    }
  }

  const subprocesses = {};

  window.__alloy_ipc_receive = function(payload, signature) {
      if (signature !== String(payload.length ^ 42)) return; // Placeholder for real HMAC
      const data = JSON.parse(payload);
      if (data.method.startsWith('browser.')) {
          const api = data.method.split('.')[1];
          if (typeof window[api] === 'function') {
              window[api](...data.params);
          }
      }
  };
  Object.defineProperty(window, "__alloy_ipc_receive", { writable: false, configurable: false });

  const Alloy = {
    spawn: function(cmd, opts) {
      const arg = (Array.isArray(cmd)) ? cmd : (cmd && cmd.cmd ? cmd : [cmd]);
      const options = (Array.isArray(cmd)) ? opts : cmd;
      const res = JSON.parse(window.__alloy_spawn_bridge(arg, options));
      if (res.error) throw new Error(res.error);
      const proc = new Subprocess(res.id, res.pid, options);
      subprocesses[res.id] = proc;
      if (options && options.onExit) proc._onExit = options.onExit;
      return proc;
    },
    spawnSync: function(cmd, opts) {
        const arg = (Array.isArray(cmd)) ? cmd : (cmd && cmd.cmd ? cmd : [cmd]);
        const options = (Array.isArray(cmd)) ? opts : cmd;
        const res = JSON.parse(window.__alloy_spawn_sync(arg, options));
        if (res.stdout) res.stdout = Buffer.from(res.stdout);
        if (res.stderr) res.stderr = Buffer.from(res.stderr);
        return res;
    },
    Terminal: class {
        constructor(options) {
            this.options = options;
            this.closed = false;
        }
        write(data) {}
        resize(cols, rows) {}
        setRawMode(enabled) {}
        ref() {}
        unref() {}
        close() { this.closed = true; }
        async [Symbol.asyncDispose]() { this.close(); }
    },
    file: function(path) { return new AlloyFile(path); },
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
    __onExit: function(id, exitCode, signalCode) {
      const proc = subprocesses[id];
      if (proc) {
        proc.exitCode = exitCode;
        proc.signalCode = signalCode;
        if (proc._stdoutController) proc._stdoutController.close();
        if (proc._stderrController) proc._stderrController.close();
        if (proc._onExit) proc._onExit(proc, exitCode, signalCode);
        proc._resolveExited(exitCode);
        delete subprocesses[id];
        window.__alloy_cleanup(id);
      }
    },
    __onMessage: function(message) {
        // Implementation for receiving IPC from child processes
    }
  };
  window.Alloy = Alloy;
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
      var _payload = {\n\
        id: _id,\n\
        method: method,\n\
        params: _params\n\
      };\n\
      if (arguments[0] && typeof arguments[0] === 'string' && arguments[0].startsWith('__alloy_secret:')) {\n\
          _payload.secret = arguments[0].split(':')[1];\n\
          _payload.method = arguments[1];\n\
          _payload.params = Array.prototype.slice.call(arguments, 2);\n\
      }\n\
      var promise = new Promise(function(resolve, reject) {\n\
        _promises[_id] = { resolve, reject };\n\
      });\n\
      this.post(JSON.stringify(_payload));\n\
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
    Webview_.prototype.onBind = function(name, warn, secret) {\n\
      window[name] = (function() {\n\
        var params = [name].concat(Array.prototype.slice.call(arguments));\n\
        if (secret) params.unshift('__alloy_secret:' + secret);\n\
        return Webview_.prototype.call.apply(this, params);\n\
      }).bind(this);\n\
    };\n\
    Webview_.prototype.onBindGlobal = function(name, warn, secret) {\n\
      Object.defineProperty(window, name, {\n\
        value: (function() {\n\
          var params = [name].concat(Array.prototype.slice.call(arguments));\n\
          if (secret) params.unshift('__alloy_secret:' + secret);\n\
          return Webview_.prototype.call.apply(this, params);\n\
        }).bind(this),\n\
        writable: false,\n\
        configurable: true\n\
      });\n\
    };\n\
    return Webview_;\n\
  })();\n\
  window.__webview__ = new Webview();\n\
})()";
    return js;
  }

  std::string create_bind_script() {
    std::string js = "(function() {\n  'use strict';\n";
    js += "  const __alloy_secret = " + json_escape(m_ipc_secret) + ";\n";
    std::string warn = m_warn_overwrite_on_bind ? "true" : "false";
    for (const auto &binding : bindings) {
        std::string secret = binding.second.is_secure ? "__alloy_secret" : "null";
        if (binding.second.is_global) {
            js += "  window.__webview__.onBindGlobal(" + json_escape(binding.first) + ", " + warn + ", " + secret + ");\n";
        } else {
            js += "  window.__webview__.onBind(" + json_escape(binding.first) + ", " + warn + ", " + secret + ");\n";
        }
    }
    js += "})()";
    return js;
  }

  virtual void on_window_created() { inc_window_count(); }

  virtual void on_message(const std::string &msg) {
    auto id = json_parse(msg, "id", 0);
    auto name = json_parse(msg, "method", 0);
    auto args = json_parse(msg, "params", 0);

    auto found = bindings.find(name);
    if (found == bindings.end()) {
      return;
    }

    if (found->second.is_secure) {
        auto secret = json_parse(msg, "secret", 0);
        if (secret != m_ipc_secret) {
            resolve(id, 1, "{\"error\":\"Unauthorized secure binding access\"}");
            return;
        }
    }

    const auto &context = found->second.ctx;
    dispatch([=] { context.call(id, args); });
  }

  virtual void on_window_destroyed(bool skip_termination = false) {
    if (dec_window_count() <= 0) {
      if (!skip_termination) {
        terminate();
      }
    }
  }

  void deplete_run_loop_event_queue() {
    bool done{};
    dispatch([&] { done = true; });
    run_event_loop_while([&] { return !done; });
  }

  virtual void run_event_loop_while(std::function<bool()> fn) = 0;

  bool m_warn_overwrite_on_bind = false;

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

  static JSValue js_alloy_log(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
      for (int i = 0; i < argc; i++) {
          const char *str = JS_ToCString(ctx, argv[i]);
          if (str) {
              printf("%s%s", str, i == argc - 1 ? "" : " ");
          }
      }
      printf("\n");
      return JS_UNDEFINED;
  }

  std::string secure_eval_internal(const std::string& js) {
      if (js.empty()) return "null";
      JSGCRef val_ref;
      JSValue *val = JS_AddGCRef(m_qjs_ctx, &val_ref);
      *val = JS_Eval(m_qjs_ctx, js.c_str(), js.size(), "<eval>", JS_EVAL_TYPE_GLOBAL);
      std::string result;
      if (JS_IsException(*val)) {
          JSValue exception = JS_GetException(m_qjs_ctx);
          const char *str = JS_ToCString(m_qjs_ctx, exception);
          result = "{\"error\":" + json_escape(str) + "}";
      } else {
          const char *str = JS_ToCString(m_qjs_ctx, *val);
          result = "{\"result\":" + json_escape(str ? str : "undefined") + "}";
      }
      JS_DeleteGCRef(m_qjs_ctx, &val_ref);
      return result;
  }

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

  std::map<std::string, binding_info_t> bindings;
  std::map<std::string, std::shared_ptr<subprocess>> m_subprocesses;
  std::map<std::string, std::shared_ptr<sqlite_db>> m_sqlite_dbs;
  std::map<std::string, std::shared_ptr<sqlite_stmt>> m_sqlite_stmts;
  uint8_t *m_qjs_mem = nullptr;
  JSContext *m_qjs_ctx = nullptr;
  JSGCRef m_qjs_global_ref, m_qjs_alloy_ref;

  user_script *m_bind_script{};
  std::list<user_script> m_user_scripts;

  std::string m_ipc_secret;
  bool m_is_init_script_added{};
  bool m_is_size_set{};
  bool m_owns_window{};
  static const int m_initial_width = 640;
  static const int m_initial_height = 480;
};

} // namespace alloy::engine

#endif // ALLOY_ENGINE_ENGINE_BASE_HH
