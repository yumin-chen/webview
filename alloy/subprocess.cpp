#include "subprocess.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <pty.h>
#include <spawn.h>
#include <poll.h>
#include <sys/ioctl.h>
extern char **environ;
#endif

namespace alloy {

subprocess::subprocess(const std::vector<std::string>& cmd, const spawn_options& options)
    : m_cmd(cmd), m_options(options) {
    spawn();
}

subprocess::~subprocess() {
    if (!m_exited) {
        kill();
        wait();
    }
    cleanup_pipes();
}

void subprocess::cleanup_pipes() {
#ifdef _WIN32
    if (m_stdin_fd != -1) CloseHandle(m_stdin_fd);
    if (m_stdout_fd != -1) CloseHandle(m_stdout_fd);
    if (m_stderr_fd != -1) CloseHandle(m_stderr_fd);
#else
    if (m_stdin_fd != -1) close(m_stdin_fd);
    if (m_stdout_fd != -1) close(m_stdout_fd);
    if (m_stderr_fd != -1) close(m_stderr_fd);
    if (m_pty_fd != -1) close(m_pty_fd);
    if (m_ipc_fd != -1) close(m_ipc_fd);
#endif
    m_stdin_fd = m_stdout_fd = m_stderr_fd = m_pty_fd = m_ipc_fd = -1;
}

void subprocess::spawn() {
#ifdef _WIN32
    if (m_cmd.empty()) throw std::runtime_error("Command cannot be empty");

    std::string cmd_line;
    for (const auto& arg : m_cmd) {
        cmd_line += "\"" + arg + "\" "; // Basic quoting, should be improved
    }

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdinRd, hStdinWr, hStdoutRd, hStdoutWr, hStderrRd, hStderrWr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &saAttr, 0)) throw std::runtime_error("Stdout pipe failed");
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);
    if (!CreatePipe(&hStderrRd, &hStderrWr, &saAttr, 0)) throw std::runtime_error("Stderr pipe failed");
    SetHandleInformation(hStderrRd, HANDLE_FLAG_INHERIT, 0);
    if (!CreatePipe(&hStdinRd, &hStdinWr, &saAttr, 0)) throw std::runtime_error("Stdin pipe failed");
    SetHandleInformation(hStdinWr, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hStderrWr;
    si.hStdOutput = hStdoutWr;
    si.hStdInput = hStdinRd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    LPSTR lpCwd = m_options.cwd.empty() ? NULL : (LPSTR)m_options.cwd.c_str();

    if (!CreateProcess(NULL, (LPSTR)cmd_line.c_str(), NULL, NULL, TRUE, 0, NULL, lpCwd, &si, &pi)) {
        throw std::runtime_error("CreateProcess failed");
    }

    m_process = pi.hProcess;
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutWr);
    CloseHandle(hStderrWr);
    CloseHandle(hStdinRd);

    m_stdout_fd = hStdoutRd;
    m_stderr_fd = hStderrRd;
    m_stdin_fd = hStdinWr;
#else
    if (m_cmd.empty()) throw std::runtime_error("Command cannot be empty");

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2], ipc_pipe[2];
    pipe(ipc_pipe);
    m_ipc_fd = ipc_pipe[0];
    posix_spawn_file_actions_adddup2(&actions, ipc_pipe[1], 3);

    if (m_options.stdin_mode == "pipe") {
        pipe(stdin_pipe);
        m_stdin_fd = stdin_pipe[1];
        posix_spawn_file_actions_adddup2(&actions, stdin_pipe[0], 0);
        posix_spawn_file_actions_addclose(&actions, stdin_pipe[1]);
    } else if (m_options.stdin_mode == "inherit") {
        posix_spawn_file_actions_adddup2(&actions, 0, 0);
    } else {
        int devnull = open("/dev/null", O_RDONLY);
        posix_spawn_file_actions_adddup2(&actions, devnull, 0);
    }

    if (m_options.stdout_mode == "pipe") {
        pipe(stdout_pipe);
        m_stdout_fd = stdout_pipe[0];
        posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], 1);
        posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
    } else if (m_options.stdout_mode == "inherit") {
        posix_spawn_file_actions_adddup2(&actions, 1, 1);
    } else {
        int devnull = open("/dev/null", O_WRONLY);
        posix_spawn_file_actions_adddup2(&actions, devnull, 1);
    }

    if (m_options.stderr_mode == "pipe") {
        pipe(stderr_pipe);
        m_stderr_fd = stderr_pipe[0];
        posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1], 2);
        posix_spawn_file_actions_addclose(&actions, stderr_pipe[0]);
    } else if (m_options.stderr_mode == "inherit") {
        posix_spawn_file_actions_adddup2(&actions, 2, 2);
    } else {
        int devnull = open("/dev/null", O_WRONLY);
        posix_spawn_file_actions_adddup2(&actions, devnull, 2);
    }

    std::vector<char*> argv;
    for (const auto& arg : m_cmd) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);

    if (m_options.terminal) {
        struct winsize ws;
        ws.ws_col = m_options.terminal_cols;
        ws.ws_row = m_options.terminal_rows;
        pid_t pid = forkpty(&m_pty_fd, nullptr, nullptr, &ws);
        if (pid == -1) throw std::runtime_error("forkpty failed");
        if (pid == 0) {
            setenv("TERM", m_options.terminal_name.c_str(), 1);
            if (!m_options.cwd.empty()) chdir(m_options.cwd.c_str());
            execvp(argv[0], argv.data());
            exit(1);
        }
        m_pid = pid;
    } else {
        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        // Env handling ... simplified for now
        if (posix_spawnp(&m_pid, argv[0], &actions, &attr, argv.data(), environ) != 0) {
            throw std::runtime_error("posix_spawnp failed");
        }
        posix_spawnattr_destroy(&attr);
    }

    posix_spawn_file_actions_destroy(&actions);
    close(ipc_pipe[1]);
    if (m_options.stdin_mode == "pipe") close(stdin_pipe[0]);
    if (m_options.stdout_mode == "pipe") close(stdout_pipe[1]);
    if (m_options.stderr_mode == "pipe") close(stderr_pipe[1]);
