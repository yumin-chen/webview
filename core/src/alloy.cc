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
        auto params_json = webview::detail::json_parse(req, "", 1);
        auto it = m_statements.find(stmt_id);
        if (it != m_statements.end()) {
            it->second->reset();
            for (int i = 0; ; ++i) {
                std::string p = webview::detail::json_parse(params_json, "", i);
                if (p.empty()) break;
                if (p == "null") it->second->bind_null(i + 1);
                else if (p[0] == '"') it->second->bind(i + 1, webview::detail::json_parse(params_json, "", i));
                else it->second->bind(i + 1, std::stod(p));
            }
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

    // GUI Bindings
    WV->bind("__alloy_gui_create_window", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto title = webview::detail::json_parse(req, "", 0);
        int w = std::stoi(webview::detail::json_parse(req, "", 1));
        int h = std::stoi(webview::detail::json_parse(req, "", 2));
        WV->resolve(seq, 0, std::to_string((uintptr_t)alloy_create_window(title.c_str(), w, h)));
    }, nullptr);

    WV->bind("__alloy_gui_create_component", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto type = webview::detail::json_parse(req, "", 0);
        auto parent = (alloy_component_t)std::stoull(webview::detail::json_parse(req, "", 1));
        alloy_component_t comp = nullptr;
        if (type == "button") comp = alloy_create_button(parent);
        else if (type == "textfield") comp = alloy_create_textfield(parent);
        else if (type == "textarea") comp = alloy_create_textarea(parent);
        else if (type == "label") comp = alloy_create_label(parent);
        else if (type == "checkbox") comp = alloy_create_checkbox(parent);
        else if (type == "radiobutton") comp = alloy_create_radiobutton(parent);
        else if (type == "combobox") comp = alloy_create_combobox(parent);
        else if (type == "slider") comp = alloy_create_slider(parent);
        else if (type == "spinner") comp = alloy_create_spinner(parent);
        else if (type == "progressbar") comp = alloy_create_progressbar(parent);
        else if (type == "tabview") comp = alloy_create_tabview(parent);
        else if (type == "listview") comp = alloy_create_listview(parent);
        else if (type == "treeview") comp = alloy_create_treeview(parent);
        else if (type == "webview") comp = alloy_create_webview(parent);
        else if (type == "vstack") comp = alloy_create_vstack(parent);
        else if (type == "hstack") comp = alloy_create_hstack(parent);
        else if (type == "scrollview") comp = alloy_create_scrollview(parent);
        else if (type == "switch") comp = alloy_create_switch(parent);
        else if (type == "separator") comp = alloy_create_separator(parent);
        else if (type == "image") comp = alloy_create_image(parent);
        else if (type == "icon") comp = alloy_create_icon(parent);
        else if (type == "menubar") comp = alloy_create_menubar(parent);
        else if (type == "toolbar") comp = alloy_create_toolbar(parent);
        else if (type == "statusbar") comp = alloy_create_statusbar(parent);
        else if (type == "splitter") comp = alloy_create_splitter(parent);
        else if (type == "dialog") comp = alloy_create_dialog("Dialog", 400, 300);
        else if (type == "filedialog") comp = alloy_create_filedialog(parent);
        else if (type == "colorpicker") comp = alloy_create_colorpicker(parent);
        else if (type == "datepicker") comp = alloy_create_datepicker(parent);
        else if (type == "timepicker") comp = alloy_create_timepicker(parent);
        else if (type == "link") comp = alloy_create_link(parent);
        else if (type == "chip") comp = alloy_create_chip(parent);
        else if (type == "accordion") comp = alloy_create_accordion(parent);
        else if (type == "codeeditor") comp = alloy_create_codeeditor(parent);
        else if (type == "tooltip") comp = alloy_create_tooltip(parent);
        else if (type == "groupbox") comp = alloy_create_groupbox(parent);
        else if (type == "popover") comp = alloy_create_popover(parent);
        else if (type == "badge") comp = alloy_create_badge(parent);
        else if (type == "card") comp = alloy_create_card(parent);
        else if (type == "rating") comp = alloy_create_rating(parent);
        else if (type == "menu") comp = alloy_create_menu(parent);
        else if (type == "contextmenu") comp = alloy_create_contextmenu(parent);
        else if (type == "divider") comp = alloy_create_divider(parent);
        else if (type == "loading_indicator") comp = alloy_create_loading_indicator(parent);
        else if (type == "richtexteditor") comp = alloy_create_richtexteditor(parent);

        WV->resolve(seq, 0, std::to_string((uintptr_t)comp));
    }, nullptr);

    WV->bind("__alloy_gui_set_text", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto h = (alloy_component_t)std::stoull(webview::detail::json_parse(req, "", 0));
        auto text = webview::detail::json_parse(req, "", 1);
        alloy_set_text(h, text.c_str());
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_signal_create_str", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        auto val = webview::detail::json_parse(req, "", 1);
        m_signals[id] = (alloy_signal_t)alloy_signal_create_str(val.c_str());
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_signal_set_str", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto id = webview::detail::json_parse(req, "", 0);
        auto val = webview::detail::json_parse(req, "", 1);
        alloy_signal_set_str(m_signals[id], val.c_str());
        WV->resolve(seq, 0, "");
    }, nullptr);

    WV->bind("__alloy_gui_bind_property", [this](const std::string& seq, const std::string& req, void* /*arg*/) {
        auto h = (alloy_component_t)std::stoull(webview::detail::json_parse(req, "", 0));
        int prop = std::stoi(webview::detail::json_parse(req, "", 1));
        auto sig_id = webview::detail::json_parse(req, "", 2);
        alloy_bind_property(h, (alloy_prop_id_t)prop, m_signals[sig_id]);
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
