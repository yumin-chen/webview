#include "webview/alloy.hh"
#include "webview/webview.h"

namespace webview {
namespace detail {

#define WV (static_cast<webview::webview*>(m_webview))

void AlloyRuntime::setup_bindings() {
    WV->bind("__alloy_spawn", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        auto options_json = webview::detail::json_parse(req, "", 1);

        AlloyProcess::Options options;
        std::string cmd_array = webview::detail::json_parse(options_json, "cmd", 0);
        for (int i = 0; ; ++i) {
            std::string arg = webview::detail::json_parse(cmd_array, "", i);
            if (arg.empty()) break;
            options.argv.push_back(arg);
        }

        options.cwd = webview::detail::json_parse(options_json, "cwd", 0);

        std::string terminal_obj = webview::detail::json_parse(options_json, "terminal", 0);
        if (!terminal_obj.empty() && terminal_obj != "null") {
            options.terminal = std::make_shared<AlloyProcess::TerminalOptions>();
            std::string cols = webview::detail::json_parse(terminal_obj, "cols", 0);
            if (!cols.empty()) options.terminal->cols = std::stoi(cols);
            std::string rows = webview::detail::json_parse(terminal_obj, "rows", 0);
            if (!rows.empty()) options.terminal->rows = std::stoi(rows);
        }

        auto proc = std::make_shared<AlloyProcess>();
        m_processes[id] = proc;

        auto stdout_cb = [this, id](const std::vector<char>& data) {
            std::string b64 = base64_encode(data);
            WV->dispatch([this, id, b64]() {
                WV->eval("window.__alloy_on_data(\"" + id + "\", \"stdout\", \"" + b64 + "\")");
            });
        };

        auto stderr_cb = [this, id](const std::vector<char>& data) {
            std::string b64 = base64_encode(data);
            WV->dispatch([this, id, b64]() {
                WV->eval("window.__alloy_on_data(\"" + id + "\", \"stderr\", \"" + b64 + "\")");
            });
        };

        if (options.terminal) {
             auto terminal_cb = [this, id](const std::vector<char>& data) {
                std::string b64 = base64_encode(data);
                WV->dispatch([this, id, b64]() {
                    WV->eval("window.__alloy_on_data(\"" + id + "\", \"terminal\", \"" + b64 + "\")");
                });
            };
            proc->spawn(options, terminal_cb, terminal_cb, [this, id](int code, AlloyProcess::ResourceUsage usage) {
                on_process_exit(id, code, usage);
            });
        } else {
            proc->spawn(options, stdout_cb, stderr_cb, [this, id](int code, AlloyProcess::ResourceUsage usage) {
                on_process_exit(id, code, usage);
            });
        }

        WV->resolve(seq, 0, std::to_string(proc->get_pid()));
    }, nullptr);

    WV->bind("__alloy_spawn_sync", [this](const std::string& req) -> std::string {
        auto options_json = webview::detail::json_parse(req, "", 0);
        AlloyProcess::Options options;
        std::string cmd_array = webview::detail::json_parse(options_json, "cmd", 0);
        for (int i = 0; ; ++i) {
            std::string arg = webview::detail::json_parse(cmd_array, "", i);
            if (arg.empty()) break;
            options.argv.push_back(arg);
        }
        options.cwd = webview::detail::json_parse(options_json, "cwd", 0);

        AlloyProcess proc;
        auto res = proc.spawn_sync(options);

        std::stringstream ss;
        ss << "{"
           << "\"stdout\":\"" << base64_encode(res.stdout_data) << "\","
           << "\"stderr\":\"" << base64_encode(res.stderr_data) << "\","
           << "\"exitCode\":" << res.exitCode << ","
           << "\"success\":" << (res.success ? "true" : "false") << ","
           << "\"pid\":" << res.pid << ","
           << "\"resourceUsage\":{"
           << "\"maxRSS\":" << res.resourceUsage.maxRSS << ","
           << "\"cpuTime\":{\"user\":" << res.resourceUsage.cpuTime.user << ",\"system\":" << res.resourceUsage.cpuTime.system << "}"
           << "}}";
        return ss.str();
    });

