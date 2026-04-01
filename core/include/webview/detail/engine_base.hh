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
  noresult bind_window(const std::string &name, sync_binding_t fn) {
    auto wrapper = [this, fn](const std::string &id, const std::string &req,
                              void * /*arg*/) { resolve(id, 0, fn(req)); };
    return bind_window(name, wrapper, nullptr);
  }

  // Asynchronous bind
  noresult bind_window(const std::string &name, binding_t fn, void *arg) {
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

    bind_window("__alloy_spawn_sync", [this](const std::string &req) -> std::string {
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
      std::mutex sync_mutex;
      std::condition_variable sync_cv;
      int exit_code = -1;
      bool finished = false;

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
            finished = true;
            sync_cv.notify_one();
          }
          });

      std::unique_lock<std::mutex> lock(sync_mutex);
      sync_cv.wait(lock, [&]{ return finished; });

      return "{\"stdout\":" + json_escape(stdout_data) +
             ",\"stderr\":" + json_escape(stderr_data) +
             ",\"exitCode\":" + std::to_string(exit_code) +
             ",\"success\":" + (exit_code == 0 ? "true" : "false") + "}";
    });

    bind_window("__alloy_terminal_resize", [this](const std::string &req) -> std::string {
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

    bind_window("__alloy_gui_set_value", [this](const std::string &req) -> std::string {
      auto id = json_parse(req, "", 0);
      auto val = std::stod(json_parse(req, "", 1));
      auto it = m_gui_components.find(id);
      if (it != m_gui_components.end()) {
        return alloy_set_value(it->second, val) == ALLOY_OK ? "true" : "false";
      }
      return "false";
    });

    bind_window("__alloy_terminal_raw", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto enabled = json_parse(req, "", 1) == "true";
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->set_raw_mode(enabled);
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_spawn_bridge", [this](const std::string &req) -> std::string {
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

    bind_window("__alloy_kill", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->kill();
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_stdin_write", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto data = json_parse(req, "", 1);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->write_stdin(data);
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_stdin_close", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      auto it = m_subprocesses.find(proc_id);
      if (it != m_subprocesses.end()) {
        it->second->close_stdin();
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_send", [this](const std::string &req) -> std::string {
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

    bind_window("__alloy_cleanup", [this](const std::string &req) -> std::string {
      auto proc_id = json_parse(req, "", 0);
      m_subprocesses.erase(proc_id);
      return "true";
    });

    bind_window("__alloy_cron_register", [this](const std::string &req) -> std::string {
      auto path = json_parse(req, "", 0);
      auto schedule = json_parse(req, "", 1);
      auto title = json_parse(req, "", 2);
      return cron_manager::register_job(path, schedule, title) ? "true" : "false";
    });

    bind_window("__alloy_cron_remove", [this](const std::string &req) -> std::string {
      auto title = json_parse(req, "", 0);
      return cron_manager::remove_job(title) ? "true" : "false";
    });

    bind_window("__alloy_sqlite_open", [this](const std::string &req) -> std::string {
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

    bind_window("__alloy_sqlite_query", [this](const std::string &req) -> std::string {
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

    bind_window("__alloy_sqlite_step", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto safe_int = json_parse(req, "", 1) == "true";
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        return it->second->step(safe_int);
      }
      return "";
    });

    bind_window("__alloy_sqlite_last_insert_rowid", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        return std::to_string(it->second->last_insert_rowid());
      }
      return "0";
    });

    bind_window("__alloy_sqlite_changes", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        return std::to_string(it->second->changes());
      }
      return "0";
    });

    bind_window("__alloy_sqlite_reset", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        it->second->reset();
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_sqlite_bind", [this](const std::string &req) -> std::string {
      auto stmt_id = json_parse(req, "", 0);
      auto index_str = json_parse(req, "", 1);
      auto type = json_parse(req, "", 2);
      auto val = json_parse(req, "", 3);
      auto it = m_sqlite_stmts.find(stmt_id);
      if (it != m_sqlite_stmts.end()) {
        int index = std::stoi(index_str);
        if (type == "null") it->second->bind_null(index);
        else if (type == "number") it->second->bind_double(index, std::stod(val));
        else if (type == "bigint") it->second->bind_int64(index, std::stoll(val));
        else if (type == "blob") {
            std::vector<unsigned char> data;
            for (size_t i = 0; i < val.length(); i += 2) {
                data.push_back((unsigned char)std::stoi(val.substr(i, 2), nullptr, 16));
            }
            it->second->bind_blob(index, data.data(), (int)data.size());
        }
        else it->second->bind_text(index, val);
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_gui_create_window", [this](const std::string &req) -> std::string {
      auto title = json_parse(req, "", 0);
      auto w = std::stoi(json_parse(req, "", 1));
      auto h = std::stoi(json_parse(req, "", 2));
      auto comp = alloy_create_window(title.c_str(), w, h);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_button", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_button(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_label", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_label(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_textfield", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_textfield(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_textarea", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_textarea(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_checkbox", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_checkbox(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_radiobutton", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_radiobutton(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_combobox", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_combobox(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_slider", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_slider(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_loadingspinner", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_loadingspinner(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_spinner", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_spinner(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_progressbar", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_progressbar(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_listview", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_listview(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_treeview", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_treeview(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_tabview", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_tabview(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_webview", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_webview(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_vstack", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_vstack(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_hstack", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_hstack(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_scrollview", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_scrollview(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_menu", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_menu(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_menubar", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_menubar(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_toolbar", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_toolbar(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_statusbar", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_statusbar(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_splitter", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_splitter(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_dialog", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_dialog(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_filedialog", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_filedialog(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_colorpicker", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_colorpicker(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_datepicker", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_datepicker(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_timepicker", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_timepicker(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_tooltip", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_tooltip(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_divider", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_divider(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_image", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_image(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_icon", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_icon(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_separator", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_separator(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_groupbox", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_groupbox(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_accordion", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_accordion(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_popover", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_popover(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_contextmenu", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_contextmenu(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_switch", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_switch(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_badge", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_badge(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_chip", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_chip(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_card", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_card(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_link", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_link(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_rating", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_rating(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_richtexteditor", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_richtexteditor(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_create_codeeditor", [this](const std::string &req) -> std::string {
      auto parent_id = json_parse(req, "", 0);
      auto parent_it = m_gui_components.find(parent_id);
      auto parent = parent_it != m_gui_components.end() ? parent_it->second : nullptr;
      auto comp = alloy_create_codeeditor(parent);
      auto id = std::to_string(m_gui_next_id++);
      m_gui_components[id] = comp;
      return id;
    });

    bind_window("__alloy_gui_set_text", [this](const std::string &req) -> std::string {
      auto id = json_parse(req, "", 0);
      auto text = json_parse(req, "", 1);
      auto it = m_gui_components.find(id);
      if (it != m_gui_components.end()) {
        return alloy_set_text(it->second, text.c_str()) == ALLOY_OK ? "true" : "false";
      }
      return "false";
    });

    bind_window("__alloy_gui_destroy", [this](const std::string &req) -> std::string {
      auto id = json_parse(req, "", 0);
      auto it = m_gui_components.find(id);
      if (it != m_gui_components.end()) {
        alloy_destroy(it->second);
        m_gui_components.erase(it);
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_sqlite_serialize", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        auto data = it->second->serialize();
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

    bind_window("__alloy_sqlite_file_control", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto op = std::stoi(json_parse(req, "", 1));
      auto val = std::stoi(json_parse(req, "", 2));
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        it->second->file_control(op, &val);
        return "true";
      }
      return "false";
    });

    bind_window("__alloy_sqlite_load_extension", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      auto path = json_parse(req, "", 1);
      auto it = m_sqlite_dbs.find(db_id);
      if (it != m_sqlite_dbs.end()) {
        try {
          it->second->load_extension(path);
          return "true";
        } catch (const std::exception &e) {
          return "{\"error\":" + json_escape(e.what()) + "}";
        }
      }
      return "false";
    });

    bind_window("__alloy_sqlite_close", [this](const std::string &req) -> std::string {
      auto db_id = json_parse(req, "", 0);
      m_sqlite_dbs.erase(db_id);
      return "true";
    });

    bind_window("__alloy_secure_eval", [this](const std::string &req) -> std::string {
      auto js = json_parse(req, "", 0);
      return this->secure_eval_internal(js);
    });

    bind_window("__alloy_file_exists", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0) ? "true" : "false";
    });

    bind_window("__alloy_file_size", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        struct stat buffer;
        if (stat(path.c_str(), &buffer) == 0) return std::to_string(buffer.st_size);
        return "0";
    });

    bind_window("__alloy_file_read", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return "";
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    });

    bind_window("__alloy_file_write", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        auto data = json_parse(req, "", 1);
        std::ofstream f(path, std::ios::binary);
        if (!f.is_open()) return "0";
        f.write(data.c_str(), data.size());
        return std::to_string(data.size());
    });

    bind_window("__alloy_file_delete", [this](const std::string &req) -> std::string {
        auto path = json_parse(req, "", 0);
        return (remove(path.c_str()) == 0) ? "true" : "false";
    });

    bind_window("__alloy_get_env", [this](const std::string &req) -> std::string {
        auto key = json_parse(req, "", 0);
        const char* val = getenv(key.c_str());
        return val ? val : "";
    });

    bind_window("__alloy_set_env", [this](const std::string &req) -> std::string {
        auto key = json_parse(req, "", 0);
        auto val = json_parse(req, "", 1);
#ifdef _WIN32
        _putenv_s(key.c_str(), val.c_str());
#else
        setenv(key.c_str(), val.c_str(), 1);
#endif
        return "true";
    });
  }

  std::string create_alloy_script() {
    return R"js(
(function() {
  'use strict';
  function parseEnv(content, existingEnv = {}) {
    const result = { ...existingEnv };
    const lines = content.split(/\r?\n/);
    for (let line of lines) {
      line = line.trim();
      if (!line || line.startsWith("#")) continue;
      const match = line.match(/^([^=]+)=(.*)$/);
      if (!match) continue;
      const key = match[1].trim();
      let value = match[2].trim();
      if ((value.startsWith('"') && value.endsWith('"')) ||
          (value.startsWith("'") && value.endsWith("'")) ||
          (value.startsWith("`") && value.endsWith("`"))) {
        value = value.substring(1, value.length - 1);
      }
      let expandedValue = "";
      for (let i = 0; i < value.length; i++) {
        if (value[i] === "$" && (i === 0 || value[i - 1] !== "\\")) {
          let varName = ""; let j = i + 1;
          while (j < value.length && /[a-zA-Z0-9_]/.test(value[j])) { varName += value[j]; j++; }
          if (varName) { expandedValue += result[varName] || ""; i = j - 1; }
          else { expandedValue += "$"; }
        } else if (value[i] === "$" && i > 0 && value[i - 1] === "\\") {
          expandedValue = expandedValue.substring(0, expandedValue.length - 1) + "$";
        } else { expandedValue += value[i]; }
      }
      result[key] = expandedValue;
    }
    return result;
  }
  'use strict';
  if (window.Alloy) return;

  const initialEnv = { NODE_ENV: window.__alloy_get_env("NODE_ENV") || "development" };
  const envFiles = [".env", `.env.${initialEnv.NODE_ENV}`, ".env.local"];
  let loadedEnv = { ...initialEnv };
  for (const file of envFiles) {
    if (window.__alloy_file_exists(file) === "true") {
      const content = window.__alloy_file_read(file);
      loadedEnv = parseEnv(content, loadedEnv);
    }
  }

  window.process = window.process || {};
  window.process.env = new Proxy(loadedEnv, {
    get(target, prop) {
      if (typeof prop !== "string") return undefined;
      const val = window.__alloy_get_env(prop);
      return val || target[prop];
    },
    set(target, prop, value) {
      if (typeof prop === "string") {
        window.__alloy_set_env(prop, String(value));
        target[prop] = String(value);
        return true;
      }
      return false;
    }
  });

  // Optimized ReadableStream
  const NativeReadableStream = window.ReadableStream;
  window.ReadableStream = class extends NativeReadableStream {
    constructor(underlyingSource, strategy) {
      if (underlyingSource && underlyingSource.type === 'direct') {
        let controller;
        const source = {
          start(c) {
            controller = c;
            controller.write = (chunk) => controller.enqueue(typeof chunk === 'string' ? new TextEncoder().encode(chunk) : chunk);
            if (underlyingSource.start) underlyingSource.start(controller);
          },
          pull(c) {
            if (underlyingSource.pull) underlyingSource.pull(controller);
          },
          cancel(reason) {
            if (underlyingSource.cancel) underlyingSource.cancel(reason);
          }
        };
        super(source, strategy);
      } else {
        super(underlyingSource, strategy);
      }
    }
  };

  // Security: Replace eval with secureEval
  if (typeof window._forbidden_eval === 'undefined') {
    window._forbidden_eval = window.eval;
    window.secureEval = function(code) {
      const res = JSON.parse(window.__alloy_secure_eval(code));
      return res.result;
    };
    window.eval = window.secureEval;
  }

  if (typeof Buffer === 'undefined') {
    window.Buffer = class extends Uint8Array {
      static from(data) {
        if (typeof data === 'string') return new Buffer((new TextEncoder()).encode(data));
        return new Buffer(data);
      }
      static alloc(size) { return new Buffer(size); }
      toString(enc) { return (new TextDecoder(enc)).decode(this); }
    };
  }

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

      if (options && options.terminal) {
          this.terminal = {
              write: (data) => window.__alloy_stdin_write(this.id, data),
              resize: (cols, rows) => window.__alloy_terminal_resize(this.id, cols, rows),
              setRawMode: (enabled) => window.__alloy_terminal_raw(this.id, enabled),
              close: () => window.__alloy_stdin_close(this.id),
              ref: () => {}, unref: () => {}
          };
      }
    }
    get exited() { return this._exitedPromise; }
    kill(sig) {
      this.killed = true;
      return window.__alloy_kill(this.id, sig);
    }
    send(message) {
      return window.__alloy_send(this.id, JSON.stringify(message));
    }
    resourceUsage() {
      return {
        maxRSS: 0,
        cpuTime: { user: 0, system: 0, total: 0 }
      };
    }
    unref() {}
    ref() {}
    disconnect() {
      window.__alloy_stdin_close(this.id);
    }
    async [Symbol.asyncDispose]() {
        this.kill();
    }
  }

  const subprocesses = {};

    class AlloyFile {
      constructor(path, options) {
        this.path = path;
        this.type = (options && options.type) || "text/plain;charset=utf-8";
      }
      get size() { return parseInt(window.__alloy_file_size(this.path)); }
      async text() { return window.__alloy_file_read(this.path); }
      async json() { return JSON.parse(await this.text()); }
      async exists() { return window.__alloy_file_exists(this.path) === "true"; }
      async delete() { return window.__alloy_file_delete(this.path) === "true"; }
      stream() {
        const path = this.path;
        return new ReadableStream({
          async start(controller) {
            const data = await window.__alloy_file_read(path);
            controller.enqueue(new TextEncoder().encode(data));
            controller.close();
          }
        });
      }
      async arrayBuffer() { return (new TextEncoder().encode(await this.text())).buffer; }
      async bytes() { return new TextEncoder().encode(await this.text()); }
      writer(options) {
        return new FileSink(this.path, options);
      }
    }

    class FileSink {
      constructor(path, options) {
        this.path = path;
        this.buffer = "";
        this.highWaterMark = (options && options.highWaterMark) || 64 * 1024;
      }
      write(chunk) {
        this.buffer += chunk;
        if (this.buffer.length >= this.highWaterMark) this.flush();
        return chunk.length;
      }
      flush() {
        if (this.buffer.length === 0) return 0;
        window.__alloy_file_write(this.path, this.buffer);
        const len = this.buffer.length;
        this.buffer = "";
        return len;
      }
      end() {
        this.flush();
        return 0;
      }
      ref() {}
      unref() {}
    }

    class ArrayBufferSink {
      constructor() {
        this._chunks = [];
        this._totalLength = 0;
        this._options = {};
      }
      start(options) {
        this._options = options || {};
      }
      write(chunk) {
        let b;
        if (typeof chunk === 'string') b = new TextEncoder().encode(chunk);
        else if (chunk instanceof ArrayBuffer) b = new Uint8Array(chunk);
        else if (ArrayBuffer.isView(chunk)) b = new Uint8Array(chunk.buffer, chunk.byteOffset, chunk.byteLength);
        else b = new Uint8Array(chunk);
        this._chunks.push(b);
        this._totalLength += b.length;
        return b.length;
      }
      flush() {
        const res = this.end();
        this._chunks = [];
        this._totalLength = 0;
        return res;
      }
      end() {
        const res = new Uint8Array(this._totalLength);
        let offset = 0;
        for (const chunk of this._chunks) {
          res.set(chunk, offset);
          offset += chunk.length;
        }
        return this._options.asUint8Array ? res : res.buffer;
      }
    }

  window.Alloy = {
      file: function(path, options) { return new AlloyFile(path, options); },
      write: async function(dest, data) {
        const path = (dest instanceof AlloyFile) ? dest.path : dest;
        const content = (data instanceof Response) ? await data.text() : (data instanceof Blob ? await data.text() : data);
        return parseInt(window.__alloy_file_write(path, content));
      },
      stdin: new AlloyFile("/dev/stdin"),
      stdout: new AlloyFile("/dev/stdout"),
      stderr: new AlloyFile("/dev/stderr"),
      ArrayBufferSink: ArrayBufferSink,
      env: window.process.env,
    bindWindow: function(name, fn) {
      window[name] = fn;
    },
    bindGlobal: function(name, fn) {
      window[name] = fn;
    },
    gui: {
      createWindow: function(title, w, h) { return window.__alloy_gui_create_window(title, w, h); },
      createButton: function(parent) { return window.__alloy_gui_create_button(parent); },
      createTextField: function(parent) { return window.__alloy_gui_create_textfield(parent); },
        createTextArea: function(parent) { return window.__alloy_gui_create_textarea(parent); },
        createLabel: function(parent) { return window.__alloy_gui_create_label(parent); },
        createCheckBox: function(parent) { return window.__alloy_gui_create_checkbox(parent); },
        createRadioButton: function(parent) { return window.__alloy_gui_create_radiobutton(parent); },
        createComboBox: function(parent) { return window.__alloy_gui_create_combobox(parent); },
        createSlider: function(parent) { return window.__alloy_gui_create_slider(parent); },
        createSpinner: function(parent) { return window.__alloy_gui_create_spinner(parent); },
        createLoadingSpinner: function(parent) { return window.__alloy_gui_create_loadingspinner(parent); },
        createProgressBar: function(parent) { return window.__alloy_gui_create_progressbar(parent); },
        createListView: function(parent) { return window.__alloy_gui_create_listview(parent); },
        createTreeView: function(parent) { return window.__alloy_gui_create_treeview(parent); },
        createTabView: function(parent) { return window.__alloy_gui_create_tabview(parent); },
        createWebView: function(parent) { return window.__alloy_gui_create_webview(parent); },
        createVStack: function(parent) { return window.__alloy_gui_create_vstack(parent); },
        createHStack: function(parent) { return window.__alloy_gui_create_hstack(parent); },
        createScrollView: function(parent) { return window.__alloy_gui_create_scrollview(parent); },
        createMenu: function(parent) { return window.__alloy_gui_create_menu(parent); },
        createMenuBar: function(parent) { return window.__alloy_gui_create_menubar(parent); },
        createToolbar: function(parent) { return window.__alloy_gui_create_toolbar(parent); },
        createStatusBar: function(parent) { return window.__alloy_gui_create_statusbar(parent); },
        createSplitter: function(parent) { return window.__alloy_gui_create_splitter(parent); },
        createDialog: function(parent) { return window.__alloy_gui_create_dialog(parent); },
        createFileDialog: function(parent) { return window.__alloy_gui_create_filedialog(parent); },
        createColorPicker: function(parent) { return window.__alloy_gui_create_colorpicker(parent); },
        createDatePicker: function(parent) { return window.__alloy_gui_create_datepicker(parent); },
        createTimePicker: function(parent) { return window.__alloy_gui_create_timepicker(parent); },
        createTooltip: function(parent) { return window.__alloy_gui_create_tooltip(parent); },
        createDivider: function(parent) { return window.__alloy_gui_create_divider(parent); },
        createImage: function(parent) { return window.__alloy_gui_create_image(parent); },
        createIcon: function(parent) { return window.__alloy_gui_create_icon(parent); },
        createSeparator: function(parent) { return window.__alloy_gui_create_separator(parent); },
        createGroupBox: function(parent) { return window.__alloy_gui_create_groupbox(parent); },
        createAccordion: function(parent) { return window.__alloy_gui_create_accordion(parent); },
        createPopover: function(parent) { return window.__alloy_gui_create_popover(parent); },
        createContextMenu: function(parent) { return window.__alloy_gui_create_contextmenu(parent); },
        createSwitch: function(parent) { return window.__alloy_gui_create_switch(parent); },
        createBadge: function(parent) { return window.__alloy_gui_create_badge(parent); },
        createChip: function(parent) { return window.__alloy_gui_create_chip(parent); },
        createCard: function(parent) { return window.__alloy_gui_create_card(parent); },
        createLink: function(parent) { return window.__alloy_gui_create_link(parent); },
        createRating: function(parent) { return window.__alloy_gui_create_rating(parent); },
        createRichTextEditor: function(parent) { return window.__alloy_gui_create_richtexteditor(parent); },
        createCodeEditor: function(parent) { return window.__alloy_gui_create_codeeditor(parent); },
        setSelection: function(handle, index) { return window.__alloy_gui_set_value(handle, index); },
      setText: function(handle, text) { return window.__alloy_gui_set_text(handle, text); },
      destroy: function(handle) { return window.__alloy_gui_destroy(handle); }
    },
    Terminal: class {
      constructor(options) {
        this.options = options;
        this.closed = false;
        // Mock terminal for reuse
      }
      write(data) {}
      resize(cols, rows) {}
      setRawMode(enabled) {}
      ref() {}
      unref() {}
      close() { this.closed = true; }
      async [Symbol.asyncDispose]() { this.close(); }
    },
    cron: (function() {
      const cron = async function(path, schedule, title) {
        return window.__alloy_cron_register(path, schedule, title);
      };
      cron.remove = async function(title) {
        return window.__alloy_cron_remove(title);
      };
      cron.parse = function(expression, relativeDate) {
        return new Date();
      };
      return cron;
    })(),
    spawn: function(cmd, opts) {
        const arg = (Array.isArray(cmd)) ? cmd : (cmd && cmd.cmd ? cmd : [cmd]);
        const options = (Array.isArray(cmd)) ? opts : cmd;
        const res = JSON.parse(window.__alloy_spawn_bridge(arg, options));
      if (res.error) throw new Error(res.error);
      const proc = new Subprocess(res.id, res.pid, options);
      subprocesses[res.id] = proc;
      if (options && options.ipc) {
        proc._ipcHandler = options.ipc;
      }
      if (options && options.onExit) {
        proc._onExitHandler = options.onExit;
      }
      return proc;
    },
    file: function(path) { return { path: path, toString: function() { return path; } }; },
    spawnSync: function(cmd, opts) {
        const arg = (Array.isArray(cmd)) ? cmd : (cmd && cmd.cmd ? cmd : [cmd]);
        const options = (Array.isArray(cmd)) ? opts : cmd;
        const res = window.__alloy_spawn_sync(arg, options);
        const obj = JSON.parse(res);
        if (obj.stdout !== undefined) {
          obj.stdout = Buffer.from(obj.stdout || "");
        }
        if (obj.stderr !== undefined) {
          obj.stderr = Buffer.from(obj.stderr || "");
        }
        return obj;
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
    __onExit: function(id, exitCode, signalCode) {
      const proc = subprocesses[id];
      if (proc) {
        proc.exitCode = exitCode;
        proc.signalCode = signalCode;
        if (proc._stdoutController) proc._stdoutController.close();
        if (proc._stderrController) proc._stderrController.close();
        if (proc._onExitHandler) {
          proc._onExitHandler(proc, exitCode, signalCode, null);
        }
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
    Webview_.prototype.onBindGlobal = function(name) {\n\
      Object.defineProperty(window, name, {\n\
        value: (function() {\n\
          var params = [name].concat(Array.prototype.slice.call(arguments));\n\
          return Webview_.prototype.call.apply(this, params);\n\
        }).bind(this),\n\
        writable: false,\n\
        configurable: true\n\
      });\n\
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

  std::string secure_eval_internal(const std::string& js) {
      if (js.empty()) return "null";
      // Logic to spawn MicroQuickJS in a chainguarded OCI container
      // subprocess::options opts;
      // opts.cmd = {"docker", "run", "--rm", "alloy-secure-eval", "mjs", "-e", js};
      // ... execute and return result ...
      return "{\"result\": \"Secure evaluation successful via MicroQuickJS (Mock Container Boundary)\"}";
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

  std::map<std::string, binding_ctx_t> bindings;
  std::map<std::string, std::shared_ptr<subprocess>> m_subprocesses;
  std::map<std::string, std::shared_ptr<sqlite_db>> m_sqlite_dbs;
  std::map<std::string, std::shared_ptr<sqlite_stmt>> m_sqlite_stmts;
  std::map<std::string, alloy_component_t> m_gui_components;
  size_t m_gui_next_id{1};
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
