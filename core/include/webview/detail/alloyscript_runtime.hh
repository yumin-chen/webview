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

#ifndef WEBVIEW_DETAIL_ALLOYSCRIPT_RUNTIME_HH
#define WEBVIEW_DETAIL_ALLOYSCRIPT_RUNTIME_HH

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <functional>
#include <atomic>
#include <sstream>
#include <algorithm>
#include <deque>
#include <fstream>
#include <sqlite3.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <spawn.h>
#ifdef WEBVIEW_PLATFORM_LINUX
#include <pty.h>
#include <utmp.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif
#ifdef __APPLE__
#include <util.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif
extern char **environ;
#endif

#include "json.hh"

namespace webview {
namespace detail {

class alloyscript_runtime {
public:
  struct sqlite_db_state {
    sqlite3 *db{nullptr};
    std::map<std::string, sqlite3_stmt *> stmt_cache;
    bool strict{false};
    bool safe_integers{false};

    ~sqlite_db_state() {
        for (auto &kv : stmt_cache) {
            sqlite3_finalize(kv.second);
        }
        if (db) sqlite3_close(db);
    }
  };

  struct shared_state {
#ifdef _WIN32
    HANDLE hProcess{NULL};
    HANDLE hStdin{NULL};
    HANDLE hStdout{NULL};
    HANDLE hStderr{NULL};
    DWORD dwProcessId{0};
#else
    pid_t pid{-1};
    int stdin_fd{-1};
    int stdout_fd{-1};
    int stderr_fd{-1};
    int ipc_fd{-1};
#endif
    std::atomic<bool> exited{false};
    int exit_code{-1};
    int signal_code{-1};
    std::atomic<bool> monitoring{false};

    std::mutex mutex;
    std::deque<std::string> stdin_queue;
    std::condition_variable stdin_cv;
    std::thread stdin_thread;

    std::function<void(const std::string&)> on_stdout;
    std::function<void(const std::string&)> on_stderr;
    std::function<void(const std::string&)> on_ipc;
    std::function<void(int, int)> on_exit;

    ~shared_state() {
        monitoring = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            stdin_cv.notify_all();
        }
        if (stdin_thread.joinable()) stdin_thread.join();

#ifdef _WIN32
        if (hProcess) CloseHandle(hProcess);
        if (hStdin) CloseHandle(hStdin);
        if (hStdout) CloseHandle(hStdout);
        if (hStderr) CloseHandle(hStderr);
#else
        if (stdin_fd != -1) close(stdin_fd);
        if (stdout_fd != -1) close(stdout_fd);
        if (stderr_fd != -1) close(stderr_fd);
        if (ipc_fd != -1) close(ipc_fd);
#endif
    }
  };

  struct shell_result {
    int exit_code;
    std::string stdout_data;
    std::string stderr_data;
  };

  alloyscript_runtime() = default;
  virtual ~alloyscript_runtime() {}

  static std::vector<std::string> tokenize(const std::string& command) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_quotes = false;
    char quote_char = 0;
    bool escaped = false;

