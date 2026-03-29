#ifndef WEBVIEW_META_HH
#define WEBVIEW_META_HH

#include "webview.h"
#include "detail/json.hh"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <pty.h>
#include <utmp.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include <iostream>

namespace webview {
namespace meta {

struct ProcessInfo {
    pid_t pid;
    int stdin_fd = -1;
    int stdout_fd = -1;
    int stderr_fd = -1;
    int pty_master = -1;
    bool exited = false;
    int exit_code = -1;
    int signal_code = -1;

    // Monitoring state
    guint stdout_watch = 0;
    guint stderr_watch = 0;
    guint child_watch = 0;
};

class SubprocessManager {
public:
    SubprocessManager(::webview::webview* w) : m_webview(w) {}

    SubprocessManager() : m_webview(nullptr) {}

    ~SubprocessManager() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_processes) {
            auto info = pair.second;
            if (!info->exited) {
                kill(info->pid, SIGTERM);
            }
            cleanup_monitoring(info);
        }
    }

    std::string spawn(const std::vector<std::string>& cmd, const std::string& options_json) {
        bool terminal = ::webview::detail::json_parse(options_json, "terminal", 0) != "";

        int in_pipe[2], out_pipe[2], err_pipe[2];
        if (!terminal) {
            if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
                return "{\"error\": \"pipe failed\"}";
            }
        }

        pid_t pid;
        int master;
        if (terminal) {
            pid = forkpty(&master, NULL, NULL, NULL);
        } else {
            pid = fork();
        }

        if (pid < 0) {
            return "{\"error\": \"fork failed\"}";
        }

        if (pid == 0) {
            if (!terminal) {
                dup2(in_pipe[0], STDIN_FILENO);
                dup2(out_pipe[1], STDOUT_FILENO);
                dup2(err_pipe[1], STDERR_FILENO);
                close(in_pipe[1]);
                close(out_pipe[0]);
                close(err_pipe[0]);
                close(in_pipe[0]);
                close(out_pipe[1]);
                close(err_pipe[1]);
            }

            std::vector<char*> argv;
            for (const auto& s : cmd) {
                argv.push_back(const_cast<char*>(s.c_str()));
            }
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            exit(1);
        }

        auto info = std::make_shared<ProcessInfo>();
        info->pid = pid;
        if (terminal) {
            info->pty_master = master;
            info->stdin_fd = master;
            info->stdout_fd = master;
            fcntl(master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);
        } else {
            close(in_pipe[0]);
            close(out_pipe[1]);
            close(err_pipe[1]);
            info->stdin_fd = in_pipe[1];
            info->stdout_fd = out_pipe[0];
            info->stderr_fd = err_pipe[0];
            fcntl(info->stdout_fd, F_SETFL, fcntl(info->stdout_fd, F_GETFL) | O_NONBLOCK);
            fcntl(info->stderr_fd, F_SETFL, fcntl(info->stderr_fd, F_GETFL) | O_NONBLOCK);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_processes[pid] = info;
        }

        setup_monitoring(info, terminal);

        return "{\"pid\": " + std::to_string(pid) + "}";
    }

    std::string spawnSync(const std::vector<std::string>& cmd, const std::string& /*options_json*/) {
        int out_pipe[2], err_pipe[2];
        if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
            return "{\"success\": false, \"error\": \"pipe failed\"}";
        }

        pid_t pid = fork();
        if (pid < 0) {
            return "{\"success\": false, \"error\": \"fork failed\"}";
        }

        if (pid == 0) {
            dup2(out_pipe[1], STDOUT_FILENO);
            dup2(err_pipe[1], STDERR_FILENO);
            close(out_pipe[0]);
            close(err_pipe[0]);

            std::vector<char*> argv;
            for (const auto& s : cmd) {
                argv.push_back(const_cast<char*>(s.c_str()));
            }
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            exit(1);
        }

        close(out_pipe[1]);
        close(err_pipe[1]);

        std::string stdout_str, stderr_str;
        char buffer[4096];
        ssize_t n;
        while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0) {
            stdout_str.append(buffer, n);
        }
        while ((n = read(err_pipe[0], buffer, sizeof(buffer))) > 0) {
            stderr_str.append(buffer, n);
        }
        close(out_pipe[0]);
        close(err_pipe[0]);

        int status;
        waitpid(pid, &status, 0);

        bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

        std::string res = "{";
        res += "\"success\": " + std::string(success ? "true" : "false") + ",";
        res += "\"exitCode\": " + std::to_string(exit_code) + ",";
        res += "\"stdout\": " + ::webview::detail::json_escape(stdout_str) + ",";
        res += "\"stderr\": " + ::webview::detail::json_escape(stderr_str) + ",";
        res += "\"pid\": " + std::to_string(pid);
        res += "}";
        return res;
    }

    void writeStdin(int pid, const std::string& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(pid);
        if (it != m_processes.end() && it->second->stdin_fd != -1) {
            ssize_t n = write(it->second->stdin_fd, data.c_str(), data.size());
            (void)n;
        }
    }

    void closeStdin(int pid) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(pid);
        if (it != m_processes.end() && it->second->stdin_fd != -1) {
            if (it->second->pty_master == -1) {
                close(it->second->stdin_fd);
                it->second->stdin_fd = -1;
            }
        }
    }

    void killProcess(int pid, int sig) {
        kill(pid, sig);
    }

    void resizeTerminal(int pid, int cols, int rows) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(pid);
        if (it != m_processes.end() && it->second->pty_master != -1) {
            struct winsize ws;
            ws.ws_col = static_cast<unsigned short>(cols);
            ws.ws_row = static_cast<unsigned short>(rows);
            ioctl(it->second->pty_master, TIOCSWINSZ, &ws);
        }
    }