    WV->bind("__alloy_write", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        auto data_str = webview::detail::json_parse(req, "", 1);
        auto it = m_processes.find(id);
        if (it != m_processes.end()) {
            std::vector<char> data(data_str.begin(), data_str.end());
            it->second->write_stdin(data);
        }
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_kill", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        auto it = m_processes.find(id);
        if (it != m_processes.end()) {
            it->second->kill_process();
        }
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_resize", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        int cols = std::stoi(webview::detail::json_parse(req, "", 1));
        int rows = std::stoi(webview::detail::json_parse(req, "", 2));
        auto it = m_processes.find(id);
        if (it != m_processes.end()) {
            it->second->resize_terminal(cols, rows);
        }
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_sqlite_open", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        auto filename = webview::detail::json_parse(req, "", 1);
        auto flags_str = webview::detail::json_parse(req, "", 2);
        int flags = flags_str.empty() ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) : std::stoi(flags_str);

        try {
            m_databases[id] = std::make_shared<AlloySQLite>(filename, flags);
            WV->resolve(seq, 0, "");
        } catch (const std::exception& e) {
            WV->resolve(seq, 1, e.what());
        }
    }, nullptr);

    WV->bind("__alloy_sqlite_close", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        m_databases.erase(id);
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_sqlite_prepare", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto db_id = webview::detail::json_parse(req, "", 0);
        auto stmt_id = webview::detail::json_parse(req, "", 1);
        auto sql = webview::detail::json_parse(req, "", 2);

        auto it = m_databases.find(db_id);
        if (it != m_databases.end()) {
            try {
                m_statements[stmt_id] = it->second->prepare(sql);
                WV->resolve(seq, 0, "");
            } catch (const std::exception& e) {
                WV->resolve(seq, 1, e.what());
            }
        } else {
            WV->resolve(seq, 1, "Database not found");
        }
    }, nullptr);

    WV->bind("__alloy_sqlite_step", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto stmt_id = webview::detail::json_parse(req, "", 0);
        auto it = m_statements.find(stmt_id);
        if (it != m_statements.end()) {
            int res = sqlite3_step(it->second->get());
            if (res == SQLITE_ROW) {
                std::stringstream ss;
                ss << "{";
                int cols = sqlite3_column_count(it->second->get());
                for (int i = 0; i < cols; ++i) {
                    if (i > 0) ss << ",";
                    ss << "\"" << sqlite3_column_name(it->second->get(), i) << "\":";
                    int type = sqlite3_column_type(it->second->get(), i);
                    if (type == SQLITE_INTEGER) ss << sqlite3_column_int64(it->second->get(), i);
                    else if (type == SQLITE_FLOAT) ss << sqlite3_column_double(it->second->get(), i);
                    else if (type == SQLITE_TEXT) ss << webview::detail::json_escape((const char*)sqlite3_column_text(it->second->get(), i));
                    else if (type == SQLITE_NULL) ss << "null";
                    else ss << "\"blob\"";
                }
                ss << "}";
                WV->resolve(seq, 0, ss.str());
            } else if (res == SQLITE_DONE) {
                WV->resolve(seq, 0, "null");
            } else {
                WV->resolve(seq, 1, sqlite3_errmsg(sqlite3_db_handle(it->second->get())));
            }
        } else {
            WV->resolve(seq, 1, "Statement not found");
        }
    }, nullptr);

    WV->bind("__alloy_sqlite_finalize", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto stmt_id = webview::detail::json_parse(req, "", 0);
        m_statements.erase(stmt_id);
        WV->resolve(seq, 0, "");
    }, nullptr);
}

void AlloyRuntime::on_process_exit(const std::string& id, int code, AlloyProcess::ResourceUsage usage) {
    WV->dispatch([this, id, code, usage]() {
        std::stringstream ss;
        ss << "{"
           << "\"maxRSS\":" << usage.maxRSS << ","
           << "\"cpuTime\":{\"user\":" << usage.cpuTime.user << ",\"system\":" << usage.cpuTime.system << "}"
           << "}";
        WV->eval("window.__alloy_on_exit(\"" + id + "\", " + std::to_string(code) + ", " + ss.str() + ")");
        m_processes.erase(id);
    });
}

} // namespace detail
} // namespace webview
