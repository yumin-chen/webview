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

#ifndef ALLOY_ENGINE_SUBPROCESS_HH
#define ALLOY_ENGINE_SUBPROCESS_HH

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <chrono>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#if defined(__linux__)
#include <pty.h>
#elif defined(__APPLE__)
#include <util.h>
#endif
extern char **environ;
#endif

namespace alloy::engine {

struct resource_usage {
    struct {
        long voluntary;
        long involuntary;
    } context_switches;
    struct {
        long user;   // microseconds
        long system; // microseconds
        long total;  // microseconds
    } cpu_time;
    long max_rss; // bytes
    struct {
        long sent;
        long received;
    } messages;
    struct {
        long in;
        long out;
    } ops;
    long shm_size;
    long signal_count;
    long swap_count;
};

class subprocess {
public:
    struct options {
        std::vector<std::string> cmd;
        std::string cwd;
        std::map<std::string, std::string> env;
        bool use_terminal = false;
        int timeout = 0; // ms
        int kill_signal = 15;
        struct terminal_opts {
            int cols = 80;
            int rows = 24;
            std::string name = "xterm-256color";
        } terminal;
    };

    struct callbacks {
        std::function<void(const std::string&, bool)> on_data;
        std::function<void(int, const std::string&)> on_exit;
    };

    subprocess(const options& opts) : m_opts(opts) {}
    ~subprocess() {
        if (m_read_thread_stdout.joinable()) m_read_thread_stdout.join();
        if (m_read_thread_stderr.joinable()) m_read_thread_stderr.join();
#ifdef _WIN32
        if (m_pi.hProcess) CloseHandle(m_pi.hProcess);
        if (m_pi.hThread) CloseHandle(m_pi.hThread);
        if (m_stdin_h) CloseHandle(m_stdin_h);
        if (m_stdout_h) CloseHandle(m_stdout_h);
        if (m_stderr_h) CloseHandle(m_stderr_h);
#else
        if (m_stdin_fd >= 0) ::close(m_stdin_fd);
        if (m_stdout_fd >= 0) ::close(m_stdout_fd);
        if (m_stderr_fd >= 0) ::close(m_stderr_fd);
#endif
    }

    bool spawn(callbacks c) {
        auto state = std::make_shared<shared_state>();
        state->on_data = c.on_data;
        state->on_exit = c.on_exit;
        m_state = state;

#ifdef _WIN32
        return spawn_windows();
#else
        return spawn_posix();
#endif
    }

    void kill(int sig = 15) {
#ifdef _WIN32
        if (m_pi.hProcess) TerminateProcess(m_pi.hProcess, (UINT)sig);
#else
        if (m_pid > 0) ::kill(m_pid, sig);
#endif
    }

    int get_pid() const {
#ifdef _WIN32
        return static_cast<int>(m_pi.dwProcessId);
#else
        return static_cast<int>(m_pid);
#endif
    }

    void write_stdin(const std::string& data) {
#ifdef _WIN32
        if (m_stdin_h) {
            DWORD written;
            WriteFile(m_stdin_h, data.c_str(), (DWORD)data.size(), &written, NULL);
        }
#else
        if (m_stdin_fd >= 0) ::write(m_stdin_fd, data.c_str(), data.size());
#endif
    }

    void close_stdin() {
#ifdef _WIN32
        if (m_stdin_h) { CloseHandle(m_stdin_h); m_stdin_h = NULL; }
#else
        if (m_stdin_fd >= 0) { ::close(m_stdin_fd); m_stdin_fd = -1; }
#endif
    }

    void resize_terminal(int cols, int rows) {
#ifndef _WIN32
        if (m_opts.use_terminal && m_stdout_fd >= 0) {
            struct winsize ws;
            ws.ws_col = (unsigned short)cols;
            ws.ws_row = (unsigned short)rows;
            ioctl(m_stdout_fd, TIOCSWINSZ, &ws);
        }
#endif
    }

