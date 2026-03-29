#ifndef ALLOY_SUBPROCESS_HPP
#define ALLOY_SUBPROCESS_HPP

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE process_handle_t;
typedef HANDLE pipe_handle_t;
#else
#include <unistd.h>
#include <sys/types.h>
#include <spawn.h>
typedef pid_t process_handle_t;
typedef int pipe_handle_t;
#endif

namespace alloy {

struct spawn_options {
    std::string cwd;
    std::map<std::string, std::string> env;
    std::string stdin_mode = "pipe";
    std::string stdout_mode = "pipe";
    std::string stderr_mode = "pipe";
    bool terminal = false;
    int terminal_cols = 80;
    int terminal_rows = 24;
    std::string terminal_name = "xterm-256color";
};

class subprocess {
public:
    subprocess(const std::vector<std::string>& cmd, const spawn_options& options);
    ~subprocess();

    int pid() const;
    void kill(int signal = 15);
    bool is_alive();
    int wait();

    struct result {
        int exit_code;
        std::string stdout_data;
        std::string stderr_data;
    };
    result wait_sync();

    void stdin_write(const std::string& data);
    void stdin_end();

    void terminal_write(const std::string& data);
    void terminal_resize(int cols, int rows);

    pipe_handle_t get_stdout_fd() const { return m_stdout_fd; }
    pipe_handle_t get_stderr_fd() const { return m_stderr_fd; }
    pipe_handle_t get_pty_fd() const { return m_pty_fd; }
    pipe_handle_t get_ipc_fd() const { return m_ipc_fd; }

    void send(const std::string& message);

    static ssize_t read_pipe(pipe_handle_t fd, char* buf, size_t sz);

private:
    std::vector<std::string> m_cmd;
    spawn_options m_options;
#ifdef _WIN32
    HANDLE m_process = INVALID_HANDLE_VALUE;
#else
    pid_t m_pid = -1;
#endif
    std::atomic<bool> m_exited{false};
    int m_exit_code = -1;

    pipe_handle_t m_stdin_fd = -1;
    pipe_handle_t m_stdout_fd = -1;
    pipe_handle_t m_stderr_fd = -1;
    pipe_handle_t m_pty_fd = -1;
    pipe_handle_t m_ipc_fd = -1;

    void spawn();
    void cleanup_pipes();
};

} // namespace alloy

#endif // ALLOY_SUBPROCESS_HPP
