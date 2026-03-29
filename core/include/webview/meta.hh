#ifndef WEBVIEW_META_HH
#define WEBVIEW_META_HH

#include "webview.h"
#include "detail/json.hh"
#include "detail/base64.hh"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#if defined(__linux__)
#include <pty.h>
#include <utmp.h>
#elif defined(__APPLE__)
#include <util.h>
#endif

#include <termios.h>
#include <sys/ioctl.h>
#include <spawn.h>

#ifdef WEBVIEW_GTK
#include <gtk/gtk.h>
#endif

extern char **environ;

namespace webview {
namespace meta {

struct ProcessInfo {
    std::string handle;
    pid_t pid = -1;
    int stdin_fd = -1;
    int stdout_fd = -1;
    int stderr_fd = -1;
    int pty_master = -1;
    bool exited = false;
    int exit_code = -1;
    int signal_code = -1;

#ifdef WEBVIEW_GTK
    unsigned int stdout_watch = 0;
    unsigned int stderr_watch = 0;
    unsigned int child_watch = 0;
#endif
};

class SubprocessManager : public std::enable_shared_from_this<SubprocessManager> {
public:
    SubprocessManager(::webview::webview* w) : m_webview(w) {}
    SubprocessManager() : m_webview(nullptr) {}

    ~SubprocessManager() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_processes) {
            auto info = pair.second;
            if (!info->exited && info->pid > 0) {
                kill(info->pid, SIGTERM);
            }
            cleanup_monitoring(info);
        }
    }

    void bind(::webview::webview& w) {
        m_webview = &w;
        auto self = shared_from_this();

        w.bind("__meta_spawn", [self](const std::string& id, const std::string& req, void*) {
            std::string handle = ::webview::detail::json_parse(req, "", 0);
            std::string cmd_json = ::webview::detail::json_parse(req, "", 1);
            std::string opts_json = ::webview::detail::json_parse(req, "", 2);

            std::vector<std::string> cmd;
            for (int i = 0; ; ++i) {
                std::string s = ::webview::detail::json_parse(cmd_json, "", i);
                if (s.empty()) break;
                cmd.push_back(s);
            }

            std::string res = self->spawn(handle, cmd, opts_json);
            self->m_webview->resolve(id, 0, res);
        }, nullptr);

        w.bind("__meta_spawnSync", [self](const std::string& id, const std::string& req, void*) {
            std::string cmd_json = ::webview::detail::json_parse(req, "", 0);
            std::string opts_json = ::webview::detail::json_parse(req, "", 1);

            std::vector<std::string> cmd;
            for (int i = 0; ; ++i) {
                std::string s = ::webview::detail::json_parse(cmd_json, "", i);
                if (s.empty()) break;
                cmd.push_back(s);
            }

            std::string res = self->spawnSync(cmd, opts_json);
            self->m_webview->resolve(id, 0, res);
        }, nullptr);

        w.bind("__meta_write", [self](const std::string& req) -> std::string {
            std::string handle = ::webview::detail::json_parse(req, "", 0);
            std::string data_b64 = ::webview::detail::json_parse(req, "", 1);
            if (!handle.empty()) self->writeStdin(handle, ::webview::detail::base64_decode(data_b64));
            return "";
        });

        w.bind("__meta_closeStdin", [self](const std::string& req) -> std::string {
            std::string handle = ::webview::detail::json_parse(req, "", 0);
            if (!handle.empty()) self->closeStdin(handle);
            return "";
        });

        w.bind("__meta_kill", [self](const std::string& req) -> std::string {
            std::string handle = ::webview::detail::json_parse(req, "", 0);
            std::string sig_str = ::webview::detail::json_parse(req, "", 1);
            if (!handle.empty()) {
                int sig = SIGTERM;
                if (sig_str == "SIGKILL" || sig_str == "9") sig = SIGKILL;
                self->killProcess(handle, sig);
            }
            return "";
        });

        w.bind("__meta_resize", [self](const std::string& req) -> std::string {
            std::string handle = ::webview::detail::json_parse(req, "", 0);
            std::string cols_str = ::webview::detail::json_parse(req, "", 1);
            std::string rows_str = ::webview::detail::json_parse(req, "", 2);
            if (!handle.empty() && !cols_str.empty() && !rows_str.empty()) {
                self->resizeTerminal(handle, std::stoi(cols_str), std::stoi(rows_str));
            }
            return "";
        });

        w.bind("__meta_cleanup", [self](const std::string& req) -> std::string {
            std::string handle = ::webview::detail::json_parse(req, "", 0);
            if (!handle.empty()) self->cleanup(handle);
            return "";
        });
    }