    void set_raw_mode(bool enabled) {
#ifndef _WIN32
        if (m_opts.use_terminal && m_stdout_fd >= 0) {
            struct termios t;
            tcgetattr(m_stdout_fd, &t);
            if (enabled) cfmakeraw(&t);
            else {
                t.c_lflag |= (ECHO | ICANON | ISIG);
                t.c_iflag |= (ICRNL | IXON);
            }
            tcsetattr(m_stdout_fd, TCSANOW, &t);
        }
#endif
    }

    resource_usage get_resource_usage() {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return m_state->usage;
    }

private:
    struct shared_state {
        std::mutex mutex;
        std::function<void(const std::string&, bool)> on_data;
        std::function<void(int, const std::string&)> on_exit;
        bool finished = false;
        int exit_code = -1;
        std::string signal_name = "";
        resource_usage usage{};
    };

#ifndef _WIN32
    bool spawn_posix() {
        if (m_opts.use_terminal) return spawn_posix_pty();
        int out_pipe[2], err_pipe[2], in_pipe[2];
        if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0 || pipe(in_pipe) != 0) return false;
        std::vector<char*> argv;
        for (const auto& arg : m_opts.cmd) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        std::vector<std::string> env_list;
        std::vector<char*> envp;
        if (!m_opts.env.empty()) {
            for (const auto& pair : m_opts.env) {
                env_list.push_back(pair.first + "=" + pair.second);
                envp.push_back(const_cast<char*>(env_list.back().c_str()));
            }
            envp.push_back(nullptr);
        } else {
            char** e = environ;
            while (*e) envp.push_back(*e++);
            envp.push_back(nullptr);
        }
        m_pid = fork();
        if (m_pid < 0) return false;
        if (m_pid == 0) {
            if (!m_opts.cwd.empty()) { if (chdir(m_opts.cwd.c_str()) != 0) _exit(127); }
            dup2(out_pipe[1], STDOUT_FILENO); dup2(err_pipe[1], STDERR_FILENO); dup2(in_pipe[0], STDIN_FILENO);
            ::close(out_pipe[0]); ::close(out_pipe[1]); ::close(err_pipe[0]); ::close(err_pipe[1]); ::close(in_pipe[0]); ::close(in_pipe[1]);
            execvp(argv[0], argv.data()); _exit(127);
        }
        ::close(out_pipe[1]); ::close(err_pipe[1]); ::close(in_pipe[0]);
        m_stdout_fd = out_pipe[0]; m_stderr_fd = err_pipe[0]; m_stdin_fd = in_pipe[1];
        start_monitoring(); return true;
    }

