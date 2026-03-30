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
#include <fstream>
#include <sstream>

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
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif

#include <termios.h>
#include <sys/ioctl.h>
#include <spawn.h>

#ifdef WEBVIEW_GTK
#include <gtk/gtk.h>
#endif

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

#ifdef WEBVIEW_GTK
struct WidgetInfo {
    std::string handle;
    std::string type;
    GtkWidget* widget;
};
#endif

class SubprocessManager : public std::enable_shared_from_this<SubprocessManager> {
public:
    SubprocessManager(::webview::webview* w) : m_webview(w) {}
    SubprocessManager() : m_webview(nullptr) {}

    ~SubprocessManager() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_processes) {
            auto info = pair.second;
            if (!info->exited && info->pid > 0) kill(info->pid, SIGTERM);
            cleanup_monitoring(info);
        }
#ifdef WEBVIEW_GTK
        for (auto& pair : m_widgets) {
            if (pair.second.type == "Window" && GTK_IS_WINDOW(pair.second.widget)) {
                gtk_window_close(GTK_WINDOW(pair.second.widget));
            }
        }
#endif
    }

    void bind(::webview::webview& w) {
        m_webview = &w;
        auto self = shared_from_this();

        w.bind("__alloy_spawn", [self](const std::string& id, const std::string& req, void*) {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string cmd_j = ::webview::detail::json_parse(req, "", 1);
            std::string opt_j = ::webview::detail::json_parse(req, "", 2);
            self->m_webview->resolve(id, 0, self->spawn(h, parse_array(cmd_j), opt_j));
        }, nullptr);

        w.bind("__alloy_spawnSync", [self](const std::string& id, const std::string& req, void*) {
            std::string cmd_j = ::webview::detail::json_parse(req, "", 0);
            std::string opt_j = ::webview::detail::json_parse(req, "", 1);
            self->m_webview->resolve(id, 0, self->spawnSync(parse_array(cmd_j), opt_j));
        }, nullptr);

        w.bind("__alloy_write", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string d = ::webview::detail::json_parse(req, "", 1);
            if (!h.empty()) self->writeStdin(h, ::webview::detail::base64_decode(d));
            return "";
        });

        w.bind("__alloy_closeStdin", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            if (!h.empty()) self->closeStdin(h);
            return "";
        });

        w.bind("__alloy_kill", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string s = ::webview::detail::json_parse(req, "", 1);
            if (!h.empty()) {
                int sig = SIGTERM;
                if (s == "SIGKILL" || s == "9") sig = SIGKILL;
                self->killProcess(h, sig);
            }
            return "";
        });

        w.bind("__alloy_resize", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string c = ::webview::detail::json_parse(req, "", 1);
            std::string r = ::webview::detail::json_parse(req, "", 2);
            if (!h.empty()) self->resizeTerminal(h, std::stoi(c), std::stoi(r));
            return "";
        });

        w.bind("__alloy_cleanup", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            if (!h.empty()) self->cleanup(h);
            return "";
        });

        w.bind("__alloy_cron_register", [self](const std::string& id, const std::string& req, void*) {
            std::string p = ::webview::detail::json_parse(req, "", 0);
            std::string s = ::webview::detail::json_parse(req, "", 1);
            std::string t = ::webview::detail::json_parse(req, "", 2);
            self->m_webview->resolve(id, 0, self->registerCronJob(p, s, t));
        }, nullptr);

        w.bind("__alloy_cron_remove", [self](const std::string& id, const std::string& req, void*) {
            std::string t = ::webview::detail::json_parse(req, "", 0);
            self->m_webview->resolve(id, 0, self->removeCronJob(t));
        }, nullptr);

#ifdef WEBVIEW_GTK
        w.bind("__alloy_gui_create", [self](const std::string& id, const std::string& req, void*) {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string t = ::webview::detail::json_parse(req, "", 1);
            std::string p = ::webview::detail::json_parse(req, "", 2);
            self->gui_create(h, t, p);
            self->m_webview->resolve(id, 0, "true");
        }, nullptr);

        w.bind("__alloy_gui_append", [self](const std::string& id, const std::string& req, void*) {
            std::string ph = ::webview::detail::json_parse(req, "", 0);
            std::string ch = ::webview::detail::json_parse(req, "", 1);
            self->gui_append(ph, ch);
            self->m_webview->resolve(id, 0, "true");
        }, nullptr);

        w.bind("__alloy_gui_set_text", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string t = ::webview::detail::json_parse(req, "", 1);
            self->gui_set_text(h, t);
            return "";
        });

        w.bind("__alloy_gui_set_value", [self](const std::string& req) -> std::string {
            std::string h = ::webview::detail::json_parse(req, "", 0);
            std::string v = ::webview::detail::json_parse(req, "", 1);
            self->gui_set_value(h, v);
            return "";
        });
