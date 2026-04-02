#ifndef WEBVIEW_META_HH
#define WEBVIEW_META_HH

#include "webview.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <iostream>

#ifndef _WIN32
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glib.h>
#include <glib-unix.h>

#if defined(__linux__) || defined(__APPLE__)
#include <pty.h>
#include <utmp.h>
#endif
#endif

namespace webview {

class Subprocess {
public:
    int pid = -1;
    int stdin_fd = -1;
    int stdout_fd = -1;
    int stderr_fd = -1;
    int terminal_fd = -1;
    bool killed = false;
    int exit_code = -1;
    std::string signal_code;

#ifndef _WIN32
    guint stdout_watch = 0;
    guint stderr_watch = 0;
    guint terminal_watch = 0;
    guint child_watch = 0;
#endif

    Subprocess(int p) : pid(p) {}
    ~Subprocess() {
#ifndef _WIN32
        if (stdout_watch) g_source_remove(stdout_watch);
        if (stderr_watch) g_source_remove(stderr_watch);
        if (terminal_watch) g_source_remove(terminal_watch);
        if (child_watch) g_source_remove(child_watch);

        if (stdin_fd != -1) close(stdin_fd);
        if (stdout_fd != -1) close(stdout_fd);
        if (stderr_fd != -1) close(stderr_fd);
        if (terminal_fd != -1) close(terminal_fd);
#endif
    }
};

class MetaRuntime {
public:
    MetaRuntime(detail::engine_base& w) : m_webview(w) {}

    std::string spawn(const std::vector<std::string>& cmd, const std::string& options_json) {
#ifdef _WIN32
        (void)cmd; (void)options_json;
        return "{\"error\":\"spawn not implemented on Windows\"}";
#else
        std::string use_terminal_str = detail::json_parse(options_json, "terminal", 0);
        bool use_terminal = !use_terminal_str.empty() && use_terminal_str != "null";

        if (use_terminal) {
            int master;
            pid_t pid = forkpty(&master, nullptr, nullptr, nullptr);
            if (pid == 0) { // Child
                std::vector<char*> argv;
                for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
                argv.push_back(nullptr);
                execvp(argv[0], argv.data());
                _exit(1);
            } else if (pid > 0) { // Parent
                auto proc = std::make_shared<Subprocess>(pid);
                proc->terminal_fd = master;
                m_subprocesses[pid] = proc;
                setup_async_io(proc);
                setup_child_watch(proc);
                return "{\"pid\":" + std::to_string(pid) + "}";
            }
        } else {
            int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];
            if (pipe(pipe_stdin) < 0 || pipe(pipe_stdout) < 0 || pipe(pipe_stderr) < 0) {
                return "{\"error\":\"pipe failed\"}";
            }

            pid_t pid = fork();
            if (pid == 0) { // Child
                dup2(pipe_stdin[0], STDIN_FILENO);
                dup2(pipe_stdout[1], STDOUT_FILENO);
                dup2(pipe_stderr[1], STDERR_FILENO);

                close(pipe_stdin[0]); close(pipe_stdin[1]);
                close(pipe_stdout[0]); close(pipe_stdout[1]);
                close(pipe_stderr[0]); close(pipe_stderr[1]);

                std::vector<char*> argv;
                for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
                argv.push_back(nullptr);

                execvp(argv[0], argv.data());
                _exit(1);
            } else if (pid > 0) { // Parent
                close(pipe_stdin[0]);
                close(pipe_stdout[1]);
                close(pipe_stderr[1]);

                auto proc = std::make_shared<Subprocess>(pid);
                proc->stdin_fd = pipe_stdin[1];
                proc->stdout_fd = pipe_stdout[0];
                proc->stderr_fd = pipe_stderr[0];

                m_subprocesses[pid] = proc;

                setup_async_io(proc);
                setup_child_watch(proc);

                return "{\"pid\":" + std::to_string(pid) + "}";
            }
        }
        return "{\"error\":\"fork failed\"}";
#endif
    }

