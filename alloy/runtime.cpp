#include "runtime.hpp"
#include "cron_parser.hpp"
#include "cron_manager.hpp"
#include "subprocess.hpp"
#include "process_manager.hpp"
#include "webview/json_deprecated.hh"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <poll.h>

namespace alloy {

void runtime::init(webview::webview& w) {
    w.bind("Alloy_cron_parse", [&](const std::string& req) -> std::string {
        try {
            std::string expression = webview::json_parse(req, "", 0);
            std::string relative_date_str = webview::json_parse(req, "", 1);
            if (expression.empty()) return "null";
            auto expr = cron_parser::parse(expression);
            auto relative_to = std::chrono::system_clock::now();
            if (!relative_date_str.empty()) {
                try {
                    auto ms = std::stoll(relative_date_str);
                    relative_to = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
                } catch (...) {}
            }
            auto next_time = cron_parser::next(expr, relative_to);
            std::time_t t = std::chrono::system_clock::to_time_t(next_time);
            std::tm tm_buf;
#ifdef _WIN32
            gmtime_s(&tm_buf, &t);
#else
            gmtime_r(&t, &tm_buf);
#endif
            std::ostringstream oss;
            oss << "\"" << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S.000Z") << "\"";
            return oss.str();
        } catch (...) { return "null"; }
    });

    w.bind("Alloy_cron_register", [&](const std::string& req) -> std::string {
        try {
            std::string path = webview::json_parse(req, "", 0);
            std::string schedule = webview::json_parse(req, "", 1);
            std::string title = webview::json_parse(req, "", 2);
            if (path.empty() || schedule.empty() || title.empty()) return "false";
            cron_manager::register_job(path, schedule, title);
            return "true";
        } catch (...) { return "false"; }
    });

    w.bind("Alloy_cron_remove", [&](const std::string& req) -> std::string {
        try {
            std::string title = webview::json_parse(req, "", 0);
            if (title.empty()) return "false";
            cron_manager::remove_job(title);
            return "true";
        } catch (...) { return "false"; }
    });

    w.bind("Alloy_spawn", [&](const std::string& req) -> std::string {
        try {
            std::string cmd_json = webview::json_parse(req, "", 0);
            std::string options_json = webview::json_parse(req, "", 1);
            std::vector<std::string> cmd;
            for(int i=0; ; ++i) {
                std::string arg = webview::json_parse(cmd_json, "", i);
                if (arg.empty()) break;
                cmd.push_back(arg);
            }
            spawn_options options;
            options.cwd = webview::json_parse(options_json, "cwd", -1);
            auto proc = std::make_shared<subprocess>(cmd, options);
            std::string id = process_manager::instance().register_proc(proc);
            std::ostringstream oss;
            oss << "{\"id\":\"" << id << "\", \"pid\":" << proc->pid() << "}";
            return oss.str();
        } catch (...) { return "null"; }
    });

    w.bind("Alloy_spawnSync", [&](const std::string& req) -> std::string {
        try {
            std::string cmd_json = webview::json_parse(req, "", 0);
            std::vector<std::string> cmd;
            for(int i=0; ; ++i) {
                std::string arg = webview::json_parse(cmd_json, "", i);
                if (arg.empty()) break;
                cmd.push_back(arg);
            }
            spawn_options options;
            auto proc = std::make_shared<subprocess>(cmd, options);
            auto res = proc->wait_sync();
            std::ostringstream oss;
            oss << "{\"exitCode\":" << res.exit_code
                << ",\"stdout\":\"" << webview::detail::json_escape(res.stdout_data) << "\""
                << ",\"stderr\":\"" << webview::detail::json_escape(res.stderr_data) << "\"}";
            return oss.str();
        } catch (...) { return "null"; }
    });

    w.bind("Alloy_proc_kill", [&](const std::string& req) -> std::string {
        std::string id = webview::json_parse(req, "", 0);
        auto proc = process_manager::instance().get_proc(id);
        if (proc) { proc->kill(); return "true"; }
        return "false";
    });

    w.bind("Alloy_proc_stdin_write", [&](const std::string& req) -> std::string {
        std::string id = webview::json_parse(req, "", 0);
        std::string data = webview::json_parse(req, "", 1);
        auto proc = process_manager::instance().get_proc(id);
        if (proc) { proc->stdin_write(data); return "true"; }
        return "false";
    });

    std::thread([&w]() {
        while (true) {
            auto procs = process_manager::instance().get_all_procs();
            if (procs.empty()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); continue; }
            std::vector<pollfd> fds;
            std::vector<std::string> ids;
            std::vector<std::string> streams;
            for (auto& pair : procs) {
                if (pair.second->get_stdout_fd() != -1) { fds.push_back({pair.second->get_stdout_fd(), POLLIN, 0}); ids.push_back(pair.first); streams.push_back("stdout"); }
                if (pair.second->get_stderr_fd() != -1) { fds.push_back({pair.second->get_stderr_fd(), POLLIN, 0}); ids.push_back(pair.first); streams.push_back("stderr"); }
            }
            if (poll(fds.data(), fds.size(), 100) > 0) {
                for (size_t i = 0; i < fds.size(); ++i) {
                    if (fds[i].revents & POLLIN) {
                        char buf[4096];
                        ssize_t n = read(fds[i].fd, buf, sizeof(buf));
                        if (n > 0) {
                            std::string data(buf, n);
                            std::string js = "window.Alloy._onProcData('" + ids[i] + "', '" + streams[i] + "', " + webview::detail::json_escape(data) + ")";
                            w.dispatch([&w, js]() { w.eval(js); });
                        }
                    }
                }
            }
            for (auto it = procs.begin(); it != procs.end(); ++it) {
                if (!it->second->is_alive()) {
                    int code = it->second->wait();
                    std::string js = "window.Alloy._onProcExit('" + it->first + "', " + std::to_string(code) + ")";
                    w.dispatch([&w, js]() { w.eval(js); });
                    process_manager::instance().unregister_proc(it->first);
                }
            }
        }
    }).detach();