    std::string spawn(const std::string& handle, const std::vector<std::string>& cmd, const std::string& options_json) {
        bool terminal = ::webview::detail::json_parse(options_json, "terminal", -1) != "";
        std::string cwd = ::webview::detail::json_parse(options_json, "cwd", -1);

        auto info = std::make_shared<ProcessInfo>();
        info->handle = handle;

        pid_t pid;
        int stdin_fd = -1, stdout_fd = -1, stderr_fd = -1, pty_master = -1;

        if (terminal) {
            pid = forkpty(&pty_master, NULL, NULL, NULL);
            if (pid < 0) return "{\"error\": \"forkpty failed\"}";
            if (pid == 0) {
                if (!cwd.empty()) { if(chdir(cwd.c_str())){}}
                std::vector<char*> argv;
                for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
                argv.push_back(nullptr);
                execvp(argv[0], argv.data());
                _exit(1);
            }
            stdin_fd = stdout_fd = pty_master;
            fcntl(pty_master, F_SETFL, fcntl(pty_master, F_GETFL) | O_NONBLOCK);
        } else {
            int in_pipe[2], out_pipe[2], err_pipe[2];
            if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0 || pipe(err_pipe) < 0) return "{\"error\": \"pipe failed\"}";

            pid = fork();
            if (pid < 0) return "{\"error\": \"fork failed\"}";
            if (pid == 0) {
                if (!cwd.empty()) { if(chdir(cwd.c_str())){}}
                dup2(in_pipe[0], STDIN_FILENO);
                dup2(out_pipe[1], STDOUT_FILENO);
                dup2(err_pipe[1], STDERR_FILENO);
                close(in_pipe[1]); close(out_pipe[0]); close(err_pipe[0]);
                std::vector<char*> argv;
                for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
                argv.push_back(nullptr);
                execvp(argv[0], argv.data());
                _exit(1);
            }

            close(in_pipe[0]); close(out_pipe[1]); close(err_pipe[1]);
            stdin_fd = in_pipe[1];
            stdout_fd = out_pipe[0];
            stderr_fd = err_pipe[0];
            fcntl(stdout_fd, F_SETFL, fcntl(stdout_fd, F_GETFL) | O_NONBLOCK);
            fcntl(stderr_fd, F_SETFL, fcntl(stderr_fd, F_GETFL) | O_NONBLOCK);
        }

