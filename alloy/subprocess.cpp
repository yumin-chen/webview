#include "subprocess.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <vector>

#ifdef _WIN32
// Minimal Windows implementation placeholders
#else
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <pty.h>
#include <spawn.h>
#include <poll.h>
extern char **environ;
#endif

namespace alloy {

subprocess::subprocess(const std::vector<std::string>& cmd, const spawn_options& options)
    : m_cmd(cmd), m_options(options) {
    spawn();
}

subprocess::~subprocess() {
    if (m_pid != -1) {
        kill();
        wait();
    }
}

void subprocess::spawn() {
#ifdef _WIN32
    if (m_cmd.empty()) throw std::runtime_error("Command cannot be empty");

    std::string cmd_line;
    for (const auto& arg : m_cmd) cmd_line += "\"" + arg + "\" ";

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdinRd, hStdinWr, hStdoutRd, hStdoutWr, hStderrRd, hStderrWr;
    CreatePipe(&hStdoutRd, &hStdoutWr, &saAttr, 0);
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);
    CreatePipe(&hStderrRd, &hStderrWr, &saAttr, 0);
    SetHandleInformation(hStderrRd, HANDLE_FLAG_INHERIT, 0);
    CreatePipe(&hStdinRd, &hStdinWr, &saAttr, 0);
    SetHandleInformation(hStdinWr, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hStderrWr;
    si.hStdOutput = hStdoutWr;
    si.hStdInput = hStdinRd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, (LPSTR)cmd_line.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        throw std::runtime_error("CreateProcess failed");
    }

    m_process = pi.hProcess;
    m_pid = pi.dwProcessId;
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

    posix_spawn_file_actions_adddup2(&actions, ipc_pipe[1], 3); // IPC FD

    if (m_options.terminal) {
        struct winsize ws;
        ws.ws_col = m_options.terminal_cols;
        ws.ws_row = m_options.terminal_rows;

        pid_t pid = forkpty(&m_pty_fd, nullptr, nullptr, &ws);
        if (pid == -1) throw std::runtime_error("forkpty failed");

        if (pid == 0) { // Child
            setenv("TERM", m_options.terminal_name.c_str(), 1);
            execvp(argv[0], argv.data());
            exit(1);
        }
        m_pid = pid;
    } else {
        if (posix_spawnp(&m_pid, argv[0], &actions, nullptr, argv.data(), environ) != 0) {
            throw std::runtime_error("posix_spawnp failed");
        }
    }

    posix_spawn_file_actions_destroy(&actions);
    close(ipc_pipe[1]);
    if (m_options.stdin_mode == "pipe") close(stdin_pipe[0]);
    if (m_options.stdout_mode == "pipe") close(stdout_pipe[1]);
    if (m_options.stderr_mode == "pipe") close(stderr_pipe[1]);
#endif
}

void subprocess::send(const std::string& message) {
    if (m_ipc_fd != -1) {
        // Simple JSON-like frame: <length>\n<message>
        std::string frame = std::to_string(message.size()) + "\n" + message;
        write(m_ipc_fd, frame.c_str(), frame.size());
    }
}

void subprocess::terminal_write(const std::string& data) {
    if (m_pty_fd != -1) write(m_pty_fd, data.c_str(), data.size());
}

void subprocess::terminal_resize(int cols, int rows) {
    if (m_pty_fd != -1) {
        struct winsize ws;
        ws.ws_col = cols;
        ws.ws_row = rows;
        ioctl(m_pty_fd, TIOCSWINSZ, &ws);
    }
}

int subprocess::pid() const { return m_pid; }

void subprocess::kill(int sig) {
#ifdef _WIN32
    // Windows TerminateProcess
#else
    if (m_pid != -1) ::kill(m_pid, sig);
#endif
}

int subprocess::wait() {
#ifdef _WIN32
    // Windows WaitForSingleObject
    return 0;
#else
    if (m_pid == -1) return -1;
    int status;
    waitpid(m_pid, &status, 0);
    m_pid = -1;
    return WEXITSTATUS(status);
#endif
}

subprocess::result subprocess::wait_sync() {
    result res;
#ifdef _WIN32
    // Windows sync wait
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
    if (m_stdin_fd != -1) write(m_stdin_fd, data.c_str(), data.size());
}

void subprocess::stdin_end() {
    if (m_stdin_fd != -1) { close(m_stdin_fd); m_stdin_fd = -1; }
}

} // namespace alloy