    for (size_t i = 0; i < command.length(); ++i) {
        char c = command[i];
        if (escaped) {
            token += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (in_quotes) {
            if (c == quote_char) {
                in_quotes = false;
            } else {
                token += c;
            }
        } else {
            if (c == '"' || c == '\'') {
                in_quotes = true;
                quote_char = c;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
  }

  // Native built-ins
  static shell_result builtin_echo(const std::vector<std::string>& args) {
      std::string out;
      for (size_t i = 1; i < args.size(); ++i) {
          out += args[i] + (i == args.size() - 1 ? "" : " ");
      }
      return {0, out + "\n", ""};
  }

  static shell_result builtin_pwd() {
      char buf[4096];
#ifdef _WIN32
      if (_getcwd(buf, sizeof(buf))) return {0, std::string(buf) + "\n", ""};
#else
      if (getcwd(buf, sizeof(buf))) return {0, std::string(buf) + "\n", ""};
#endif
      return {1, "", "pwd failed\n"};
  }

  static shell_result builtin_ls(const std::vector<std::string>& args) {
      std::string path = ".";
      if (args.size() > 1) path = args[1];
      std::string out;
#ifdef _WIN32
      WIN32_FIND_DATA findFileData;
      HANDLE hFind = FindFirstFile((path + "\\*").c_str(), &findFileData);
      if (hFind == INVALID_HANDLE_VALUE) return {1, "", "ls: " + path + ": No such file or directory\n"};
      do {
          out += std::string(findFileData.cFileName) + "\n";
      } while (FindNextFile(hFind, &findFileData) != 0);
      FindClose(hFind);
#else
      DIR *dir = opendir(path.c_str());
      if (!dir) return {1, "", "ls: " + path + ": No such file or directory\n"};
      struct dirent *ent;
      while ((ent = readdir(dir)) != nullptr) {
          out += std::string(ent->d_name) + "\n";
      }
      closedir(dir);
#endif
      return {0, out, ""};
  }

  static shell_result builtin_mkdir(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "mkdir: missing operand\n"};
#ifdef _WIN32
      if (_mkdir(args[1].c_str()) == 0) return {0, "", ""};
#else
      if (mkdir(args[1].c_str(), 0755) == 0) return {0, "", ""};
#endif
      return {1, "", "mkdir failed\n"};
  }

  static shell_result builtin_rm(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "rm: missing operand\n"};
      if (remove(args[1].c_str()) == 0) return {0, "", ""};
      return {1, "", "rm failed\n"};
  }

  static shell_result builtin_cat(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "cat: missing operand\n"};
      std::string out;
      for (size_t i = 1; i < args.size(); ++i) {
          std::ifstream ifs(args[i]);
          if (!ifs) return {1, "", "cat: " + args[i] + ": No such file or directory\n"};
          std::stringstream ss; ss << ifs.rdbuf(); out += ss.str();
      }
      return {0, out, ""};
  }

  static shell_result builtin_touch(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "touch: missing operand\n"};
      for (size_t i = 1; i < args.size(); ++i) {
          std::ofstream ofs(args[i], std::ios_base::app);
      }
      return {0, "", ""};
  }