        info->pid = pid;
        info->stdin_fd = stdin_fd;
        info->stdout_fd = stdout_fd;
        info->stderr_fd = stderr_fd;
        info->pty_master = pty_master;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_processes[handle] = info;
        }

        setup_monitoring(info, terminal);
        return "{\"pid\": " + std::to_string(pid) + "}";
    }

    std::string spawnSync(const std::vector<std::string>& cmd, const std::string& options_json) {
        std::string cwd = ::webview::detail::json_parse(options_json, "cwd", -1);
        int out_pipe[2], err_pipe[2];
        if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) return "{\"success\": false, \"error\": \"pipe failed\"}";

        pid_t pid = fork();
        if (pid < 0) return "{\"success\": false, \"error\": \"fork failed\"}";
        if (pid == 0) {
            if (!cwd.empty()) { if(chdir(cwd.c_str())){}}
            dup2(out_pipe[1], STDOUT_FILENO);
            dup2(err_pipe[1], STDERR_FILENO);
            close(out_pipe[0]); close(err_pipe[0]);
            std::vector<char*> argv;
            for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            _exit(1);
        }

        close(out_pipe[1]); close(err_pipe[1]);
        std::string stdout_str, stderr_str;
        char buffer[4096];
        ssize_t n;
        while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0) stdout_str.append(buffer, n);
        while ((n = read(err_pipe[0], buffer, sizeof(buffer))) > 0) stderr_str.append(buffer, n);
        close(out_pipe[0]); close(err_pipe[0]);

        int status;
        waitpid(pid, &status, 0);
        bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        return "{\"success\": " + std::string(success ? "true" : "false") +
               ", \"exitCode\": " + std::to_string(WIFEXITED(status) ? WEXITSTATUS(status) : -1) +
               ", \"stdout\": \""+ ::webview::detail::base64_encode(stdout_str) + "\"" +
               ", \"stderr\": \""+ ::webview::detail::base64_encode(stderr_str) + "\"" +
               ", \"pid\": " + std::to_string(pid) + "}";
    }

    void writeStdin(const std::string& handle, const std::string& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(handle);
        if (it != m_processes.end() && it->second->stdin_fd != -1) {
            ssize_t n = write(it->second->stdin_fd, data.c_str(), data.size());
            (void)n;
        }
    }

    void closeStdin(const std::string& handle) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(handle);
        if (it != m_processes.end() && it->second->stdin_fd != -1) {
            if (it->second->pty_master == -1) {
                close(it->second->stdin_fd);
                it->second->stdin_fd = -1;
            }
        }
    }

    void killProcess(const std::string& handle, int sig) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(handle);
        if (it != m_processes.end() && it->second->pid > 0) {
            kill(it->second->pid, sig);
        }
    }

    void resizeTerminal(const std::string& handle, int cols, int rows) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(handle);
        if (it != m_processes.end() && it->second->pty_master != -1) {
            struct winsize ws;
            ws.ws_col = static_cast<unsigned short>(cols);
            ws.ws_row = static_cast<unsigned short>(rows);
            ioctl(it->second->pty_master, TIOCSWINSZ, &ws);
        }
    }

    void cleanup(const std::string& handle) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(handle);
        if (it != m_processes.end()) {
            cleanup_monitoring(it->second);
            m_processes.erase(it);
        }
    }

private:
    struct WatchData {
        std::weak_ptr<SubprocessManager> manager;
        std::string handle;
        bool is_stderr;
    };

#ifdef WEBVIEW_GTK
    static gboolean on_io_ready(GIOChannel* source, GIOCondition condition, gpointer user_data) {
        auto* data = static_cast<WatchData*>(user_data);
        char buffer[4096];
        gsize bytes_read;
        GIOStatus status = g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, NULL);

        if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
            std::string s(buffer, bytes_read);
            auto mgr = data->manager.lock();
            if (mgr && mgr->m_webview) {
                std::string handle = data->handle;
                bool is_stderr = data->is_stderr;
                mgr->m_webview->dispatch([mgr, handle, is_stderr, s] {
                    std::lock_guard<std::mutex> lock(mgr->m_mutex);
                    auto it = mgr->m_processes.find(handle);
                    if (it != mgr->m_processes.end()) {
                        std::string type = it->second->pty_master != -1 ? "terminal" : (is_stderr ? "stderr" : "stdout");
                        std::string js = "window.meta._onData(" + ::webview::detail::json_escape(handle) + ", " +
                                         ::webview::detail::json_escape(type) + ", " +
                                         ::webview::detail::json_escape(::webview::detail::base64_encode(s)) + ")";
                        mgr->m_webview->eval(js);
                    }
                });
            }
        }

        if (status == G_IO_STATUS_EOF || condition & (G_IO_HUP | G_IO_ERR)) return FALSE;
        return TRUE;
    }

    static void on_child_exit(GPid pid, gint status, gpointer user_data) {
        auto* data = static_cast<WatchData*>(user_data);
        auto mgr = data->manager.lock();
        if (mgr && mgr->m_webview) {
            std::string handle = data->handle;
            mgr->m_webview->dispatch([mgr, handle, status] {
                std::lock_guard<std::mutex> lock(mgr->m_mutex);
                auto it = mgr->m_processes.find(handle);
                if (it != mgr->m_processes.end()) {
                    it->second->exited = true;
                    it->second->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                    it->second->signal_code = WIFSIGNALED(status) ? WTERMSIG(status) : -1;
                    std::string js = "window.meta._onExit(" + ::webview::detail::json_escape(handle) + ", " +
                                     std::to_string(it->second->exit_code) + ", " +
                                     std::to_string(it->second->signal_code) + ")";
                    mgr->m_webview->eval(js);
                }
            });
        }
        g_spawn_close_pid(pid);
    }