#endif
}

int subprocess::pid() const {
#ifdef _WIN32
    return GetProcessId(m_process);
#else
    return m_pid;
#endif
}

void subprocess::kill(int sig) {
#ifdef _WIN32
    TerminateProcess(m_process, sig);
#else
    if (m_pid != -1) ::kill(m_pid, sig);
#endif
}

bool subprocess::is_alive() {
    if (m_exited) return false;
#ifdef _WIN32
    DWORD status;
    if (GetExitCodeProcess(m_process, &status)) return status == STILL_ACTIVE;
#else
    int status;
    pid_t result = waitpid(m_pid, &status, WNOHANG);
    if (result == 0) return true;
    if (result == m_pid) {
        m_exited = true;
        m_exit_code = WEXITSTATUS(status);
        return false;
    }
#endif
    return false;
}

int subprocess::wait() {
    if (m_exited) return m_exit_code;
#ifdef _WIN32
    WaitForSingleObject(m_process, INFINITE);
    DWORD status;
    GetExitCodeProcess(m_process, &status);
    m_exit_code = status;
#else
    int status;
    waitpid(m_pid, &status, 0);
    m_exit_code = WEXITSTATUS(status);
#endif
    m_exited = true;
    return m_exit_code;
}

subprocess::result subprocess::wait_sync() {
    result res;
#ifdef _WIN32
    // Windows polling ... simplified
    res.exit_code = wait();
#else
    std::vector<pollfd> fds;
    if (m_stdout_fd != -1) fds.push_back({m_stdout_fd, POLLIN, 0});
    if (m_stderr_fd != -1) fds.push_back({m_stderr_fd, POLLIN, 0});

    while (!fds.empty()) {
        if (poll(fds.data(), fds.size(), -1) > 0) {
            for (auto it = fds.begin(); it != fds.end(); ) {
                if (it->revents & POLLIN) {
                    char buf[4096];
                    ssize_t n = read(it->fd, buf, sizeof(buf));
                    if (n > 0) {
                        if (it->fd == m_stdout_fd) res.stdout_data.append(buf, n);
                        else res.stderr_data.append(buf, n);
                        ++it;
                    } else {
                        it = fds.erase(it);
                    }
                } else if (it->revents & (POLLHUP | POLLERR)) {
                    it = fds.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    res.exit_code = wait();
#endif
    return res;
}

void subprocess::stdin_write(const std::string& data) {
    if (m_stdin_fd != -1) {
#ifdef _WIN32
        DWORD written;
        WriteFile(m_stdin_fd, data.c_str(), data.size(), &written, NULL);
#else
        write(m_stdin_fd, data.c_str(), data.size());
#endif
    }
}

void subprocess::stdin_end() {
#ifdef _WIN32
    if (m_stdin_fd != -1) { CloseHandle(m_stdin_fd); m_stdin_fd = -1; }
#else
    if (m_stdin_fd != -1) { close(m_stdin_fd); m_stdin_fd = -1; }
#endif
}

void subprocess::send(const std::string& message) {
    if (m_ipc_fd != -1) {
        std::string frame = std::to_string(message.size()) + "\n" + message;
#ifdef _WIN32
        // Windows IPC write
#else
        write(m_ipc_fd, frame.c_str(), frame.size());
#endif
    }
}

void subprocess::terminal_write(const std::string& data) {
#ifndef _WIN32
    if (m_pty_fd != -1) write(m_pty_fd, data.c_str(), data.size());
#endif
}

void subprocess::terminal_resize(int cols, int rows) {
#ifndef _WIN32
    if (m_pty_fd != -1) {
        struct winsize ws;
        ws.ws_col = cols;
        ws.ws_row = rows;
        ioctl(m_pty_fd, TIOCSWINSZ, &ws);
    }
#endif
}

ssize_t subprocess::read_pipe(pipe_handle_t fd, char* buf, size_t sz) {
#ifdef _WIN32
    DWORD read_bytes;
    if (ReadFile(fd, buf, (DWORD)sz, &read_bytes, NULL)) return (ssize_t)read_bytes;
    return -1;
#else
    return read(fd, buf, sz);
#endif
}

} // namespace alloy
