#ifndef WEBVIEW_DETAIL_ALLOY_PROCESS_HH
#define WEBVIEW_DETAIL_ALLOY_PROCESS_HH

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <sys/resource.h>

#if defined(__linux__)
#include <pty.h>
#include <utmp.h>
#elif defined(__APPLE__)
#include <util.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

extern char **environ;

namespace webview {
namespace detail {

class AlloyProcess {
public:
    struct TerminalOptions {
        int cols = 80;
        int rows = 24;
        std::string name = "xterm-256color";
    };

    struct Options {
        std::vector<std::string> argv;
        std::string cwd;
        std::map<std::string, std::string> env;
        std::shared_ptr<TerminalOptions> terminal;
    };

    struct ResourceUsage {
        long maxRSS;
        struct {
            long user;
            long system;
        } cpuTime;
    };

    struct SyncResult {
        std::vector<char> stdout_data;
        std::vector<char> stderr_data;
        int exitCode;
        bool success;
        ResourceUsage resourceUsage;
        pid_t pid;
    };

    using DataCallback = std::function<void(const std::vector<char>&)>;
    using ExitCallback = std::function<void(int, ResourceUsage)>;

    AlloyProcess() : m_pid(-1), m_running(false), m_pty_master(-1) {
        m_stdin_pipe[0] = m_stdin_pipe[1] = -1;
        m_stdout_pipe[0] = m_stdout_pipe[1] = -1;
        m_stderr_pipe[0] = m_stderr_pipe[1] = -1;
    }

    ~AlloyProcess() {
        m_running = false;
        if (m_stdout_thread.joinable()) m_stdout_thread.join();
        if (m_stderr_thread.joinable()) m_stderr_thread.join();
        if (m_exit_thread.joinable()) m_exit_thread.join();
        close_pipes();
        if (m_pty_master != -1) {
            close(m_pty_master);
            m_pty_master = -1;
        }
    }

    bool spawn(const Options& options,
               DataCallback stdout_cb,
               DataCallback stderr_cb,
               ExitCallback exit_cb) {
        if (options.argv.empty()) return false;

        if (options.terminal) {
            return spawn_pty(options, stdout_cb, exit_cb);
        }

        if (pipe(m_stdin_pipe) == -1 || pipe(m_stdout_pipe) == -1 || pipe(m_stderr_pipe) == -1) {
            close_pipes();
            return false;
        }

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);

        posix_spawn_file_actions_adddup2(&actions, m_stdin_pipe[0], STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&actions, m_stdout_pipe[1], STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, m_stderr_pipe[1], STDERR_FILENO);

        posix_spawn_file_actions_addclose(&actions, m_stdin_pipe[1]);
        posix_spawn_file_actions_addclose(&actions, m_stdout_pipe[0]);
        posix_spawn_file_actions_addclose(&actions, m_stderr_pipe[0]);

#if defined(__GLIBC__) || defined(__APPLE__)
        if (!options.cwd.empty()) {
#if defined(__APPLE__)
            // macOS 10.15+
            posix_spawn_file_actions_addchdir(&actions, options.cwd.c_str());
#else
            posix_spawn_file_actions_addchdir_np(&actions, options.cwd.c_str());
#endif
        }
#endif

        std::vector<char*> argv;
        for (const auto& arg : options.argv) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        char **envp = environ;
        std::vector<std::string> env_strings;
        std::vector<char*> env_ptrs;
        if (!options.env.empty()) {
            for (const auto& kv : options.env) {
                env_strings.push_back(kv.first + "=" + kv.second);
            }
            for (auto& s : env_strings) {
                env_ptrs.push_back(const_cast<char*>(s.c_str()));
            }
            env_ptrs.push_back(nullptr);
            envp = env_ptrs.data();
        }

        int status = posix_spawnp(&m_pid, argv[0], &actions, nullptr, argv.data(), envp);

        posix_spawn_file_actions_destroy(&actions);

        close(m_stdin_pipe[0]); m_stdin_pipe[0] = -1;
        close(m_stdout_pipe[1]); m_stdout_pipe[1] = -1;
        close(m_stderr_pipe[1]); m_stderr_pipe[1] = -1;

        if (status != 0) {
            close_pipes();
            return false;
        }