  static shell_result builtin_which(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "which: missing operand\n"};
#ifdef _WIN32
      return {1, "", "which not implemented on Windows\n"};
#else
      std::string cmd = "which " + args[1];
      FILE* pipe = popen(cmd.c_str(), "r");
      if (!pipe) return {1, "", "which failed\n"};
      char buffer[1024]; std::string result = "";
      while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
      pclose(pipe);
      if (result.empty()) return {1, "", ""};
      return {0, result, ""};
#endif
  }

  static shell_result builtin_mv(const std::vector<std::string>& args) {
      if (args.size() < 3) return {1, "", "mv: missing operand\n"};
      if (rename(args[1].c_str(), args[2].c_str()) == 0) return {0, "", ""};
      return {1, "", "mv failed\n"};
  }

  static shell_result builtin_dirname(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "dirname: missing operand\n"};
      std::string path = args[1];
      size_t pos = path.find_last_of("/\\");
      if (pos == std::string::npos) return {0, ".\n", ""};
      if (pos == 0) return {0, "/\n", ""};
      return {0, path.substr(0, pos) + "\n", ""};
  }

  static shell_result builtin_basename(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "basename: missing operand\n"};
      std::string path = args[1];
      size_t pos = path.find_last_of("/\\");
      if (pos == std::string::npos) return {0, path + "\n", ""};
      return {0, path.substr(pos + 1) + "\n", ""};
  }

  static shell_result builtin_seq(const std::vector<std::string>& args) {
      if (args.size() < 2) return {1, "", "seq: missing operand\n"};
      int start = 1, end = 0, step = 1;
      if (args.size() == 2) { end = std::stoi(args[1]); }
      else if (args.size() == 3) { start = std::stoi(args[1]); end = std::stoi(args[2]); }
      else if (args.size() >= 4) { start = std::stoi(args[1]); step = std::stoi(args[2]); end = std::stoi(args[3]); }
      std::string out;
      for (int i = start; (step > 0 ? i <= end : i >= end); i += step) {
          out += std::to_string(i) + "\n";
      }
      return {0, out, ""};
  }

  static shell_result builtin_yes(const std::vector<std::string>& args) {
      std::string msg = "y";
      if (args.size() > 1) msg = args[1];
      std::string out;
      for (int i = 0; i < 100; i++) out += msg + "\n";
      return {0, out, ""};
  }

  static shell_result builtin_true() { return {0, "", ""}; }
  static shell_result builtin_false() { return {1, "", ""}; }

  static shell_result builtin_exit(const std::vector<std::string>& args) {
      int code = 0;
      if (args.size() > 1) code = std::stoi(args[1]);
      exit(code);
  }

  // GUI methods (Basic native control wrapping)
  struct gui_component {
      std::string id;
      std::string type;
      void* native_handle{nullptr};
      std::map<std::string, std::string> props;
      std::vector<std::string> children;
  };

  std::map<std::string, std::shared_ptr<gui_component>> m_gui_components;

  std::string gui_create(const std::string& type, const std::string& props_json) {
      auto comp = std::make_shared<gui_component>();
      comp->type = type;
      comp->id = std::to_string(reinterpret_cast<uintptr_t>(comp.get()));

#ifdef _WIN32
      if (type == "Window") {
          comp->native_handle = CreateWindowExW(0, L"AlloyWindow", L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, GetModuleHandle(NULL), NULL);
      } else if (type == "Button") {
          comp->native_handle = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, NULL, NULL, GetModuleHandle(NULL), NULL);
      } else if (type == "TextField") {
          comp->native_handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 0, 0, 200, 25, NULL, NULL, GetModuleHandle(NULL), NULL);
      } else if (type == "TextArea") {
          comp->native_handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL, 0, 0, 200, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
      } else if (type == "Label") {
          comp->native_handle = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, NULL, NULL, GetModuleHandle(NULL), NULL);
      } else if (type == "CheckBox") {
          comp->native_handle = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 0, 0, 100, 20, NULL, NULL, GetModuleHandle(NULL), NULL);
      } else if (type == "ProgressBar") {
          comp->native_handle = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 200, 20, NULL, NULL, GetModuleHandle(NULL), NULL);
      }
#endif

#ifdef WEBVIEW_PLATFORM_LINUX
      if (type == "Window") {
          comp->native_handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
          gtk_window_set_default_size(GTK_WINDOW(comp->native_handle), 800, 600);
      } else if (type == "Button") {
          comp->native_handle = gtk_button_new();
      } else if (type == "Label") {
          comp->native_handle = gtk_label_new("");
      } else if (type == "TextField") {
          comp->native_handle = gtk_entry_new();
      } else if (type == "TextArea") {
          comp->native_handle = gtk_text_view_new();
      } else if (type == "CheckBox") {
          comp->native_handle = gtk_check_button_new();
      } else if (type == "RadioButton") {
          comp->native_handle = gtk_radio_button_new(NULL);
      } else if (type == "ComboBox") {
          comp->native_handle = gtk_combo_box_text_new();
      } else if (type == "Slider") {
          comp->native_handle = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
      } else if (type == "ProgressBar") {
          comp->native_handle = gtk_progress_bar_new();
      } else if (type == "TabView") {
          comp->native_handle = gtk_notebook_new();
      } else if (type == "ListView") {
          comp->native_handle = gtk_tree_view_new();
      } else if (type == "TreeView") {
          comp->native_handle = gtk_tree_view_new();
      } else if (type == "WebView") {
          comp->native_handle = webkit_web_view_new();
      } else if (type == "VStack") {
          comp->native_handle = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      } else if (type == "HStack") {
          comp->native_handle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
      } else if (type == "ScrollView") {
          comp->native_handle = gtk_scrolled_window_new(NULL, NULL);
      }