    bool spawn_posix_pty() {
        int master_fd;
        struct winsize ws;
        ws.ws_col = (unsigned short)m_opts.terminal.cols;
        ws.ws_row = (unsigned short)m_opts.terminal.rows;
        m_pid = forkpty(&master_fd, NULL, NULL, &ws);
        if (m_pid < 0) return false;
        if (m_pid == 0) {
            if (!m_opts.cwd.empty()) chdir(m_opts.cwd.c_str());
            std::vector<char*> argv;
            for (const auto& arg : m_opts.cmd) argv.push_back(const_cast<char*>(arg.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data()); _exit(127);
        }
        m_stdout_fd = master_fd; m_stdin_fd = master_fd; m_stderr_fd = -1;
        start_monitoring(); return true;
    }
#else
    bool spawn_windows() {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
        HANDLE out_r, out_w, err_r, err_w, in_r, in_w;
        if (!CreatePipe(&out_r, &out_w, &sa, 0)) return false;
        if (!CreatePipe(&err_r, &err_w, &sa, 0)) return false;
        if (!CreatePipe(&in_r, &in_w, &sa, 0)) return false;
        SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0); SetHandleInformation(err_r, HANDLE_FLAG_INHERIT, 0); SetHandleInformation(in_w, HANDLE_FLAG_INHERIT, 0);
        STARTUPINFOA si = {0}; si.cb = sizeof(si); si.hStdOutput = out_w; si.hStdError = err_w; si.hStdInput = in_r; si.dwFlags |= STARTF_USESTDHANDLES;
        std::string cmdline = "";
        for (const auto& arg : m_opts.cmd) {
            std::string escaped = ""; bool quote = arg.empty() || arg.find_first_of(" \t\n\v\"") != std::string::npos;
            if (quote) escaped += '\"';
            for (size_t i = 0; i < arg.size(); ++i) {
                size_t backslashes = 0; while (i < arg.size() && arg[i] == '\\') { i++; backslashes++; }
                if (i == arg.size()) { if (quote) escaped.append(backslashes * 2, '\\'); else escaped.append(backslashes, '\\'); break; }
                else if (arg[i] == '\"') { escaped.append(backslashes * 2 + 1, '\\'); escaped += '\"'; }
                else { escaped.append(backslashes, '\\'); escaped += arg[i]; }
            }
            if (quote) escaped += '\"'; cmdline += (cmdline.empty() ? "" : " ") + escaped;
        }
        if (!CreateProcessA(NULL, (LPSTR)cmdline.c_str(), NULL, NULL, TRUE, 0, NULL, m_opts.cwd.empty() ? NULL : m_opts.cwd.c_str(), &si, &m_pi)) {
            CloseHandle(out_r); CloseHandle(out_w); CloseHandle(err_r); CloseHandle(err_w); CloseHandle(in_r); CloseHandle(in_w); return false;
        }
        CloseHandle(out_w); CloseHandle(err_w); CloseHandle(in_r);
        m_stdout_h = out_r; m_stderr_h = err_r; m_stdin_h = in_w;
        start_monitoring(); return true;
    }
#endif