    w.init(R"js(
        window.Alloy = window.Alloy || {};
        window.Alloy.cron = function(path, schedule, title) { return window.Alloy_cron_register(path, schedule, title); };
        window.Alloy.cron.parse = function(expression, relativeDate) {
            const result = window.Alloy_cron_parse(expression, relativeDate ? relativeDate.getTime() : null);
            return result ? new Date(result) : null;
        };
        window.Alloy.cron.remove = function(title) { return window.Alloy_cron_remove(title); };
        window.Alloy._procs = {};
        window.Alloy._onProcData = function(id, stream, data) { if (window.Alloy._procs[id]) window.Alloy._procs[id]._onData(stream, data); };
        window.Alloy._onProcExit = function(id, code) { if (window.Alloy._procs[id]) window.Alloy._procs[id]._onExit(code); delete window.Alloy._procs[id]; };
        window.Alloy.spawn = function(cmd, options) {
            const res = JSON.parse(window.Alloy_spawn(cmd, options));
            if (!res) return null;
            const proc = new Alloy.Subprocess(res.id, res.pid);
            window.Alloy._procs[res.id] = proc;
            return proc;
        };
        window.Alloy.spawnSync = function(cmd, options) { return JSON.parse(window.Alloy_spawnSync(cmd, options)); };
        window.Alloy.Subprocess = class {
            constructor(id, pid) {
                this.id = id; this.pid = pid;
                this._handlers = { stdout: [], stderr: [] };
                this.exited = new Promise((resolve) => { this._resolveExited = resolve; });
            }
            _onData(stream, data) { this._handlers[stream].forEach(h => h(data)); }
            _onExit(code) { this._resolveExited(code); }
            kill(signal) { window.Alloy_proc_kill(this.id, signal); }
            unref() {}
            get stdout() {
                return {
                    text: () => new Promise(resolve => {
                        let out = ""; const h = (d) => { out += d; };
                        this._handlers.stdout.push(h);
                        this.exited.then(() => resolve(out));
                    }),
                    on: (event, handler) => { if(event==='data') this._handlers.stdout.push(handler); }
                };
            }
            get stderr() {
                return {
                    on: (event, handler) => { if(event==='data') this._handlers.stderr.push(handler); }
                };
            }
        };
    )js");
}

} // namespace alloy