    std::string spawnSync(const std::vector<std::string>& cmd, const std::string& /*options_json*/) {
#ifdef _WIN32
        (void)cmd;
        return "{\"error\":\"spawnSync not implemented on Windows\"}";
#else
        int pipe_stdout[2], pipe_stderr[2];
        if (pipe(pipe_stdout) < 0 || pipe(pipe_stderr) < 0) return "{\"error\":\"pipe failed\"}";

        pid_t pid = fork();
        if (pid == 0) {
            dup2(pipe_stdout[1], STDOUT_FILENO);
            dup2(pipe_stderr[1], STDERR_FILENO);
            close(pipe_stdout[0]); close(pipe_stdout[1]);
            close(pipe_stderr[0]); close(pipe_stderr[1]);

            std::vector<char*> argv;
            for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            _exit(1);
        }

        close(pipe_stdout[1]); close(pipe_stderr[1]);

        std::string out_str, err_str;
        char buf[4096];
        ssize_t n;
        while ((n = read(pipe_stdout[0], buf, sizeof(buf))) > 0) out_str.append(buf, n);
        while ((n = read(pipe_stderr[0], buf, sizeof(buf))) > 0) err_str.append(buf, n);

        int status;
        waitpid(pid, &status, 0);

        close(pipe_stdout[0]); close(pipe_stderr[0]);
        int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

        return "{\"pid\":" + std::to_string(pid) +
               ",\"exitCode\":" + std::to_string(exitCode) +
               ",\"stdout\":" + detail::json_escape(out_str) +
               ",\"stderr\":" + detail::json_escape(err_str) + "}";
#endif
    }

#ifndef _WIN32
    struct IOContext {
        MetaRuntime* runtime;
        std::weak_ptr<Subprocess> proc;
        std::string stream;
    };

    void setup_async_io(std::shared_ptr<Subprocess> proc) {
        auto io_callback = [](GIOChannel *source, GIOCondition /*condition*/, gpointer data) -> gboolean {
            auto ctx = static_cast<IOContext*>(data);
            auto proc_ptr = ctx->proc.lock();
            if (!proc_ptr) return FALSE;

            char buf[4096];
            gsize bytes_read;
            GError *error = nullptr;
            GIOStatus status = g_io_channel_read_chars(source, buf, sizeof(buf), &bytes_read, &error);

            if (bytes_read > 0) {
                std::string data_str(buf, bytes_read);
                ctx->runtime->m_webview.eval("window.meta.__onData(" + std::to_string(proc_ptr->pid) + ", '" + ctx->stream + "', " + detail::json_escape(data_str) + ")");
            }

            if (status == G_IO_STATUS_EOF || status == G_IO_STATUS_ERROR) {
                if (error) g_error_free(error);
                return FALSE;
            }
            return TRUE;
        };

        if (proc->stdout_fd != -1) {
            GIOChannel* channel = g_io_channel_unix_new(proc->stdout_fd);
            g_io_channel_set_encoding(channel, nullptr, nullptr);
            proc->stdout_watch = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR), io_callback, new IOContext{this, proc, "stdout"}, [](gpointer d){ delete static_cast<IOContext*>(d); });
            g_io_channel_unref(channel);
        }
        if (proc->stderr_fd != -1) {
            GIOChannel* channel = g_io_channel_unix_new(proc->stderr_fd);
            g_io_channel_set_encoding(channel, nullptr, nullptr);
            proc->stderr_watch = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR), io_callback, new IOContext{this, proc, "stderr"}, [](gpointer d){ delete static_cast<IOContext*>(d); });
            g_io_channel_unref(channel);
        }
        if (proc->terminal_fd != -1) {
            GIOChannel* channel = g_io_channel_unix_new(proc->terminal_fd);
            g_io_channel_set_encoding(channel, nullptr, nullptr);
            proc->terminal_watch = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR), io_callback, new IOContext{this, proc, "terminal"}, [](gpointer d){ delete static_cast<IOContext*>(d); });
            g_io_channel_unref(channel);
        }
    }

    void setup_child_watch(std::shared_ptr<Subprocess> proc) {
        auto watch_callback = [](GPid pid, gint status, gpointer data) {
            auto p = static_cast<std::pair<MetaRuntime*, std::weak_ptr<Subprocess>>*>(data);
            auto proc_ptr = p->second.lock();
            if (proc_ptr) {
                proc_ptr->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                p->first->m_webview.eval("window.meta.__onExit(" + std::to_string(pid) + ", " + std::to_string(proc_ptr->exit_code) + ")");
            }
            g_spawn_close_pid(pid);
        };
        proc->child_watch = g_child_watch_add_full(G_PRIORITY_DEFAULT, proc->pid, watch_callback, new std::pair<MetaRuntime*, std::weak_ptr<Subprocess>>(this, proc), [](gpointer d){ delete static_cast<std::pair<MetaRuntime*, std::weak_ptr<Subprocess>>*>(d); });
    }
#endif

    void write_to_stdin(int pid, const std::string& data) {
#ifndef _WIN32
        auto it = m_subprocesses.find(pid);
        if (it != m_subprocesses.end()) {
            int fd = it->second->terminal_fd != -1 ? it->second->terminal_fd : it->second->stdin_fd;
            if (fd != -1) {
                if (write(fd, data.c_str(), data.size())) {}
            }
        }
#else
        (void)pid; (void)data;
#endif
    }

    void kill_process(int pid, int sig) {
#ifndef _WIN32
        kill(pid, sig);
#else
        (void)pid; (void)sig;
#endif
    }

private:
    detail::engine_base& m_webview;
    std::map<int, std::shared_ptr<Subprocess>> m_subprocesses;
};

} // namespace webview

#endif // WEBVIEW_META_HH