private:
    struct WatchData {
        SubprocessManager* manager;
        std::shared_ptr<ProcessInfo> info;
        bool is_stderr;
    };

    static gboolean on_io_ready(GIOChannel* source, GIOCondition condition, gpointer user_data) {
        auto* data = static_cast<WatchData*>(user_data);
        char buffer[4096];
        gsize bytes_read;
        GError* error = NULL;
        GIOStatus status = g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, &error);

        if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
            std::string s(buffer, bytes_read);
            std::string type = data->info->pty_master != -1 ? "terminal" : (data->is_stderr ? "stderr" : "stdout");
            std::string js = "window.meta._onData(" + std::to_string(data->info->pid) + ", " +
                             ::webview::detail::json_escape(type) + ", " +
                             ::webview::detail::json_escape(s) + ")";
            data->manager->m_webview->dispatch([=] {
                data->manager->m_webview->eval(js);
            });
        }

        if (status == G_IO_STATUS_EOF || condition & (G_IO_HUP | G_IO_ERR)) {
            return FALSE;
        }
        return TRUE;
    }

    static void on_child_exit(GPid pid, gint status, gpointer user_data) {
        auto* data = static_cast<WatchData*>(user_data);
        data->info->exited = true;
        data->info->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        data->info->signal_code = WIFSIGNALED(status) ? WTERMSIG(status) : -1;

        std::string js = "window.meta._onExit(" + std::to_string(data->info->pid) + ", " +
                         std::to_string(data->info->exit_code) + ", " +
                         std::to_string(data->info->signal_code) + ")";
        data->manager->m_webview->dispatch([=] {
            data->manager->m_webview->eval(js);
        });
        g_spawn_close_pid(pid);
    }

    void setup_monitoring(std::shared_ptr<ProcessInfo> info, bool terminal) {
        auto* data_out = new WatchData{this, info, false};
        GIOChannel* chan_out = g_io_channel_unix_new(info->stdout_fd);
        g_io_channel_set_encoding(chan_out, NULL, NULL);
        info->stdout_watch = g_io_add_watch_full(chan_out, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR),
                                                on_io_ready, data_out, [](gpointer p) { delete static_cast<WatchData*>(p); });
        g_io_channel_unref(chan_out);

        if (!terminal) {
            auto* data_err = new WatchData{this, info, true};
            GIOChannel* chan_err = g_io_channel_unix_new(info->stderr_fd);
            g_io_channel_set_encoding(chan_err, NULL, NULL);
            info->stderr_watch = g_io_add_watch_full(chan_err, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR),
                                                    on_io_ready, data_err, [](gpointer p) { delete static_cast<WatchData*>(p); });
            g_io_channel_unref(chan_err);
        }

        auto* data_exit = new WatchData{this, info, false};
        info->child_watch = g_child_watch_add_full(G_PRIORITY_DEFAULT, info->pid, on_child_exit, data_exit, [](gpointer p) { delete static_cast<WatchData*>(p); });
    }

    void cleanup_monitoring(std::shared_ptr<ProcessInfo> info) {
        if (info->stdout_watch) g_source_remove(info->stdout_watch);
        if (info->stderr_watch) g_source_remove(info->stderr_watch);
        if (info->child_watch) g_source_remove(info->child_watch);
        if (info->stdin_fd != -1) close(info->stdin_fd);
        if (info->stdout_fd != -1 && info->stdout_fd != info->stdin_fd) close(info->stdout_fd);
        if (info->stderr_fd != -1) close(info->stderr_fd);
    }

    ::webview::webview* m_webview;
    std::map<int, std::shared_ptr<ProcessInfo>> m_processes;
    std::mutex m_mutex;
};

} // namespace meta
} // namespace webview

#endif // WEBVIEW_META_HH