#endif

    void setup_monitoring(std::shared_ptr<ProcessInfo> info, bool terminal) {
#ifdef WEBVIEW_GTK
        auto self = shared_from_this();
        auto* data_out = new WatchData{self, info->handle, false};
        GIOChannel* chan_out = g_io_channel_unix_new(info->stdout_fd);
        g_io_channel_set_encoding(chan_out, NULL, NULL);
        info->stdout_watch = g_io_add_watch_full(chan_out, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR),
                                                on_io_ready, data_out, [](gpointer p) { delete static_cast<WatchData*>(p); });
        g_io_channel_unref(chan_out);

        if (!terminal) {
            auto* data_err = new WatchData{self, info->handle, true};
            GIOChannel* chan_err = g_io_channel_unix_new(info->stderr_fd);
            info->stderr_watch = g_io_add_watch_full(chan_err, G_PRIORITY_DEFAULT, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR),
                                                    on_io_ready, data_err, [](gpointer p) { delete static_cast<WatchData*>(p); });
            g_io_channel_unref(chan_err);
        }

        auto* data_exit = new WatchData{self, info->handle, false};
        info->child_watch = g_child_watch_add_full(G_PRIORITY_DEFAULT, info->pid, on_child_exit, data_exit, [](gpointer p) { delete static_cast<WatchData*>(p); });
#endif
    }

    void cleanup_monitoring(std::shared_ptr<ProcessInfo> info) {
#ifdef WEBVIEW_GTK
        if (info->stdout_watch) g_source_remove(info->stdout_watch);
        if (info->stderr_watch) g_source_remove(info->stderr_watch);
        if (info->child_watch) g_source_remove(info->child_watch);
#endif
        if (info->stdin_fd != -1) close(info->stdin_fd);
        if (info->stdout_fd != -1 && info->stdout_fd != info->stdin_fd) close(info->stdout_fd);
        if (info->stderr_fd != -1) close(info->stderr_fd);
    }

    ::webview::webview* m_webview;
    std::map<std::string, std::shared_ptr<ProcessInfo>> m_processes;
    std::mutex m_mutex;
};

} // namespace meta
} // namespace webview

#else
namespace webview {
namespace meta {
class SubprocessManager : public std::enable_shared_from_this<SubprocessManager> {
public:
    SubprocessManager(::webview::webview*) {}
    SubprocessManager() {}
    void bind(::webview::webview&) {}
    std::string spawn(const std::string&, const std::vector<std::string>&, const std::string&) { return "{\"error\": \"Not implemented\"}"; }
    std::string spawnSync(const std::vector<std::string>&, const std::string&) { return "{\"success\": false}"; }
    void writeStdin(const std::string&, const std::string&) {}
    void closeStdin(const std::string&) {}
    void killProcess(const std::string&, int) {}
    void resizeTerminal(const std::string&, int, int) {}
    void cleanup(const std::string&) {}
};
}
}
#endif

#endif // WEBVIEW_META_HH
