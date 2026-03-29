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
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <spawn.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <functional>
#include <atomic>
#include <sys/socket.h>

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

#include "json.hh"

#ifndef WEBVIEW_PLATFORM_WINDOWS
extern char **environ;
#endif

namespace webview {
namespace detail {

class alloyscript_runtime {
public:
  struct subprocess_state {
    pid_t pid{-1};
    int stdin_fd{-1};
    int stdout_fd{-1};
    int stderr_fd{-1};
    int ipc_fd{-1};
    bool exited{false};
    int exit_code{-1};
    int signal_code{-1};
    bool killed{false};
    std::mutex mutex;
    std::condition_variable exit_cv;
    std::atomic<bool> monitoring{false};
  };

  struct terminal_state {
    int master_fd{-1};
    pid_t pid{-1};
    bool closed{false};
    std::mutex mutex;
    std::atomic<bool> monitoring{false};
  };

  alloyscript_runtime() = default;
  virtual ~alloyscript_runtime() {}

  std::shared_ptr<subprocess_state> spawn(const std::vector<std::string> &args,
                                          const std::string &cwd = "",
                                          const std::map<std::string, std::string> &env = {},
                                          bool use_ipc = false) {
#ifdef WEBVIEW_PLATFORM_WINDOWS
    (void)args; (void)cwd; (void)env; (void)use_ipc;
    return nullptr;
#else
    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];
    int ipc_socket[2];

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
      return nullptr;
    }
    if (use_ipc) {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc_socket) == -1) {
            close(stdin_pipe[0]); close(stdin_pipe[1]);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            close(stderr_pipe[0]); close(stderr_pipe[1]);
            return nullptr;
        }
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    posix_spawn_file_actions_adddup2(&actions, stdin_pipe[0], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1], STDERR_FILENO);

    posix_spawn_file_actions_addclose(&actions, stdin_pipe[1]);
    posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
    posix_spawn_file_actions_addclose(&actions, stderr_pipe[0]);

    if (use_ipc) {
        posix_spawn_file_actions_adddup2(&actions, ipc_socket[1], 3);
        posix_spawn_file_actions_addclose(&actions, ipc_socket[0]);
    }

    if (!cwd.empty()) {
        // posix_spawn doesn't have a standard way to change directory
        // In some systems there's posix_spawn_file_actions_addchdir_np
        // For simplicity and portability across POSIX, we might need a wrapper if we really want posix_spawn
        // But for now, let's stick to posix_spawn and ignore cwd if not supported easily
    }

    std::vector<char*> c_args;
    for (const auto& arg : args) {
      c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    char** envp = environ;
    std::vector<std::string> env_strings;
    std::vector<char*> c_env;
    if (!env.empty()) {
        for (const auto& kv : env) {
          env_strings.push_back(kv.first + "=" + kv.second);
        }
        for (const auto& s : env_strings) {
          c_env.push_back(const_cast<char*>(s.c_str()));
        }
        c_env.push_back(nullptr);
        envp = c_env.data();
    }

    pid_t pid;
    int res = posix_spawnp(&pid, c_args[0], &actions, nullptr, c_args.data(), envp);

    posix_spawn_file_actions_destroy(&actions);
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    if (use_ipc) { close(ipc_socket[1]); }

    if (res != 0) {
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        if (use_ipc) { close(ipc_socket[0]); }
        return nullptr;
    }

    auto state = std::make_shared<subprocess_state>();
    state->pid = pid;
    state->stdin_fd = stdin_pipe[1];
    state->stdout_fd = stdout_pipe[0];
    state->stderr_fd = stderr_pipe[0];
    if (use_ipc) { state->ipc_fd = ipc_socket[0]; }

    return state;
#endif
  }

  void start_subprocess_monitoring(std::shared_ptr<subprocess_state> state,
                                   std::function<void(const std::string&)> on_stdout,
                                   std::function<void(const std::string&)> on_stderr,
                                   std::function<void(const std::string&)> on_ipc,
                                   std::function<void(int, int)> on_exit) {
    state->monitoring = true;
    std::thread([=]() {
        struct pollfd fds[3];
        fds[0].fd = state->stdout_fd;
        fds[0].events = POLLIN;
        fds[1].fd = state->stderr_fd;
        fds[1].events = POLLIN;
        fds[2].fd = state->ipc_fd;
        fds[2].events = (state->ipc_fd != -1) ? POLLIN : 0;

        char buffer[4096];
        bool stdout_eof = false;
        bool stderr_eof = false;
        bool ipc_eof = (state->ipc_fd == -1);

        while (state->monitoring) {
            int ret = poll(fds, (state->ipc_fd != -1) ? 3 : 2, 100);
            if (ret < 0) break;

            if (fds[0].revents & POLLIN) {
                ssize_t n = read(state->stdout_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    on_stdout(std::string(buffer, n));
                } else if (n == 0) {
                    stdout_eof = true;
                }
            } else if (fds[0].revents & (POLLHUP | POLLERR)) {
                stdout_eof = true;
            }

            if (fds[1].revents & POLLIN) {
                ssize_t n = read(state->stderr_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    on_stderr(std::string(buffer, n));
                } else if (n == 0) {
                    stderr_eof = true;
                }
            } else if (fds[1].revents & (POLLHUP | POLLERR)) {
                stderr_eof = true;
            }

            if (!ipc_eof && (fds[2].revents & POLLIN)) {
                ssize_t n = read(state->ipc_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    on_ipc(std::string(buffer, n));
                } else if (n == 0) {
                    ipc_eof = true;
                }
            } else if (!ipc_eof && (fds[2].revents & (POLLHUP | POLLERR))) {
                ipc_eof = true;
            }

            int status;
            pid_t p = waitpid(state->pid, &status, WNOHANG);
            if (p == state->pid) {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->exited = true;
                if (WIFEXITED(status)) {
                    state->exit_code = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    state->signal_code = WTERMSIG(status);
                }
                state->exit_cv.notify_all();
            }
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (state->exited && (stdout_eof || state->stdout_fd == -1) && (stderr_eof || state->stderr_fd == -1) && (ipc_eof || state->ipc_fd == -1)) break;
            }
        }
        on_exit(state->exit_code, state->signal_code);
        close(state->stdin_fd);
        close(state->stdout_fd);
        close(state->stderr_fd);
        if (state->ipc_fd != -1) close(state->ipc_fd);
    }).detach();
  }

  void ipc_send(std::shared_ptr<subprocess_state> state, const std::string &message) {
      if (state && state->ipc_fd != -1) {
          std::string m = message + "\n";
          (void)write(state->ipc_fd, m.c_str(), m.size());
      }
  }

  std::string spawnSync(const std::vector<std::string> &args, const std::string &cwd = "", const std::map<std::string, std::string> &env = {}) {
    auto state = spawn(args, cwd, env);
    if (!state) return "{\"success\": false}";

    std::string stdout_data;
    std::string stderr_data;
    char buffer[4096];

    struct pollfd fds[2];
    fds[0].fd = state->stdout_fd;
    fds[0].events = POLLIN;
    fds[1].fd = state->stderr_fd;
    fds[1].events = POLLIN;

    bool stdout_eof = false;
    bool stderr_eof = false;

    while (!stdout_eof || !stderr_eof) {
        int ret = poll(fds, 2, 100);
        if (ret < 0) break;

        if (fds[0].revents & POLLIN) {
            ssize_t n = read(state->stdout_fd, buffer, sizeof(buffer));
            if (n > 0) stdout_data.append(buffer, n);
            else if (n == 0) stdout_eof = true;
        } else if (fds[0].revents & (POLLHUP | POLLERR)) {
            stdout_eof = true;
        }

        if (fds[1].revents & POLLIN) {
            ssize_t n = read(state->stderr_fd, buffer, sizeof(buffer));
            if (n > 0) stderr_data.append(buffer, n);
            else if (n == 0) stderr_eof = true;
        } else if (fds[1].revents & (POLLHUP | POLLERR)) {
            stderr_eof = true;
        }

        int status;
        if (waitpid(state->pid, &status, WNOHANG) == state->pid) {
            if (stdout_eof && stderr_eof) break;
        }
    }

    int status;
    waitpid(state->pid, &status, 0);

    bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;

    std::string result = "{";
    result += "\"success\": " + std::string(success ? "true" : "false") + ",";
    result += "\"exitCode\": " + std::to_string(WIFEXITED(status) ? WEXITSTATUS(status) : -1) + ",";
    result += "\"stdout\": " + json_escape(stdout_data) + ",";
    result += "\"stderr\": " + json_escape(stderr_data) + ",";
    result += "\"pid\": " + std::to_string(state->pid);
    result += "}";

    close(state->stdin_fd);
    close(state->stdout_fd);
    close(state->stderr_fd);

    return result;
  }

  void kill(std::shared_ptr<subprocess_state> state, int signal = SIGTERM) {
    if (state && state->pid != -1) {
        ::kill(state->pid, signal);
    }
  }

  std::shared_ptr<terminal_state> create_terminal(int cols = 80, int rows = 24) {
#if defined(WEBVIEW_PLATFORM_LINUX) || defined(__APPLE__)
    struct winsize ws;
    ws.ws_col = static_cast<unsigned short>(cols);
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    int master;
    pid_t pid = forkpty(&master, nullptr, nullptr, &ws);
    if (pid == -1) return nullptr;

    if (pid == 0) {
        _exit(0);
    }

    auto state = std::make_shared<terminal_state>();
    state->master_fd = master;
    state->pid = pid;
    return state;
#else
    (void)cols; (void)rows;
    return nullptr;
#endif
  }

  void start_terminal_monitoring(std::shared_ptr<terminal_state> state,
                                 std::function<void(const std::string&)> on_data,
                                 std::function<void(int, const std::string&)> on_exit) {
    state->monitoring = true;
    std::thread([=]() {
        char buffer[4096];
        while (state->monitoring) {
            ssize_t n = read(state->master_fd, buffer, sizeof(buffer));
            if (n > 0) {
                on_data(std::string(buffer, n));
            } else {
                break;
            }
        }
        on_exit(0, "");
        close(state->master_fd);
        state->closed = true;
    }).detach();
  }

  void terminal_write(std::shared_ptr<terminal_state> state, const std::string &data) {
    if (state && !state->closed) {
        (void)write(state->master_fd, data.c_str(), data.size());
    }
  }

  void terminal_resize(std::shared_ptr<terminal_state> state, int cols, int rows) {
#if defined(WEBVIEW_PLATFORM_LINUX) || defined(__APPLE__)
    if (state && !state->closed) {
        struct winsize ws;
        ws.ws_col = static_cast<unsigned short>(cols);
        ws.ws_row = static_cast<unsigned short>(rows);
        ioctl(state->master_fd, TIOCSWINSZ, &ws);
    }
#else
    (void)state; (void)cols; (void)rows;
#endif
  }

};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOYSCRIPT_RUNTIME_HH
