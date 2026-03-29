#ifndef WEBVIEW_ALLOY_HH
#define WEBVIEW_ALLOY_HH

#include "macros.h"
#include "types.hh"
#include "errors.hh"
#include "json.hh"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#if defined(__APPLE__)
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#include <util.h>
#elif defined(__linux__)
#include <pty.h>
#include <utmp.h>
#else
extern char **environ;
#endif
#endif

namespace webview {
namespace detail {

inline std::string base64_encode(const std::string &in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

struct terminal_options {
    int cols = 80;
    int rows = 24;
    std::string name = "xterm-256color";
};

struct spawn_options {
    std::string cwd;
    std::map<std::string, std::string> env;
    bool has_ipc = false;
    terminal_options terminal;
    bool use_terminal = false;
    std::string stdin_type = "none";
    std::string stdout_type = "pipe";
    std::string stderr_type = "inherit";
    std::string serialization = "advanced";
    int timeout = 0;
};

struct sync_result {
    int exit_code = -1;
    std::string stdout_data;
    std::string stderr_data;
    std::string usage_json = "{}";
};

#ifdef _WIN32
class win32_subprocess {
public:
    win32_subprocess(HANDLE hProcess, HANDLE hThread, HANDLE hStdin, HANDLE hStdout, HANDLE hStderr)
        : m_process(hProcess), m_thread(hThread), m_stdin(hStdin), m_stdout(hStdout), m_stderr(hStderr) {}
    ~win32_subprocess() {
        if (m_stdin != INVALID_HANDLE_VALUE) CloseHandle(m_stdin);
        if (m_stdout != INVALID_HANDLE_VALUE) CloseHandle(m_stdout);
        if (m_stderr != INVALID_HANDLE_VALUE) CloseHandle(m_stderr);
        if (m_thread != INVALID_HANDLE_VALUE) CloseHandle(m_thread);
        if (m_process != INVALID_HANDLE_VALUE) CloseHandle(m_process);
    }
    void kill(int sig) { if (m_process != INVALID_HANDLE_VALUE) { TerminateProcess(m_process, (UINT)sig); m_killed = true; } }
    void stdin_write(const std::string& data) { if (m_stdin != INVALID_HANDLE_VALUE) { DWORD written; WriteFile(m_stdin, data.data(), (DWORD)data.size(), &written, NULL); } }
    void stdin_close() { if (m_stdin != INVALID_HANDLE_VALUE) { CloseHandle(m_stdin); m_stdin = INVALID_HANDLE_VALUE; } }
    HANDLE get_process() const { return m_process; }
    HANDLE get_stdout() const { return m_stdout; }
    HANDLE get_stderr() const { return m_stderr; }
    bool is_killed() const { return m_killed; }
    int get_pid() const { return (int)GetProcessId(m_process); }
private:
    HANDLE m_process; HANDLE m_thread; HANDLE m_stdin; HANDLE m_stdout; HANDLE m_stderr; std::atomic<bool> m_killed{false};
};
#else
class posix_subprocess {
public:
    posix_subprocess(pid_t pid, int stdin_fd, int stdout_fd, int stderr_fd, int ipc_fd = -1)
        : m_pid(pid), m_stdin(stdin_fd), m_stdout(stdout_fd), m_stderr(stderr_fd), m_ipc_fd(ipc_fd) {}
    ~posix_subprocess() {
        if (m_stdin != -1) close(m_stdin); if (m_stdout != -1) close(m_stdout); if (m_stderr != -1) close(m_stderr); if (m_ipc_fd != -1) close(m_ipc_fd);
    }
    void kill(int sig) { if (m_pid > 0) { ::kill(m_pid, sig); m_killed = true; } }
    void stdin_write(const std::string& data) { if (m_stdin != -1) { write(m_stdin, data.data(), data.size()); } }
    void stdin_close() { if (m_stdin != -1) { close(m_stdin); m_stdin = -1; } }
    void ipc_send(const std::string& message) { if (m_ipc_fd != -1) { write(m_ipc_fd, message.data(), message.size()); write(m_ipc_fd, "\n", 1); } }
    void ipc_disconnect() { if (m_ipc_fd != -1) { close(m_ipc_fd); m_ipc_fd = -1; } }
    void resize(int cols, int rows) {
#if defined(__APPLE__) || defined(__linux__)
        struct winsize ws; ws.ws_col = (unsigned short)cols; ws.ws_row = (unsigned short)rows;
        ioctl(m_stdin, TIOCSWINSZ, &ws);
#endif
    }
    pid_t get_pid() const { return m_pid; }
    bool is_killed() const { return m_killed; }
    int get_stdout() const { return m_stdout; }
    int get_stderr() const { return m_stderr; }
    int get_ipc_fd() const { return m_ipc_fd; }
private:
    pid_t m_pid; int m_stdin; int m_stdout; int m_stderr; int m_ipc_fd; std::atomic<bool> m_killed{false};
};
#endif

class alloy_runtime {
public:
    using dispatcher_t = std::function<noresult(std::function<void()>)>;
    alloy_runtime(dispatcher_t dispatcher, std::function<void(const std::string&)> evaluator)
        : m_dispatcher(dispatcher), m_evaluator(evaluator) {}

    int spawn(const std::string& id, const std::vector<std::string>& cmd, const spawn_options& opts) {
        if (cmd.empty()) return 0;
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa; sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
        HANDLE hStdinR = INVALID_HANDLE_VALUE, hStdinW = INVALID_HANDLE_VALUE, hStdoutR = INVALID_HANDLE_VALUE, hStdoutW = INVALID_HANDLE_VALUE, hStderrR = INVALID_HANDLE_VALUE, hStderrW = INVALID_HANDLE_VALUE;
        if (opts.stdin_type == "pipe") CreatePipe(&hStdinR, &hStdinW, &sa, 0);
        if (opts.stdout_type == "pipe") CreatePipe(&hStdoutR, &hStdoutW, &sa, 0);
        if (opts.stderr_type == "pipe") CreatePipe(&hStderrR, &hStderrW, &sa, 0);
        STARTUPINFOA si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
        si.hStdInput = (hStdinR != INVALID_HANDLE_VALUE) ? hStdinR : GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = (hStdoutW != INVALID_HANDLE_VALUE) ? hStdoutW : GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = (hStderrW != INVALID_HANDLE_VALUE) ? hStderrW : GetStdHandle(STD_ERROR_HANDLE);
        si.dwFlags |= STARTF_USESTDHANDLES;
        std::string cl = ""; for (const auto& s : cmd) { if (!cl.empty()) cl += " "; cl += "\"" + s + "\""; }
        if (CreateProcessA(NULL, (LPSTR)cl.c_str(), NULL, NULL, TRUE, 0, NULL, opts.cwd.empty() ? NULL : opts.cwd.c_str(), &si, &pi)) {
            if (hStdinR != INVALID_HANDLE_VALUE) CloseHandle(hStdinR); if (hStdoutW != INVALID_HANDLE_VALUE) CloseHandle(hStdoutW); if (hStderrW != INVALID_HANDLE_VALUE) CloseHandle(hStderrW);
            auto proc = std::make_shared<win32_subprocess>(pi.hProcess, pi.hThread, hStdinW, hStdoutR, hStderrR);
            { std::lock_guard<std::mutex> lock(m_mutex); m_subprocesses[id] = proc; }
            start_monitoring(id, proc); return proc->get_pid();
        }
        return 0;
#else
        int stdin_p[2] = {-1,-1}, stdout_p[2] = {-1,-1}, stderr_p[2] = {-1,-1}, ipc_p[2] = {-1,-1}, master = -1;
        if (opts.use_terminal) {
#if defined(__APPLE__) || defined(__linux__)
            struct winsize ws; ws.ws_col = (unsigned short)opts.terminal.cols; ws.ws_row = (unsigned short)opts.terminal.rows;
            pid_t pid = forkpty(&master, nullptr, nullptr, &ws);
            if (pid == 0) {
                if (!opts.cwd.empty()) chdir(opts.cwd.c_str());
                std::vector<char*> av; for (const auto& s : cmd) av.push_back(const_cast<char*>(s.c_str())); av.push_back(nullptr);
                execvp(av[0], av.data()); exit(1);
            } else if (pid > 0) {
                auto proc = std::make_shared<posix_subprocess>(pid, master, master, -1);
                { std::lock_guard<std::mutex> lock(m_mutex); m_subprocesses[id] = proc; }
                start_monitoring(id, proc); return (int)pid;
            }
#endif
        }
        posix_spawn_file_actions_t acts; posix_spawn_file_actions_init(&acts);
        if (opts.stdin_type == "pipe") { pipe(stdin_p); posix_spawn_file_actions_adddup2(&acts, stdin_p[0], STDIN_FILENO); posix_spawn_file_actions_addclose(&acts, stdin_p[1]); }
        if (opts.stdout_type == "pipe") { pipe(stdout_p); posix_spawn_file_actions_adddup2(&acts, stdout_p[1], STDOUT_FILENO); posix_spawn_file_actions_addclose(&acts, stdout_p[0]); }
        if (opts.stderr_type == "pipe") { pipe(stderr_p); posix_spawn_file_actions_adddup2(&acts, stderr_p[1], STDERR_FILENO); posix_spawn_file_actions_addclose(&acts, stderr_p[0]); }
        if (opts.has_ipc) { pipe(ipc_p); posix_spawn_file_actions_adddup2(&acts, ipc_p[0], 3); }
        std::vector<char*> av; for (const auto& s : cmd) av.push_back(const_cast<char*>(s.c_str())); av.push_back(nullptr);
        pid_t pid; int st = posix_spawn(&pid, av[0], &acts, nullptr, av.data(), environ);
        posix_spawn_file_actions_destroy(&acts);
        if (stdin_p[0] != -1) close(stdin_p[0]); if (stdout_p[1] != -1) close(stdout_p[1]); if (stderr_p[1] != -1) close(stderr_p[1]); if (ipc_p[0] != -1) close(ipc_p[0]);
        if (st != 0) {
            if (stdin_p[1] != -1) close(stdin_p[1]); if (stdout_p[0] != -1) close(stdout_p[0]); if (stderr_p[0] != -1) close(stderr_p[0]); if (ipc_p[1] != -1) close(ipc_p[1]);
            return 0;
        }
        auto proc = std::make_shared<posix_subprocess>(pid, stdin_p[1], stdout_p[0], stderr_p[0], ipc_p[1]);
        { std::lock_guard<std::mutex> lock(m_mutex); m_subprocesses[id] = proc; }
        start_monitoring(id, proc); return (int)pid;
#endif
    }

    sync_result spawn_sync(const std::vector<std::string>& cmd, const spawn_options& opts) {
        sync_result res;
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa; sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
        HANDLE hOutR, hOutW, hErrR, hErrW;
        CreatePipe(&hOutR, &hOutW, &sa, 0); SetHandleInformation(hOutR, HANDLE_FLAG_INHERIT, 0);
        CreatePipe(&hErrR, &hErrW, &sa, 0); SetHandleInformation(hErrR, HANDLE_FLAG_INHERIT, 0);
        STARTUPINFOA si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); si.hStdOutput = hOutW; si.hStdError = hErrW; si.dwFlags |= STARTF_USESTDHANDLES;
        std::string cl = ""; for (const auto& s : cmd) { if (!cl.empty()) cl += " "; cl += "\"" + s + "\""; }
        if (CreateProcessA(NULL, (LPSTR)cl.c_str(), NULL, NULL, TRUE, 0, NULL, opts.cwd.empty() ? NULL : opts.cwd.c_str(), &si, &pi)) {
            CloseHandle(hOutW); CloseHandle(hErrW);
            auto r_all = [](HANDLE h) { std::string d; char b[4096]; DWORD n; while (ReadFile(h, b, sizeof(b), &n, NULL) && n > 0) d.append(b, n); CloseHandle(h); return d; };
            std::thread t1([&] { res.stdout_data = r_all(hOutR); }); std::thread t2([&] { res.stderr_data = r_all(hErrR); });
            WaitForSingleObject(pi.hProcess, INFINITE); DWORD ec; GetExitCodeProcess(pi.hProcess, &ec); res.exit_code = (int)ec;
            t1.join(); t2.join(); CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        }
#else
        int so[2], se[2]; pipe(so); pipe(se);
        pid_t pid = fork();
        if (pid == 0) {
            if (!opts.cwd.empty()) chdir(opts.cwd.c_str());
            dup2(so[1], STDOUT_FILENO); dup2(se[1], STDERR_FILENO); close(so[0]); close(se[0]);
            std::vector<char*> av; for (const auto& s : cmd) av.push_back(const_cast<char*>(s.c_str())); av.push_back(nullptr);
            execvp(av[0], av.data()); exit(1);
        } else if (pid > 0) {
            close(so[1]); close(se[1]);
            auto r_all = [](int fd) { std::string d; char b[4096]; ssize_t n; while ((n = read(fd, b, sizeof(b))) > 0) d.append(b, n); close(fd); return d; };
            std::thread t1([&] { res.stdout_data = r_all(so[0]); }); std::thread t2([&] { res.stderr_data = r_all(se[0]); });
            int st; struct rusage usage; wait4(pid, &st, 0, &usage);
            res.exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
            t1.join(); t2.join(); long mr = usage.ru_maxrss;
#ifdef __APPLE__
            mr /= 1024;
#endif
            res.usage_json = "{\"maxRSS\":" + std::to_string(mr) + "}";
        }
#endif
        return res;
    }

    void kill(const std::string& id, int sig) { std::lock_guard<std::mutex> lock(m_mutex); auto it = m_subprocesses.find(id); if (it != m_subprocesses.end()) it->second->kill(sig); }
    void stdin_write(const std::string& id, const std::string& data) { std::lock_guard<std::mutex> lock(m_mutex); auto it = m_subprocesses.find(id); if (it != m_subprocesses.end()) it->second->stdin_write(data); }
    void stdin_close(const std::string& id) { std::lock_guard<std::mutex> lock(m_mutex); auto it = m_subprocesses.find(id); if (it != m_subprocesses.end()) it->second->stdin_close(); }
    void ipc_send(const std::string& id, const std::string& msg) {
#ifndef _WIN32
        std::lock_guard<std::mutex> lock(m_mutex); auto it = m_subprocesses.find(id); if (it != m_subprocesses.end()) it->second->ipc_send(msg);
#endif
    }
    void ipc_disconnect(const std::string& id) {
#ifndef _WIN32
        std::lock_guard<std::mutex> lock(m_mutex); auto it = m_subprocesses.find(id); if (it != m_subprocesses.end()) it->second->ipc_disconnect();
#endif
    }
    void terminal_resize(const std::string& id, int cols, int rows) {
#ifndef _WIN32
        std::lock_guard<std::mutex> lock(m_mutex); auto it = m_subprocesses.find(id); if (it != m_subprocesses.end()) it->second->resize(cols, rows);
#endif
    }

private:
    dispatcher_t m_dispatcher; std::function<void(const std::string&)> m_evaluator; std::mutex m_mutex;
#ifdef _WIN32
    std::map<std::string, std::shared_ptr<win32_subprocess>> m_subprocesses;
    void start_monitoring(const std::string& id, std::shared_ptr<win32_subprocess> proc) {
        if (proc->get_stdout() != INVALID_HANDLE_VALUE) {
            std::thread([this, id, proc]() {
                char b[4096]; DWORD n;
                while (ReadFile(proc->get_stdout(), b, sizeof(b), &n, NULL) && n > 0) {
                    std::string d(b, n); m_dispatcher([this, id, d]() { m_evaluator("window.alloy.__on_data(" + json_escape(id) + ", 'stdout', " + json_escape(base64_encode(d)) + ", true)"); return noresult{}; });
                }
            }).detach();
        }
        std::thread([this, id, proc]() {
            WaitForSingleObject(proc->get_process(), INFINITE); DWORD ec; GetExitCodeProcess(proc->get_process(), &ec);
            m_dispatcher([this, id, ec]() { m_evaluator("window.alloy.__on_exit(" + json_escape(id) + ", " + std::to_string(ec) + ", {})"); { std::lock_guard<std::mutex> lock(m_mutex); m_subprocesses.erase(id); } return noresult{}; });
        }).detach();
    }
#else
    std::map<std::string, std::shared_ptr<posix_subprocess>> m_subprocesses;
    void start_monitoring(const std::string& id, std::shared_ptr<posix_subprocess> proc) {
        auto create_reader = [this, id](int fd, const std::string& stream) {
            if (fd == -1) return;
            std::thread([this, id, fd, stream]() {
                char b[4096]; ssize_t n;
                while ((n = read(fd, b, sizeof(b))) > 0) {
                    std::string d(b, n); m_dispatcher([this, id, stream, d]() {
                        m_evaluator("window.alloy.__on_data(" + json_escape(id) + ", " + json_escape(stream) + ", " + json_escape(base64_encode(d)) + ", true)");
                        return noresult{};
                    });
                }
            }).detach();
        };
        create_reader(proc->get_stdout(), "stdout");
        create_reader(proc->get_stderr(), "stderr");
        create_reader(proc->get_ipc_fd(), "ipc");
        std::thread([this, id, proc]() {
            int st; struct rusage usage; wait4(proc->get_pid(), &st, 0, &usage);
            int ec = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
            m_dispatcher([this, id, ec, usage]() {
                long mr = usage.ru_maxrss;
#ifdef __APPLE__
                mr /= 1024;
#endif
                std::string uj = "{\"maxRSS\":" + std::to_string(mr) + "}";
                m_evaluator("window.alloy.__on_exit(" + json_escape(id) + ", " + std::to_string(ec) + ", " + uj + ")");
                { std::lock_guard<std::mutex> lock(m_mutex); m_subprocesses.erase(id); }
                return noresult{};
            });
        }).detach();
    }
#endif
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_ALLOY_HH