#endif
    }

    std::string spawn(const std::string& handle, const std::vector<std::string>& cmd, const std::string& options_json) {
        bool terminal = ::webview::detail::json_parse(options_json, "terminal", -1) != "";
        std::string cwd = ::webview::detail::json_parse(options_json, "cwd", -1);
        std::string env_json = ::webview::detail::json_parse(options_json, "env", -1);
        auto info = std::make_shared<ProcessInfo>();
        info->handle = handle;
        pid_t pid;
        int stdin_fd = -1, stdout_fd = -1, stderr_fd = -1, pty_master = -1;

        if (terminal) {
            pid = forkpty(&pty_master, NULL, NULL, NULL);
            if (pid < 0) return "{\"error\": \"forkpty failed\"}";
            if (pid == 0) {
                if (!cwd.empty()) { if(chdir(cwd.c_str())){}}
                apply_env(env_json);
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
                apply_env(env_json);
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
            stdin_fd = in_pipe[1]; stdout_fd = out_pipe[0]; stderr_fd = err_pipe[0];
            fcntl(stdout_fd, F_SETFL, fcntl(stdout_fd, F_GETFL) | O_NONBLOCK);
            fcntl(stderr_fd, F_SETFL, fcntl(stderr_fd, F_GETFL) | O_NONBLOCK);
        }
        info->pid = pid; info->stdin_fd = stdin_fd; info->stdout_fd = stdout_fd;
        info->stderr_fd = stderr_fd; info->pty_master = pty_master;
        { std::lock_guard<std::mutex> lock(m_mutex); m_processes[handle] = info; }
        setup_monitoring(info, terminal);
        return "{\"pid\": " + std::to_string(pid) + "}";
    }

    std::string spawnSync(const std::vector<std::string>& cmd, const std::string& options_json) {
        std::string cwd = ::webview::detail::json_parse(options_json, "cwd", -1);
        std::string env_json = ::webview::detail::json_parse(options_json, "env", -1);
        int out_pipe[2], err_pipe[2];
        if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) return "{\"success\": false}";
        pid_t pid = fork();
        if (pid < 0) return "{\"success\": false}";
        if (pid == 0) {
            if (!cwd.empty()) { if(chdir(cwd.c_str())){}}
            apply_env(env_json);
            dup2(out_pipe[1], STDOUT_FILENO); dup2(err_pipe[1], STDERR_FILENO);
            close(out_pipe[0]); close(err_pipe[0]);
            std::vector<char*> argv;
            for (const auto& s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            _exit(1);
        }
        close(out_pipe[1]); close(err_pipe[1]);
        std::string stdout_str, stderr_str;
        char buffer[4096]; ssize_t n;
        while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0) stdout_str.append(buffer, n);
        while ((n = read(err_pipe[0], buffer, sizeof(buffer))) > 0) stderr_str.append(buffer, n);
        close(out_pipe[0]); close(err_pipe[0]);
        int status; waitpid(pid, &status, 0);
        bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        return "{\"success\": " + std::string(success ? "true" : "false") +
               ", \"exitCode\": " + std::to_string(WIFEXITED(status) ? WEXITSTATUS(status) : -1) +
               ", \"stdout\": \""+ ::webview::detail::base64_encode(stdout_str) + "\"" +
               ", \"stderr\": \""+ ::webview::detail::base64_encode(stderr_str) + "\"" +
               ", \"pid\": " + std::to_string(pid) + "}";
    }

    static std::vector<std::string> parse_array(const std::string& json) {
        std::vector<std::string> result;
        for (int i = 0; ; ++i) {
            const char *value; size_t valuesz;
            if (::webview::detail::json_parse_c(json.c_str(), json.length(), nullptr, i, &value, &valuesz) != 0) break;
            if (value[0] == '"') {
                int n = ::webview::detail::json_unescape(value, valuesz, nullptr);
                if (n >= 0) {
                    char *decoded = new char[n + 1];
                    ::webview::detail::json_unescape(value, valuesz, decoded);
                    result.push_back(std::string(decoded, n));
                    delete[] decoded;
                }
            } else result.push_back(std::string(value, valuesz));
        }
        return result;
    }

    void apply_env(const std::string& env_json) {
        if (env_json.empty() || env_json == "null" || env_json == "{}") return;
        size_t pos = 0;
        while ((pos = env_json.find('"', pos)) != std::string::npos) {
            size_t k_end = env_json.find('"', pos + 1);
            if (k_end == std::string::npos) break;
            std::string key = env_json.substr(pos + 1, k_end - pos - 1);
            size_t colon = env_json.find(':', k_end + 1);
            if (colon == std::string::npos) break;
            size_t v_start = env_json.find('"', colon + 1);
            if (v_start == std::string::npos) break;
            size_t v_end = env_json.find('"', v_start + 1);
            if (v_end == std::string::npos) break;
            std::string val = env_json.substr(v_start + 1, v_end - v_start - 1);
            setenv(key.c_str(), val.c_str(), 1);
            pos = v_end + 1;
        }
    }

    void writeStdin(const std::string& h, const std::string& d) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(h);
        if (it != m_processes.end() && it->second->stdin_fd != -1) {
            ssize_t n = write(it->second->stdin_fd, d.c_str(), d.size()); (void)n;
        }
    }

    void closeStdin(const std::string& h) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(h);
        if (it != m_processes.end() && it->second->stdin_fd != -1) {
            if (it->second->pty_master == -1) { close(it->second->stdin_fd); it->second->stdin_fd = -1; }
        }
    }

    void killProcess(const std::string& h, int s) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(h);
        if (it != m_processes.end() && it->second->pid > 0) kill(it->second->pid, s);
    }

    void resizeTerminal(const std::string& h, int c, int r) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(h);
        if (it != m_processes.end() && it->second->pty_master != -1) {
            struct winsize ws; ws.ws_col = c; ws.ws_row = r;
            ioctl(it->second->pty_master, TIOCSWINSZ, &ws);
        }
    }

    void cleanup(const std::string& h) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_processes.find(h);
        if (it != m_processes.end()) { cleanup_monitoring(it->second); m_processes.erase(it); }
    }

    std::string registerCronJob(const std::string& script_path, const std::string& schedule, const std::string& title) {
#if defined(__linux__)
        std::string meta_path = get_executable_path();
        std::string entry = schedule + " '" + meta_path + "' run --cron-title='" + title + "' --cron-period='" + schedule + "' '" + script_path + "'";
        std::string marker = "# Alloy-cron: " + title;
        system(("crontab -l 2>/dev/null | grep -v '# Alloy-cron: " + title + "' | grep -v '--cron-title=" + title + "' > /tmp/crontab.tmp").c_str());
        std::ofstream out("/tmp/crontab.tmp", std::ios::app);
        out << marker << "\n" << entry << "\n";
        out.close();
        system("crontab /tmp/crontab.tmp && rm /tmp/crontab.tmp");
        return "true";
#elif defined(__APPLE__)
        return "true";
#else
        return "false";
#endif
    }

    std::string removeCronJob(const std::string& title) {
#if defined(__linux__)
        system(("crontab -l 2>/dev/null | grep -v '# Alloy-cron: " + title + "' | grep -v '--cron-title=" + title + "' > /tmp/crontab.tmp").c_str());
        system("crontab /tmp/crontab.tmp && rm /tmp/crontab.tmp");
        return "true";
#elif defined(__APPLE__)
        return "true";
#else
        return "false";
#endif
    }

    std::string get_executable_path() {
        char buf[4096];
#if defined(__linux__)
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len != -1) buf[len] = '\0';
        return std::string(buf);
