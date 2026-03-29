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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <spawn.h>
#ifdef WEBVIEW_PLATFORM_LINUX
#include <pty.h>
#include <utmp.h>
#include <termios.h>
#include <sys/ioctl.h>
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

  struct terminal_state {
#ifndef _WIN32
    int master_fd{-1};
    pid_t pid{-1};
#endif
    bool closed{false};
    std::atomic<bool> monitoring{false};
    std::function<void(const std::string&)> on_data;
    std::function<void(int, const std::string&)> on_exit;
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

    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Rd = NULL;
    HANDLE hChildStd_ERR_Wr = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) return nullptr;
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) return nullptr;
    if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) return nullptr;
    if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) return nullptr;
    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) return nullptr;
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) return nullptr;

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_ERR_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmdLine;
    for (const auto& arg : args) {
        cmdLine += "\"" + arg + "\" ";
    }

    if (!CreateProcess(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, cwd.empty() ? NULL : cwd.c_str(), &siStartInfo, &piProcInfo)) {
        return nullptr;
    }

    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_ERR_Wr);
    CloseHandle(hChildStd_IN_Rd);

    state->hProcess = piProcInfo.hProcess;
    state->hStdin = hChildStd_IN_Wr;
    state->hStdout = hChildStd_OUT_Rd;
    state->hStderr = hChildStd_ERR_Rd;
    state->dwProcessId = piProcInfo.dwProcessId;
    CloseHandle(piProcInfo.hThread);
#else
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2], ipc_socket[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) return nullptr;
    if (use_ipc && socketpair(AF_UNIX, SOCK_STREAM, 0, ipc_socket) == -1) return nullptr;

    pid_t pid = fork();
    if (pid == -1) return nullptr;
    if (pid == 0) {
      dup2(stdin_pipe[0], STDIN_FILENO);
      dup2(stdout_pipe[1], STDOUT_FILENO);
      dup2(stderr_pipe[1], STDERR_FILENO);
      close(stdin_pipe[0]); close(stdin_pipe[1]);
      close(stdout_pipe[0]); close(stdout_pipe[1]);
      close(stderr_pipe[0]); close(stderr_pipe[1]);
      if (use_ipc) { dup2(ipc_socket[1], 3); close(ipc_socket[0]); close(ipc_socket[1]); }
      if (!cwd.empty()) (void)chdir(cwd.c_str());
      std::vector<char*> c_args;
      for (const auto& arg : args) c_args.push_back(const_cast<char*>(arg.c_str()));
      c_args.push_back(nullptr);
      execvp(c_args[0], c_args.data());
      _exit(127);
    }
    close(stdin_pipe[0]); close(stdout_pipe[1]); close(stderr_pipe[1]);
    if (use_ipc) close(ipc_socket[1]);
    state->pid = pid;
    state->stdin_fd = stdin_pipe[1];
    state->stdout_fd = stdout_pipe[0];
    state->stderr_fd = stderr_pipe[0];
    if (use_ipc) state->ipc_fd = ipc_socket[0];
#endif
    return state;
  }

  void start_monitoring(std::shared_ptr<shared_state> state) {
    state->monitoring = true;
    start_stdin_thread(state);
    std::thread([state]() {
#ifdef _WIN32
        char buffer[4096];
        DWORD bytesRead;
        while (state->monitoring) {
            if (ReadFile(state->hStdout, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
                if (state->on_stdout) state->on_stdout(std::string(buffer, bytesRead));
            } else break;
        }
        // Simplified monitoring for stderr and exit on Windows
        WaitForSingleObject(state->hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(state->hProcess, &exitCode);
        state->exit_code = (int)exitCode;
        state->exited = true;
        if (state->on_exit) state->on_exit(state->exit_code, 0);
#else
        struct pollfd fds[3];
        fds[0].fd = state->stdout_fd; fds[0].events = POLLIN;
        fds[1].fd = state->stderr_fd; fds[1].events = POLLIN;
        fds[2].fd = state->ipc_fd; fds[2].events = (state->ipc_fd != -1) ? POLLIN : 0;
        char buffer[4096];
        bool out_eof = false, err_eof = false, ipc_eof = (state->ipc_fd == -1);
        while (state->monitoring && (!out_eof || !err_eof || !ipc_eof)) {
            int ret = poll(fds, (state->ipc_fd != -1) ? 3 : 2, 100);
            if (ret < 0) break;
            if (fds[0].revents & POLLIN) {
                ssize_t n = read(state->stdout_fd, buffer, sizeof(buffer));
                if (n > 0) { if (state->on_stdout) state->on_stdout(std::string(buffer, n)); }
                else out_eof = true;
            } else if (fds[0].revents & (POLLHUP | POLLERR)) out_eof = true;
            if (fds[1].revents & POLLIN) {
                ssize_t n = read(state->stderr_fd, buffer, sizeof(buffer));
                if (n > 0) { if (state->on_stderr) state->on_stderr(std::string(buffer, n)); }
                else err_eof = true;
            } else if (fds[1].revents & (POLLHUP | POLLERR)) err_eof = true;
            if (!ipc_eof && (fds[2].revents & POLLIN)) {
                ssize_t n = read(state->ipc_fd, buffer, sizeof(buffer));
                if (n > 0) { if (state->on_ipc) state->on_ipc(std::string(buffer, n)); }
                else ipc_eof = true;
            } else if (!ipc_eof && (fds[2].revents & (POLLHUP | POLLERR))) ipc_eof = true;
            int status;
            if (waitpid(state->pid, &status, WNOHANG) == state->pid) {
                state->exited = true;
                if (WIFEXITED(status)) state->exit_code = WEXITSTATUS(status);
                else if (WIFSIGNALED(status)) state->signal_code = WTERMSIG(status);
            }
            if (state->exited && out_eof && err_eof && ipc_eof) break;
        }
        if (state->on_exit) state->on_exit(state->exit_code, state->signal_code);
#endif
    }).detach();
  }

  void start_stdin_thread(std::shared_ptr<shared_state> state) {
      state->stdin_thread = std::thread([state]() {
          while (state->monitoring) {
              std::string data;
              {
                  std::unique_lock<std::mutex> lock(state->mutex);
                  state->stdin_cv.wait(lock, [&] { return !state->monitoring || !state->stdin_queue.empty(); });
                  if (!state->monitoring) break;
                  data = std::move(state->stdin_queue.front());
                  state->stdin_queue.pop_front();
              }
#ifdef _WIN32
              DWORD written;
              WriteFile(state->hStdin, data.c_str(), (DWORD)data.size(), &written, NULL);
#else
              (void)write(state->stdin_fd, data.c_str(), data.size());
#endif
          }
      });
  }

  void queue_stdin(std::shared_ptr<shared_state> state, const std::string& data) {
      if (!state) return;
      std::lock_guard<std::mutex> lock(state->mutex);
      state->stdin_queue.push_back(data);
      state->stdin_cv.notify_one();
  }

};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOYSCRIPT_RUNTIME_HH