    void start_monitoring() {
        auto state = m_state;
        auto timeout = m_opts.timeout;
        auto pid = m_pid;
#ifdef _WIN32
        auto hProcess = m_pi.hProcess;
#endif
        m_read_thread_stdout = std::thread([this, state]() {
            char buffer[4096];
            while (true) {
#ifdef _WIN32
                DWORD n; if (!m_stdout_h || !ReadFile(m_stdout_h, buffer, sizeof(buffer), &n, NULL) || n == 0) break;
#else
                if (m_stdout_fd < 0) break; ssize_t n = ::read(m_stdout_fd, buffer, sizeof(buffer)); if (n <= 0) break;
#endif
                std::lock_guard<std::mutex> lock(state->mutex); if (state->on_data) state->on_data(std::string(buffer, (size_t)n), false);
            }
        });

#ifdef _WIN32
        bool has_stderr = (m_stderr_h != NULL);
#else
        bool has_stderr = (m_stderr_fd >= 0);
#endif
        if (has_stderr) {
            m_read_thread_stderr = std::thread([this, state]() {
                char buffer[4096];
                while (true) {
#ifdef _WIN32
                    DWORD n; if (!m_stderr_h || !ReadFile(m_stderr_h, buffer, sizeof(buffer), &n, NULL) || n == 0) break;
#else
                    if (m_stderr_fd < 0) break; ssize_t n = ::read(m_stderr_fd, buffer, sizeof(buffer)); if (n <= 0) break;
#endif
                    std::lock_guard<std::mutex> lock(state->mutex); if (state->on_data) state->on_data(std::string(buffer, (size_t)n), true);
                }
            });
        }

        std::thread wait_thread([this, state, timeout, pid]() {
            int exit_status = -1;
            std::string signal_name = "";
            resource_usage usage{};

#ifdef _WIN32
            DWORD res = WaitForSingleObject(m_pi.hProcess, timeout > 0 ? timeout : INFINITE);
            if (res == WAIT_TIMEOUT) {
                TerminateProcess(m_pi.hProcess, 15);
                exit_status = 15;
                signal_name = "SIGTERM";
            } else {
                DWORD dwExitCode;
                if (GetExitCodeProcess(m_pi.hProcess, &dwExitCode))
                    exit_status = static_cast<int>(dwExitCode);
            }
            // Basic Windows resource usage
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(m_pi.hProcess, &pmc, sizeof(pmc))) {
                usage.max_rss = (long)pmc.PeakWorkingSetSize;
            }
            FILETIME createTime, exitTime, kernelTime, userTime;
            if (GetProcessTimes(m_pi.hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                auto to_us = [](FILETIME ft) {
                    ULARGE_INTEGER ui;
                    ui.LowPart = ft.dwLowDateTime;
                    ui.HighPart = ft.dwHighDateTime;
                    return (long)(ui.QuadPart / 10);
                };
                usage.cpu_time.user = to_us(userTime);
                usage.cpu_time.system = to_us(kernelTime);
                usage.cpu_time.total = usage.cpu_time.user + usage.cpu_time.system;
            }
#else
            struct rusage ru;
            if (timeout > 0) {
                auto start = std::chrono::steady_clock::now();
                while (true) {
                    int status;
                    int res = wait4(m_pid, &status, WNOHANG, &ru);
                    if (res > 0) {
                        if (WIFEXITED(status))
                            exit_status = WEXITSTATUS(status);
                        else if (WIFSIGNALED(status)) {
                            exit_status = WTERMSIG(status);
                            signal_name = "SIG" + std::to_string(exit_status);
                        }
                        break;
                    }
                    if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(timeout)) {
                        ::kill(m_pid, m_opts.kill_signal);
                        signal_name = "SIGTERM";
                        // We still need to reap it to get rusage
                        wait4(m_pid, &status, 0, &ru);
                        exit_status = m_opts.kill_signal;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            } else {
                int status;
                wait4(m_pid, &status, 0, &ru);
                if (WIFEXITED(status))
                    exit_status = WEXITSTATUS(status);
                else if (WIFSIGNALED(status)) {
                    exit_status = WTERMSIG(status);
                    signal_name = "SIG" + std::to_string(exit_status);
                }
            }
            usage.cpu_time.user = ru.ru_utime.tv_sec * 1000000L + ru.ru_utime.tv_usec;
            usage.cpu_time.system = ru.ru_stime.tv_sec * 1000000L + ru.ru_stime.tv_usec;
            usage.cpu_time.total = usage.cpu_time.user + usage.cpu_time.system;
            usage.max_rss = ru.ru_maxrss * 1024L; // ru_maxrss is in KB on Linux
            usage.context_switches.voluntary = ru.ru_nvcsw;
            usage.context_switches.involuntary = ru.ru_nivcsw;
            usage.messages.sent = ru.ru_msgsnd;
            usage.messages.received = ru.ru_msgrcv;
            usage.ops.in = ru.ru_inblock;
            usage.ops.out = ru.ru_oublock;
            usage.signal_count = ru.ru_nsignals;
            usage.swap_count = ru.ru_nswap;
#endif
            std::lock_guard<std::mutex> lock(state->mutex);
            state->exit_code = exit_status;
            state->signal_name = signal_name;
            state->usage = usage;
            state->finished = true;
            if (state->on_exit)
                state->on_exit(exit_status, signal_name);
        });
        wait_thread.detach();
    }

    options m_opts;
    std::shared_ptr<shared_state> m_state;
#ifdef _WIN32
    PROCESS_INFORMATION m_pi = {0}; HANDLE m_stdin_h = NULL; HANDLE m_stdout_h = NULL; HANDLE m_stderr_h = NULL;
#else
    pid_t m_pid = -1; int m_stdin_fd = -1; int m_stdout_fd = -1; int m_stderr_fd = -1;
#endif
    std::thread m_read_thread_stdout; std::thread m_read_thread_stderr;
};

} // namespace alloy::engine

#endif // ALLOY_ENGINE_SUBPROCESS_HH