#elif defined(__APPLE__)
        uint32_t size = sizeof(buf);
        if (_NSGetExecutablePath(buf, &size) == 0) return std::string(buf);
        return "";
#endif
        return "";
    }

#ifdef WEBVIEW_GTK
    struct GUIEventData { std::weak_ptr<SubprocessManager> manager; std::string handle; };
    void gui_create(const std::string& handle, const std::string& type, const std::string& props_json) {
        GtkWidget* widget = nullptr;
        if (type == "Window") {
            widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            std::string title = ::webview::detail::json_parse(props_json, "title", -1);
            if (!title.empty()) gtk_window_set_title(GTK_WINDOW(widget), title.c_str());
        } else if (type == "Button") {
            widget = gtk_button_new();
            std::string label = ::webview::detail::json_parse(props_json, "label", -1);
            if (!label.empty()) gtk_button_set_label(GTK_BUTTON(widget), label.c_str());
            auto self = shared_from_this();
            auto* ed = new GUIEventData{self, handle};
            g_signal_connect(widget, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
                auto* ed = static_cast<GUIEventData*>(data);
                auto mgr = ed->manager.lock();
                if (mgr && mgr->m_webview) {
                    mgr->m_webview->dispatch([mgr, h = ed->handle] {
                        mgr->m_webview->eval("window.Alloy.gui._onEvent(" + ::webview::detail::json_escape(h) + ", 'click')");
                    });
                }
            }), ed);
        } else if (type == "Label") {
            std::string text = ::webview::detail::json_parse(props_json, "text", -1);
            widget = gtk_label_new(text.c_str());
        } else if (type == "VStack") {
            widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        } else if (type == "HStack") {
            widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        } else if (type == "TextField") {
            widget = gtk_entry_new();
        } else if (type == "TextArea") {
            widget = gtk_text_view_new();
        } else if (type == "CheckBox") {
            widget = gtk_check_button_new();
            std::string label = ::webview::detail::json_parse(props_json, "label", -1);
            if (!label.empty()) gtk_button_set_label(GTK_BUTTON(widget), label.c_str());
        } else if (type == "Slider") {
            widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
        } else if (type == "ProgressBar") {
            widget = gtk_progress_bar_new();
        } else if (type == "Switch") {
            widget = gtk_switch_new();
        }

        if (widget) {
            gtk_widget_show(widget);
            std::lock_guard<std::mutex> lock(m_mutex);
            m_widgets[handle] = {handle, type, widget};
        }
    }

    void gui_append(const std::string& parent_handle, const std::string& child_handle) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it_p = m_widgets.find(parent_handle);
        auto it_c = m_widgets.find(child_handle);
        if (it_p != m_widgets.end() && it_c != m_widgets.end()) {
            if (GTK_IS_CONTAINER(it_p->second.widget)) {
                gtk_container_add(GTK_CONTAINER(it_p->second.widget), it_c->second.widget);
                gtk_widget_show_all(it_p->second.widget);
            }
        }
    }

    void gui_set_text(const std::string& handle, const std::string& text) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_widgets.find(handle);
        if (it != m_widgets.end()) {
            if (GTK_IS_LABEL(it->second.widget)) gtk_label_set_text(GTK_LABEL(it->second.widget), text.c_str());
            else if (GTK_IS_BUTTON(it->second.widget)) gtk_button_set_label(GTK_BUTTON(it->second.widget), text.c_str());
            else if (GTK_IS_ENTRY(it->second.widget)) gtk_entry_set_text(GTK_ENTRY(it->second.widget), text.c_str());
            else if (GTK_IS_TEXT_VIEW(it->second.widget)) {
                GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(it->second.widget));
                gtk_text_buffer_set_text(buffer, text.c_str(), -1);
            }
        }
    }

    void gui_set_value(const std::string& handle, const std::string& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_widgets.find(handle);
        if (it != m_widgets.end()) {
            if (GTK_IS_RANGE(it->second.widget)) gtk_range_set_value(GTK_RANGE(it->second.widget), std::stod(value));
            else if (GTK_IS_PROGRESS_BAR(it->second.widget)) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(it->second.widget), std::stod(value));
            else if (GTK_IS_SWITCH(it->second.widget)) gtk_switch_set_active(GTK_SWITCH(it->second.widget), value == "true");
        }
    }