#endif
      m_gui_components[comp->id] = comp;
      return comp->id;
  }

  void gui_update(const std::string& id, const std::string& props_json) {
      auto it = m_gui_components.find(id);
      if (it == m_gui_components.end()) return;
      auto comp = it->second;

#ifdef _WIN32
      if (comp->type == "Window") {
          auto title = json_parse(props_json, "title", 0);
          if (!title.empty()) SetWindowTextA((HWND)comp->native_handle, title.c_str());
      } else if (comp->type == "Button") {
          auto label = json_parse(props_json, "label", 0);
          if (!label.empty()) SetWindowTextA((HWND)comp->native_handle, label.c_str());
      } else if (comp->type == "Label") {
          auto text = json_parse(props_json, "text", 0);
          if (!text.empty()) SetWindowTextA((HWND)comp->native_handle, text.c_str());
      } else if (comp->type == "TextField" || comp->type == "TextArea") {
          auto value = json_parse(props_json, "value", 0);
          if (!value.empty()) SetWindowTextA((HWND)comp->native_handle, value.c_str());
      } else if (comp->type == "ProgressBar") {
          auto value = json_parse(props_json, "value", 0);
          if (!value.empty()) SendMessage((HWND)comp->native_handle, PBM_SETPOS, (WPARAM)(std::stod(value) * 100), 0);
      }
#endif

#ifdef WEBVIEW_PLATFORM_LINUX
      if (comp->type == "Window") {
          auto title = json_parse(props_json, "title", 0);
          if (!title.empty()) gtk_window_set_title(GTK_WINDOW(comp->native_handle), title.c_str());
          gtk_widget_show_all(GTK_WIDGET(comp->native_handle));
      } else if (comp->type == "Button") {
          auto label = json_parse(props_json, "label", 0);
          if (!label.empty()) gtk_button_set_label(GTK_BUTTON(comp->native_handle), label.c_str());
      } else if (comp->type == "Label") {
          auto text = json_parse(props_json, "text", 0);
          if (!text.empty()) gtk_label_set_text(GTK_LABEL(comp->native_handle), text.c_str());
      } else if (comp->type == "ProgressBar") {
          auto value = json_parse(props_json, "value", 0);
          if (!value.empty()) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(comp->native_handle), std::stod(value));
      }
      gtk_widget_show(GTK_WIDGET(comp->native_handle));