        m_running = true;
        m_stdout_thread = std::thread(&AlloyProcess::read_loop, this, m_stdout_pipe[0], stdout_cb);
        m_stderr_thread = std::thread(&AlloyProcess::read_loop, this, m_stderr_pipe[0], stderr_cb);
        m_exit_thread = std::thread(&AlloyProcess::wait_loop, this, exit_cb);

        return true;
    }

    bool spawn_pty(const Options& options, DataCallback data_cb, ExitCallback exit_cb) {
#if defined(__linux__) || defined(__APPLE__)
        struct winsize ws = { (unsigned short)options.terminal->rows, (unsigned short)options.terminal->cols, 0, 0 };
        pid_t pid = forkpty(&m_pty_master, nullptr, nullptr, &ws);
        if (pid < 0) return false;

        if (pid == 0) {
            if (!options.cwd.empty()) {
                if (chdir(options.cwd.c_str()) != 0) _exit(1);
            }

            if (!options.env.empty()) {
                for (const auto& kv : options.env) {
                    setenv(kv.first.c_str(), kv.second.c_str(), 1);
                }
            }
            setenv("TERM", options.terminal->name.c_str(), 1);

            std::vector<char*> argv;
            for (const auto& arg : options.argv) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
            _exit(1);
        }

        m_pid = pid;
        m_running = true;
        m_stdout_thread = std::thread(&AlloyProcess::read_loop, this, m_pty_master, data_cb);
        m_exit_thread = std::thread(&AlloyProcess::wait_loop, this, exit_cb);
        return true;
#else
        return false;
#endif
    }

    SyncResult spawn_sync(const Options& options) {
        SyncResult result;
        result.success = false;
        result.exitCode = -1;

        if (options.argv.empty()) return result;

        if (pipe(m_stdin_pipe) == -1 || pipe(m_stdout_pipe) == -1 || pipe(m_stderr_pipe) == -1) {
            close_pipes();
            return result;
        }

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, m_stdin_pipe[0], STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&actions, m_stdout_pipe[1], STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, m_stderr_pipe[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, m_stdin_pipe[1]);
        posix_spawn_file_actions_addclose(&actions, m_stdout_pipe[0]);
        posix_spawn_file_actions_addclose(&actions, m_stderr_pipe[0]);

#if defined(__GLIBC__) || defined(__APPLE__)
        if (!options.cwd.empty()) {
#if defined(__APPLE__)
            posix_spawn_file_actions_addchdir(&actions, options.cwd.c_str());
#else
            posix_spawn_file_actions_addchdir_np(&actions, options.cwd.c_str());
#endif
        }
#endif

        std::vector<char*> argv;
        for (const auto& arg : options.argv) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        char **envp = environ;
        std::vector<std::string> env_strings;
        std::vector<char*> env_ptrs;
        if (!options.env.empty()) {
            for (const auto& kv : options.env) {
                env_strings.push_back(kv.first + "=" + kv.second);
            }
            for (auto& s : env_strings) {
                env_ptrs.push_back(const_cast<char*>(s.c_str()));
            }
            env_ptrs.push_back(nullptr);
            envp = env_ptrs.data();
        }

        int status = posix_spawnp(&m_pid, argv[0], &actions, nullptr, argv.data(), envp);
        posix_spawn_file_actions_destroy(&actions);

        close(m_stdin_pipe[0]); m_stdin_pipe[0] = -1;
        close(m_stdin_pipe[1]); m_stdin_pipe[1] = -1;
        close(m_stdout_pipe[1]); m_stdout_pipe[1] = -1;
        close(m_stderr_pipe[1]); m_stderr_pipe[1] = -1;

        if (status != 0) {
            close_pipes();
            return result;
        }

        result.pid = m_pid;

        auto read_all = [](int fd, std::vector<char>& out) {
            char buffer[4096];
            while (true) {
                ssize_t n = read(fd, buffer, sizeof(buffer));
                if (n > 0) {
                    out.insert(out.end(), buffer, buffer + n);
                } else if (n == 0) {
                    break;
                } else {
                    if (errno == EINTR) continue;
                    break;
                }
            }
        };

        std::thread t1(read_all, m_stdout_pipe[0], std::ref(result.stdout_data));
        std::thread t2(read_all, m_stderr_pipe[0], std::ref(result.stderr_data));

        int wait_status;
        waitpid(m_pid, &wait_status, 0);

        t1.join();
        t2.join();
        close_pipes();

        struct rusage r_usage;
        if (getrusage(RUSAGE_CHILDREN, &r_usage) == 0) {
            result.resourceUsage.maxRSS = r_usage.ru_maxrss * 1024;
            result.resourceUsage.cpuTime.user = r_usage.ru_utime.tv_sec * 1000000 + r_usage.ru_utime.tv_usec;
            result.resourceUsage.cpuTime.system = r_usage.ru_stime.tv_sec * 1000000 + r_usage.ru_stime.tv_usec;
        }

        if (WIFEXITED(wait_status)) {
            result.exitCode = WEXITSTATUS(wait_status);
            result.success = (result.exitCode == 0);
        } else if (WIFSIGNALED(wait_status)) {
            result.exitCode = -WTERMSIG(wait_status);
            result.success = false;
        }

        return result;
    }