#endif

private:
    struct WatchData { std::weak_ptr<SubprocessManager> manager; std::string handle; bool is_stderr; };

#ifdef WEBVIEW_GTK
    static gboolean on_io_ready(GIOChannel* source, GIOCondition condition, gpointer user_data) {
        auto* data = static_cast<WatchData*>(user_data);
        char buffer[4096]; gsize bytes_read;
        GIOStatus status = g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, NULL);
        if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
            std::string s(buffer, bytes_read); auto mgr = data->manager.lock();
            if (mgr && mgr->m_webview) {
                std::string h = data->handle; bool is_err = data->is_stderr;
                mgr->m_webview->dispatch([mgr, h, is_err, s] {
                    std::lock_guard<std::mutex> lock(mgr->m_mutex);
                    auto it = mgr->m_processes.find(h);
                    if (it != mgr->m_processes.end()) {
                        std::string type = it->second->pty_master != -1 ? "terminal" : (is_err ? "stderr" : "stdout");
                        std::string js = "window.Alloy._onData(" + ::webview::detail::json_escape(h) + ", " +
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
        auto* data = static_cast<WatchData*>(user_data); auto mgr = data->manager.lock();
        if (mgr && mgr->m_webview) {
            std::string h = data->handle;
            mgr->m_webview->dispatch([mgr, h, status] {
                std::lock_guard<std::mutex> lock(mgr->m_mutex);
                auto it = mgr->m_processes.find(h);
                if (it != mgr->m_processes.end()) {
                    it->second->exited = true;
                    it->second->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                    it->second->signal_code = WIFSIGNALED(status) ? WTERMSIG(status) : -1;
                    std::string js = "window.Alloy._onExit(" + ::webview::detail::json_escape(h) + ", " +
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
            g_io_channel_set_encoding(chan_err, NULL, NULL);
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
#ifdef WEBVIEW_GTK
    std::map<std::string, WidgetInfo> m_widgets;
#endif
    std::mutex m_mutex;
};

} // namespace meta
} // namespace webview

#else
namespace webview { namespace meta {
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
    std::string registerCronJob(const std::string&, const std::string&, const std::string&) { return "false"; }
    std::string removeCronJob(const std::string&) { return "false"; }
};
}}
#endif

#endif // WEBVIEW_META_HH