#endif
  }

  void gui_add_child(const std::string& parent_id, const std::string& child_id) {
      auto it_p = m_gui_components.find(parent_id);
      auto it_c = m_gui_components.find(child_id);
      if (it_p == m_gui_components.end() || it_c == m_gui_components.end()) return;
#ifdef WEBVIEW_PLATFORM_LINUX
      if (it_p->second->type == "Window") {
          gtk_container_add(GTK_CONTAINER(it_p->second->native_handle), GTK_WIDGET(it_c->second->native_handle));
      } else if (it_p->second->type == "VStack" || it_p->second->type == "HStack") {
          gtk_box_pack_start(GTK_BOX(it_p->second->native_handle), GTK_WIDGET(it_c->second->native_handle), TRUE, TRUE, 0);
      }
#endif
  }

  // SQLite methods
  std::shared_ptr<sqlite_db_state> sqlite_open(const std::string &filename, bool readonly = false, bool create = true, bool strict = false, bool safe_integers = false) {
      sqlite3 *db;
      int flags = readonly ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | (create ? SQLITE_OPEN_CREATE : 0));
      if (sqlite3_open_v2(filename.c_str(), &db, flags, nullptr) != SQLITE_OK) {
          return nullptr;
      }
      auto state = std::make_shared<sqlite_db_state>();
      state->db = db;
      state->strict = strict;
      state->safe_integers = safe_integers;
      return state;
  }

  sqlite3_stmt* sqlite_prepare(std::shared_ptr<sqlite_db_state> db_state, const std::string &sql, bool cache = true) {
      if (!db_state || !db_state->db) return nullptr;
      if (cache) {
          auto it = db_state->stmt_cache.find(sql);
          if (it != db_state->stmt_cache.end()) {
              sqlite3_reset(it->second);
              sqlite3_clear_bindings(it->second);
              return it->second;
          }
      }
      sqlite3_stmt *stmt;
      if (sqlite3_prepare_v2(db_state->db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
          return nullptr;
      }
      if (cache) {
          db_state->stmt_cache[sql] = stmt;
      }
      return stmt;
  }

  bool sqlite_bind(sqlite3_stmt *stmt, int index, const std::string &val, const std::string &type) {
      if (type == "string") return sqlite3_bind_text(stmt, index, val.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
      if (type == "number") return sqlite3_bind_double(stmt, index, std::stod(val)) == SQLITE_OK;
      if (type == "bigint") return sqlite3_bind_int64(stmt, index, std::stoll(val)) == SQLITE_OK;
      if (type == "boolean") return sqlite3_bind_int(stmt, index, (val == "true" ? 1 : 0)) == SQLITE_OK;
      if (type == "null") return sqlite3_bind_null(stmt, index) == SQLITE_OK;
      return false;
  }

  std::shared_ptr<shared_state> spawn(const std::vector<std::string> &args,
                                          const std::string &cwd = "",
                                          const std::map<std::string, std::string> &env = {},
                                          bool use_ipc = false) {
    auto state = std::make_shared<shared_state>();
#ifdef _WIN32
    (void)env; (void)use_ipc;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    HANDLE hChildStd_IN_Rd = NULL; HANDLE hChildStd_IN_Wr = NULL;
    HANDLE hChildStd_OUT_Rd = NULL; HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Rd = NULL; HANDLE hChildStd_ERR_Wr = NULL;
    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) return nullptr;
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) return nullptr;
    if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) return nullptr;
    if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) return nullptr;
    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) return nullptr;
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) return nullptr;
    PROCESS_INFORMATION piProcInfo; STARTUPINFO siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION)); ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_ERR_Wr; siStartInfo.hStdOutput = hChildStd_OUT_Wr; siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    std::string cmdLine; for (const auto& arg : args) cmdLine += "\"" + arg + "\" ";
    if (!CreateProcess(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, cwd.empty() ? NULL : cwd.c_str(), &siStartInfo, &piProcInfo)) return nullptr;
    CloseHandle(hChildStd_OUT_Wr); CloseHandle(hChildStd_ERR_Wr); CloseHandle(hChildStd_IN_Rd);
    state->hProcess = piProcInfo.hProcess; state->hStdin = hChildStd_IN_Wr; state->hStdout = hChildStd_OUT_Rd; state->hStderr = hChildStd_ERR_Rd; state->dwProcessId = piProcInfo.dwProcessId;
    CloseHandle(piProcInfo.hThread);
#else
    (void)env;
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2], ipc_socket[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) return nullptr;
    if (use_ipc && socketpair(AF_UNIX, SOCK_STREAM, 0, ipc_socket) == -1) return nullptr;
    pid_t pid = fork();
    if (pid == -1) return nullptr;
    if (pid == 0) {
      dup2(stdin_pipe[0], STDIN_FILENO); dup2(stdout_pipe[1], STDOUT_FILENO); dup2(stderr_pipe[1], STDERR_FILENO);
      close(stdin_pipe[0]); close(stdin_pipe[1]); close(stdout_pipe[0]); close(stdout_pipe[1]); close(stderr_pipe[0]); close(stderr_pipe[1]);
      if (use_ipc) { dup2(ipc_socket[1], 3); close(ipc_socket[0]); close(ipc_socket[1]); }
      if (!cwd.empty()) (void)chdir(cwd.c_str());
      std::vector<char*> c_args; for (const auto& arg : args) c_args.push_back(const_cast<char*>(arg.c_str()));
      c_args.push_back(nullptr); execvp(c_args[0], c_args.data()); _exit(127);
    }
    close(stdin_pipe[0]); close(stdout_pipe[1]); close(stderr_pipe[1]);
    if (use_ipc) close(ipc_socket[1]);
    state->pid = pid; state->stdin_fd = stdin_pipe[1]; state->stdout_fd = stdout_pipe[0]; state->stderr_fd = stderr_pipe[0]; if (use_ipc) state->ipc_fd = ipc_socket[0];