    void write_stdin(const std::vector<char>& data) {
        int fd = (m_pty_master != -1) ? m_pty_master : m_stdin_pipe[1];
        if (fd != -1) {
            (void)write(fd, data.data(), data.size());
        }
    }

    void close_stdin() {
        if (m_pty_master != -1) {
        } else if (m_stdin_pipe[1] != -1) {
            close(m_stdin_pipe[1]);
            m_stdin_pipe[1] = -1;
        }
    }

    void kill_process(int sig = SIGTERM) {
        if (m_pid > 0) {
            kill(m_pid, sig);
        }
    }

    void resize_terminal(int cols, int rows) {
#if defined(__linux__) || defined(__APPLE__)
        if (m_pty_master != -1) {
            struct winsize ws = { (unsigned short)rows, (unsigned short)cols, 0, 0 };
            ioctl(m_pty_master, TIOCSWINSZ, &ws);
        }
#endif
    }

    pid_t get_pid() const { return m_pid; }

private:
    void read_loop(int fd, DataCallback cb) {
        std::vector<char> buffer(4096);
        while (m_running) {
            ssize_t n = read(fd, buffer.data(), buffer.size());
            if (n > 0) {
                cb(std::vector<char>(buffer.begin(), buffer.begin() + n));
            } else if (n == 0) {
                break;
            } else {
                if (errno == EINTR) continue;
                break;
            }
        }
    }

    void wait_loop(ExitCallback cb) {
        int status;
        waitpid(m_pid, &status, 0);
        m_running = false;

        ResourceUsage usage = {0, {0, 0}};
        struct rusage r_usage;
        if (getrusage(RUSAGE_CHILDREN, &r_usage) == 0) {
            usage.maxRSS = r_usage.ru_maxrss * 1024;
            usage.cpuTime.user = r_usage.ru_utime.tv_sec * 1000000 + r_usage.ru_utime.tv_usec;
            usage.cpuTime.system = r_usage.ru_stime.tv_sec * 1000000 + r_usage.ru_stime.tv_usec;
        }

        if (WIFEXITED(status)) {
            cb(WEXITSTATUS(status), usage);
        } else if (WIFSIGNALED(status)) {
            cb(-WTERMSIG(status), usage);
        } else {
            cb(-1, usage);
        }
    }

    void close_pipes() {
        for (int i = 0; i < 2; ++i) {
            if (m_stdin_pipe[i] != -1) { close(m_stdin_pipe[i]); m_stdin_pipe[i] = -1; }
            if (m_stdout_pipe[i] != -1) { close(m_stdout_pipe[i]); m_stdout_pipe[i] = -1; }
            if (m_stderr_pipe[i] != -1) { close(m_stderr_pipe[i]); m_stderr_pipe[i] = -1; }
        }
    }

    pid_t m_pid;
    int m_stdin_pipe[2];
    int m_stdout_pipe[2];
    int m_stderr_pipe[2];
    int m_pty_master;
    std::atomic<bool> m_running;
    std::thread m_stdout_thread;
    std::thread m_stderr_thread;
    std::thread m_exit_thread;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOY_PROCESS_HH