#endif
    return state;
  }

  void start_stdin_thread(std::shared_ptr<shared_state> state) {
      state->stdin_thread = std::thread([state]() {
          while (state->monitoring) {
              std::string data;
              {
                  std::unique_lock<std::mutex> lock(state->mutex);
                  state->stdin_cv.wait(lock, [&] { return !state->monitoring || !state->stdin_queue.empty(); });
                  if (!state->monitoring) break;
                  data = std::move(state->stdin_queue.front()); state->stdin_queue.pop_front();
              }
#ifdef _WIN32
              DWORD written; WriteFile(state->hStdin, data.c_str(), (DWORD)data.size(), &written, NULL);
#else
              (void)write(state->stdin_fd, data.c_str(), data.size());
#endif
          }
      });
  }

  void queue_stdin(std::shared_ptr<shared_state> state, const std::string& data) {
      if (!state) return;
      std::lock_guard<std::mutex> lock(state->mutex);
      state->stdin_queue.push_back(data); state->stdin_cv.notify_one();
  }

  void start_monitoring(std::shared_ptr<shared_state> state) {
    state->monitoring = true;
    start_stdin_thread(state);
    std::thread([state]() {
#ifdef _WIN32
        char buffer[4096]; DWORD bytesRead;
        while (state->monitoring) {
            if (ReadFile(state->hStdout, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
                if (state->on_stdout) state->on_stdout(std::string(buffer, bytesRead));
            } else break;
        }
        WaitForSingleObject(state->hProcess, INFINITE);
        DWORD exitCode; GetExitCodeProcess(state->hProcess, &exitCode);
        state->exit_code = (int)exitCode; state->exited = true;
        if (state->on_exit) state->on_exit(state->exit_code, 0);
#else
        struct pollfd fds[3];
        fds[0].fd = state->stdout_fd; fds[0].events = POLLIN;
        fds[1].fd = state->stderr_fd; fds[1].events = POLLIN;
        fds[2].fd = state->ipc_fd; fds[2].events = (state->ipc_fd != -1) ? POLLIN : 0;
        char buffer[4096]; bool out_eof = false, err_eof = false, ipc_eof = (state->ipc_fd == -1);
        while (state->monitoring && (!out_eof || !err_eof || !ipc_eof)) {
            int ret = poll(fds, (state->ipc_fd != -1) ? 3 : 2, 100);
            if (ret < 0) break;
            if (fds[0].revents & POLLIN) {
                ssize_t n = read(state->stdout_fd, buffer, sizeof(buffer));
                if (n > 0) { if (state->on_stdout) state->on_stdout(std::string(buffer, n)); } else out_eof = true;
            } else if (fds[0].revents & (POLLHUP | POLLERR)) out_eof = true;
            if (fds[1].revents & POLLIN) {
                ssize_t n = read(state->stderr_fd, buffer, sizeof(buffer));
                if (n > 0) { if (state->on_stderr) state->on_stderr(std::string(buffer, n)); } else err_eof = true;
            } else if (fds[1].revents & (POLLHUP | POLLERR)) err_eof = true;
            if (!ipc_eof && (fds[2].revents & POLLIN)) {
                ssize_t n = read(state->ipc_fd, buffer, sizeof(buffer));
                if (n > 0) { if (state->on_ipc) state->on_ipc(std::string(buffer, n)); } else ipc_eof = true;
            } else if (!ipc_eof && (fds[2].revents & (POLLHUP | POLLERR))) ipc_eof = true;
            int status; if (!state->exited && waitpid(state->pid, &status, WNOHANG) == state->pid) {
                state->exited = true; if (WIFEXITED(status)) state->exit_code = WEXITSTATUS(status); else if (WIFSIGNALED(status)) state->signal_code = WTERMSIG(status);
            }
            if (state->exited && out_eof && err_eof && ipc_eof) break;
        }
        if (!state->exited) {
            int status; if (waitpid(state->pid, &status, 0) == state->pid) {
                state->exited = true; if (WIFEXITED(status)) state->exit_code = WEXITSTATUS(status); else if (WIFSIGNALED(status)) state->signal_code = WTERMSIG(status);
            }
        }
        if (state->on_exit) state->on_exit(state->exit_code, state->signal_code);
#endif
    }).detach();
  }

  std::string spawnSync(const std::vector<std::string> &args, const std::string &cwd = "", const std::map<std::string, std::string> &env = {}) {
    auto state = spawn(args, cwd, env);
    if (!state) return "{\"success\": false}";
    std::string stdout_acc, stderr_acc; char buffer[4096];
    bool success = false;
#ifdef _WIN32
    DWORD bytesRead; while (ReadFile(state->hStdout, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) stdout_acc.append(buffer, bytesRead);
    WaitForSingleObject(state->hProcess, INFINITE);
    DWORD exitCode; GetExitCodeProcess(state->hProcess, &exitCode);
    success = (exitCode == 0);
#else
    struct pollfd fds[2]; fds[0].fd = state->stdout_fd; fds[0].events = POLLIN; fds[1].fd = state->stderr_fd; fds[1].events = POLLIN;
    bool out_eof = false, err_eof = false;
    while (!out_eof || !err_eof) {
        if (poll(fds, 2, 100) < 0) break;
        if (fds[0].revents & POLLIN) {
            ssize_t n = read(state->stdout_fd, buffer, sizeof(buffer));
            if (n > 0) stdout_acc.append(buffer, n); else out_eof = true;
        } else if (fds[0].revents & (POLLHUP | POLLERR)) out_eof = true;
        if (fds[1].revents & POLLIN) {
            ssize_t n = read(state->stderr_fd, buffer, sizeof(buffer));
            if (n > 0) stderr_acc.append(buffer, n); else err_eof = true;
        } else if (fds[1].revents & (POLLHUP | POLLERR)) err_eof = true;
    }
    int status; waitpid(state->pid, &status, 0); success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
    return "{\"success\": " + std::string(success ? "true" : "false") + ", \"stdout\": " + json_escape(stdout_acc) + ", \"stderr\": " + json_escape(stderr_acc) + "}";
  }

  shell_result shell(const std::string &command, const std::string &cwd = "") {
    std::vector<std::string> pipe_segments; bool in_quotes = false; char quote_char = 0; size_t last = 0;
    for (size_t i = 0; i < command.length(); ++i) {
        char c = command[i];
        if (c == '"' || c == '\'') {
            if (!in_quotes) { in_quotes = true; quote_char = c; } else if (c == quote_char) { in_quotes = false; }
        } else if (c == '|' && !in_quotes) {
            pipe_segments.push_back(command.substr(last, i - last)); last = i + 1;
        }
    }
    pipe_segments.push_back(command.substr(last));
    if (pipe_segments.size() == 1) {
        std::vector<std::string> args = tokenize(pipe_segments[0]); if (args.empty()) return {0, "", ""};
        if (!cwd.empty()) {
#ifdef _WIN32
            _chdir(cwd.c_str());
#else
            (void)chdir(cwd.c_str());
#endif
        }
        const std::string& cmd = args[0];
        if (cmd == "echo") return builtin_echo(args);
        if (cmd == "pwd") return builtin_pwd();
        if (cmd == "ls") return builtin_ls(args);
        if (cmd == "mkdir") return builtin_mkdir(args);
        if (cmd == "rm") return builtin_rm(args);
        if (cmd == "cat") return builtin_cat(args);
        if (cmd == "touch") return builtin_touch(args);
        if (cmd == "which") return builtin_which(args);
        if (cmd == "mv") return builtin_mv(args);
        if (cmd == "dirname") return builtin_dirname(args);
        if (cmd == "basename") return builtin_basename(args);
        if (cmd == "seq") return builtin_seq(args);
        if (cmd == "yes") return builtin_yes(args);
        if (cmd == "exit") return builtin_exit(args);
        if (cmd == "true") return {0, "", ""};
        if (cmd == "false") return {1, "", ""};
    }

#ifdef _WIN32
    std::vector<std::string> args = tokenize(pipe_segments[0]);
    auto state = spawn(args, cwd); if (!state) return {127, "", "command not found\n"};
    std::string stdout_acc, stderr_acc; std::mutex acc_mutex;
    state->on_stdout = [&](const std::string &data) { std::lock_guard<std::mutex> l(acc_mutex); stdout_acc += data; };
    state->on_stderr = [&](const std::string &data) { std::lock_guard<std::mutex> l(acc_mutex); stderr_acc += data; };
    bool done = false; int exit_code = 0;
    state->on_exit = [&](int code, int sig) { (void)sig; exit_code = code; done = true; };
    start_monitoring(state); while (!done) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return {exit_code, stdout_acc, stderr_acc};
#else
    int num_cmds = static_cast<int>(pipe_segments.size());
    std::vector<int> pipes_fds(2 * (num_cmds - 1));
    for (int i = 0; i < num_cmds - 1; i++) { if (pipe(pipes_fds.data() + 2 * i) < 0) return {1, "", "pipe failed\n"}; }
    int stdout_pipe[2]; int stderr_pipe[2]; if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) return {1, "", "pipe failed\n"};
    std::vector<pid_t> pids;
    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) dup2(pipes_fds[2 * (i - 1)], STDIN_FILENO);
            if (i < num_cmds - 1) dup2(pipes_fds[2 * i + 1], STDOUT_FILENO); else dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stderr_pipe[1], STDERR_FILENO);
            for (int j = 0; j < 2 * (num_cmds - 1); j++) close(pipes_fds[j]);
            close(stdout_pipe[0]); close(stdout_pipe[1]); close(stderr_pipe[0]); close(stderr_pipe[1]);
            if (!cwd.empty()) (void)chdir(cwd.c_str());
            std::vector<std::string> args_ = tokenize(pipe_segments[i]); if (args_.empty()) _exit(0);
            std::vector<char*> c_args; for (const auto& arg : args_) c_args.push_back(const_cast<char*>(arg.c_str()));
            c_args.push_back(nullptr); execvp(c_args[0], c_args.data()); _exit(127);
        }
        pids.push_back(pid);
    }
    for (int i = 0; i < 2 * (num_cmds - 1); i++) close(pipes_fds[i]);
    close(stdout_pipe[1]); close(stderr_pipe[1]);
    std::string stdout_acc, stderr_acc; char buffer[4096]; struct pollfd poll_fds[2];
    poll_fds[0].fd = stdout_pipe[0]; poll_fds[0].events = POLLIN; poll_fds[1].fd = stderr_pipe[0]; poll_fds[1].events = POLLIN;
    bool out_eof = false, err_eof = false;
    while (!out_eof || !err_eof) {
        if (poll(poll_fds, 2, 100) < 0) break;
        if (poll_fds[0].revents & POLLIN) { ssize_t n = read(stdout_pipe[0], buffer, sizeof(buffer)); if (n > 0) stdout_acc.append(buffer, n); else out_eof = true; } else if (poll_fds[0].revents & (POLLHUP | POLLERR)) out_eof = true;
        if (poll_fds[1].revents & POLLIN) { ssize_t n = read(stderr_pipe[0], buffer, sizeof(buffer)); if (n > 0) stderr_acc.append(buffer, n); else err_eof = true; } else if (poll_fds[1].revents & (POLLHUP | POLLERR)) err_eof = true;
    }
    int last_status = 0;
    for (pid_t pid : pids) { int status; waitpid(pid, &status, 0); if (WIFEXITED(status)) last_status = WEXITSTATUS(status); }
    close(stdout_pipe[0]); close(stderr_pipe[0]); return {last_status, stdout_acc, stderr_acc};
#endif
  }
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOYSCRIPT_RUNTIME_HH
