#include <iostream>
#include <sstream>
#include <cctype>
#include <vector>
#include <string>
#include <map>
#include <memory>

#include "webview/detail/json.hh"
#include "webview/detail/alloy_process.hh"
#include "webview/detail/alloy_sqlite.hh"
#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"

#include "webview.h"
#include "webview/alloy.hh"

namespace webview {
namespace detail {

std::string AlloyRuntime::base64_encode(const std::vector<char>& data) {
    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    for (auto const& c : data) {
        char_array_3[i++] = (unsigned char)c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        int j;
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while ((i++ < 3)) ret += '=';
    }
    return ret;
}

void AlloyRuntime::alloy_bind_sqlite_params(std::shared_ptr<AlloySQLite::Statement> stmt, const std::string& params_json) {
    for (int i = 0; ; ++i) {
        std::string p = json_parse(params_json, "", i);
        if (p.empty()) break;
        stmt->bind_positional(i + 1, p);
    }
}

std::string AlloyRuntime::alloy_row_to_json(std::shared_ptr<AlloySQLite::Statement> stmt, bool values_only) {
    std::stringstream ss;
    int cols = sqlite3_column_count(stmt->get());
    bool safeInts = stmt->is_safe_integers();
    if (values_only) ss << "["; else ss << "{";
    for (int i = 0; i < cols; ++i) {
        if (i > 0) ss << ",";
        if (!values_only) ss << "\"" << sqlite3_column_name(stmt->get(), i) << "\":";
        int type = sqlite3_column_type(stmt->get(), i);
        if (type == SQLITE_INTEGER) {
            sqlite3_int64 val = sqlite3_column_int64(stmt->get(), i);
            if (safeInts) ss << "\"" << std::to_string(val) << "\"";
            else ss << val;
        } else if (type == SQLITE_FLOAT) ss << sqlite3_column_double(stmt->get(), i);
        else if (type == SQLITE_TEXT) ss << json_escape((const char*)sqlite3_column_text(stmt->get(), i));
        else if (type == SQLITE_NULL) ss << "null";
        else ss << "\"blob\"";
    }
    if (values_only) ss << "]"; else ss << "}";
    return ss.str();
}

void AlloyRuntime::setup_bindings() {
    if (!m_webview) return;
    auto* browser = static_cast<::webview::webview*>(this->m_webview);

    browser->bind("__alloy_spawn", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string opts_json = json_parse(req, "", 1);
        AlloyProcess::Options options;
        std::string cmd_array = json_parse(opts_json, "cmd", 0);
        for (int i = 0; ; ++i) {
            std::string arg = json_parse(cmd_array, "", i);
            if (arg.empty()) break;
            options.argv.push_back(arg);
        }
        options.cwd = json_parse(opts_json, "cwd", 0);

        std::string term_obj = json_parse(opts_json, "terminal", 0);
        if (!term_obj.empty() && term_obj != "null") {
            options.terminal = std::make_shared<AlloyProcess::TerminalOptions>();
            std::string cs = json_parse(term_obj, "cols", 0);
            if (!cs.empty() && cs != "null") options.terminal->cols = std::stoi(cs);
            std::string rs = json_parse(term_obj, "rows", 0);
            if (!rs.empty() && rs != "null") options.terminal->rows = std::stoi(rs);
        }
        auto proc = std::make_shared<AlloyProcess>();
        this->m_processes[id] = proc;
        auto stdout_cb = [this, browser, id](const std::vector<char>& data) {
            std::string b64 = this->base64_encode(data);
            browser->dispatch([browser, id, b64]() {
                browser->eval("window.__alloy_on_data(" + json_escape(id) + ", \"stdout\", " + json_escape(b64) + ")");
            });
        };
        auto stderr_cb = [this, browser, id](const std::vector<char>& data) {
            std::string b64 = this->base64_encode(data);
            browser->dispatch([browser, id, b64]() {
                browser->eval("window.__alloy_on_data(" + json_escape(id) + ", \"stderr\", " + json_escape(b64) + ")");
            });
        };
        if (options.terminal) {
            proc->spawn(options, stdout_cb, stdout_cb, [this, id](int c, AlloyProcess::ResourceUsage u) { this->on_process_exit(id, c, u); });
        } else {
            proc->spawn(options, stdout_cb, stderr_cb, [this, id](int c, AlloyProcess::ResourceUsage u) { this->on_process_exit(id, c, u); });
        }
        browser->resolve(seq, 0, json_escape(std::to_string(proc->get_pid())));
    }, nullptr);

    browser->bind("__alloy_spawn_sync", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string opts_json = json_parse(req, "", 0);
        AlloyProcess::Options options;
        std::string cmd_array = json_parse(opts_json, "cmd", 0);
        for (int i = 0; ; ++i) {
            std::string arg = json_parse(cmd_array, "", i);
            if (arg.empty()) break;
            options.argv.push_back(arg);
        }
        options.cwd = json_parse(opts_json, "cwd", 0);
        AlloyProcess proc;
        auto res = proc.spawn_sync(options);
        std::stringstream ss;
        ss << "{"
           << "\"stdout\":" << json_escape(this->base64_encode(res.stdout_data)) << ","
           << "\"stderr\":" << json_escape(this->base64_encode(res.stderr_data)) << ","
           << "\"exitCode\":" << res.exitCode << ","
           << "\"success\":" << (res.success ? "true" : "false") << ","
           << "\"pid\":" << res.pid << ","
           << "\"resourceUsage\":{"
           << "\"maxRSS\":" << (long long)res.resourceUsage.maxRSS << ","
           << "\"cpuTime\":{\"user\":" << (long long)res.resourceUsage.cpuTime.user << ",\"system\":" << (long long)res.resourceUsage.cpuTime.system << "}"
           << "}}";
        browser->resolve(seq, 0, ss.str());
    }, nullptr);

    browser->bind("__alloy_write", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string data_str = json_parse(req, "", 1);
        auto it = this->m_processes.find(id);
        if (it != this->m_processes.end()) {
            std::vector<char> data(data_str.begin(), data_str.end());
            it->second->write_stdin(data);
        }
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_kill", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string sig_str = json_parse(req, "", 1);
        int sig = SIGTERM;
        if (sig_str == "SIGKILL") sig = SIGKILL;
        else if (sig_str == "SIGTERM") sig = SIGTERM;
        else if (!sig_str.empty() && isdigit(sig_str[0])) sig = std::stoi(sig_str);
        auto it = this->m_processes.find(id);
        if (it != this->m_processes.end()) it->second->kill_process(sig);
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_resize", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string cs = json_parse(req, "", 1);
        std::string rs = json_parse(req, "", 2);
        int cols = cs.empty() ? 80 : std::stoi(cs);
        int rows = rs.empty() ? 24 : std::stoi(rs);
        auto it = this->m_processes.find(id);
        if (it != this->m_processes.end()) it->second->resize_terminal(cols, rows);
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_sqlite_open", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string filename = json_parse(req, "", 1);
        std::string opts_json = json_parse(req, "", 2);
        AlloySQLite::Options opts;
        if (json_parse(opts_json, "readonly", 0) == "true") opts.readonly = true;
        if (json_parse(opts_json, "create", 0) == "false") opts.create = false;
        if (json_parse(opts_json, "safeIntegers", 0) == "true") opts.safeIntegers = true;
        try {
            this->m_databases[id] = std::make_shared<AlloySQLite>(filename, opts);
            browser->resolve(seq, 0, "");
        } catch (const std::exception& e) {
            browser->resolve(seq, 1, json_escape(e.what()));
        }
    }, nullptr);

    browser->bind("__alloy_sqlite_prepare", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string db_id = json_parse(req, "", 0);
        std::string stmt_id = json_parse(req, "", 1);
        std::string sql = json_parse(req, "", 2);
        std::string cached = json_parse(req, "", 3);
        auto it = this->m_databases.find(db_id);
        if (it != this->m_databases.end()) {
            try {
                auto stmt = (cached == "true") ? it->second->query(sql) : it->second->prepare(sql);
                this->m_statements[stmt_id] = stmt;
                std::stringstream ss;
                ss << "{\"paramsCount\":" << stmt->params_count() << ",\"columnNames\":[";
                auto names = stmt->column_names();
                for (size_t i = 0; i < names.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << json_escape(names[i]);
                }
                ss << "]}";
                browser->resolve(seq, 0, ss.str());
            } catch (const std::exception& e) { browser->resolve(seq, 1, json_escape(e.what())); }
        } else browser->resolve(seq, 1, "\"DB not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_get", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string sid = json_parse(req, "", 0);
        std::string pjson = json_parse(req, "", 1);
        auto it = this->m_statements.find(sid);
        if (it != this->m_statements.end()) {
            it->second->reset();
            this->alloy_bind_sqlite_params(it->second, pjson);
            int res = sqlite3_step(it->second->get());
            if (res == SQLITE_ROW) browser->resolve(seq, 0, this->alloy_row_to_json(it->second, false));
            else if (res == SQLITE_DONE) browser->resolve(seq, 0, "null");
            else browser->resolve(seq, 1, json_escape(sqlite3_errmsg(sqlite3_db_handle(it->second->get()))));
        } else browser->resolve(seq, 1, "\"Stmt not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_all", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string sid = json_parse(req, "", 0);
        std::string pjson = json_parse(req, "", 1);
        auto it = this->m_statements.find(sid);
        if (it != this->m_statements.end()) {
            it->second->reset();
            this->alloy_bind_sqlite_params(it->second, pjson);
            std::stringstream ss; ss << "["; bool first = true;
            while (sqlite3_step(it->second->get()) == SQLITE_ROW) {
                if (!first) ss << ",";
                ss << this->alloy_row_to_json(it->second, false);
                first = false;
            }
            ss << "]";
            browser->resolve(seq, 0, ss.str());
        } else browser->resolve(seq, 1, "\"Stmt not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_values", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string sid = json_parse(req, "", 0);
        std::string pjson = json_parse(req, "", 1);
        auto it = this->m_statements.find(sid);
        if (it != this->m_statements.end()) {
            it->second->reset();
            this->alloy_bind_sqlite_params(it->second, pjson);
            std::stringstream ss; ss << "["; bool first = true;
            while (sqlite3_step(it->second->get()) == SQLITE_ROW) {
                if (!first) ss << ",";
                ss << this->alloy_row_to_json(it->second, true);
                first = false;
            }
            ss << "]";
            browser->resolve(seq, 0, ss.str());
        } else browser->resolve(seq, 1, "\"Stmt not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_run", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string sid = json_parse(req, "", 0);
        std::string pjson = json_parse(req, "", 1);
        auto it = this->m_statements.find(sid);
        if (it != this->m_statements.end()) {
            it->second->reset();
            this->alloy_bind_sqlite_params(it->second, pjson);
            int res = sqlite3_step(it->second->get());
            if (res == SQLITE_DONE || res == SQLITE_ROW) {
                std::stringstream ss;
                sqlite3* db = sqlite3_db_handle(it->second->get());
                ss << "{\"lastInsertRowid\":" << (long long)sqlite3_last_insert_rowid(db)
                   << ",\"changes\":" << sqlite3_changes(db) << "}";
                browser->resolve(seq, 0, ss.str());
            } else browser->resolve(seq, 1, json_escape(sqlite3_errmsg(sqlite3_db_handle(it->second->get()))));
        } else browser->resolve(seq, 1, "\"Stmt not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_exec", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string db_id = json_parse(req, "", 0);
        std::string sql = json_parse(req, "", 1);
        auto it = this->m_databases.find(db_id);
        if (it != this->m_databases.end()) {
            try { it->second->exec(sql); browser->resolve(seq, 0, ""); }
            catch (const std::exception& e) { browser->resolve(seq, 1, json_escape(e.what())); }
        } else browser->resolve(seq, 1, "\"DB not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_serialize", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string db_id = json_parse(req, "", 0);
        auto it = this->m_databases.find(db_id);
        if (it != this->m_databases.end()) {
            auto data = it->second->serialize();
            std::vector<char> cdata(data.begin(), data.end());
            browser->resolve(seq, 0, json_escape(this->base64_encode(cdata)));
        } else browser->resolve(seq, 1, "\"DB not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_file_control", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string db_id = json_parse(req, "", 0);
        std::string op_s = json_parse(req, "", 1);
        int op = op_s.empty() ? 0 : std::stoi(op_s);
        std::string v_j = json_parse(req, "", 2);
        auto it = this->m_databases.find(db_id);
        if (it != this->m_databases.end()) {
            int val = v_j.empty() ? 0 : std::stoi(v_j);
            int res = it->second->file_control(op, &val);
            browser->resolve(seq, 0, std::to_string(res));
        } else browser->resolve(seq, 1, "\"DB not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_stmt_to_string", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        auto it = this->m_statements.find(id);
        if (it != this->m_statements.end()) browser->resolve(seq, 0, json_escape(it->second->to_sql()));
        else browser->resolve(seq, 1, "\"Stmt not found\"");
    }, nullptr);

    browser->bind("__alloy_sqlite_close", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        this->m_databases.erase(id);
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_sqlite_finalize", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        this->m_statements.erase(id);
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_gui_create_window", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string title = json_parse(req, "", 0);
        std::string w_s = json_parse(req, "", 1);
        std::string h_s = json_parse(req, "", 2);
        int width = w_s.empty() ? 800 : std::stoi(w_s);
        int height = h_s.empty() ? 600 : std::stoi(h_s);
        auto* win = (::alloy::detail::Component*)alloy_create_window(title.c_str(), width, height);
        win->webview_ptr = browser;
        win->runtime_id = std::to_string((uintptr_t)win);
        browser->resolve(seq, 0, json_escape(win->runtime_id));
    }, nullptr);

    browser->bind("__alloy_gui_create_component", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string type = json_parse(req, "", 0);
        std::string parent_str = json_parse(req, "", 1);
        auto parent = parent_str.empty() ? nullptr : (alloy_component_t)std::stoull(parent_str);
        alloy_component_t comp_handle = nullptr;
        if (type == "button") comp_handle = alloy_create_button(parent);
        else if (type == "textfield") comp_handle = alloy_create_textfield(parent);
        else if (type == "textarea") comp_handle = alloy_create_textarea(parent);
        else if (type == "label") comp_handle = alloy_create_label(parent);
        else if (type == "checkbox") comp_handle = alloy_create_checkbox(parent);
        else if (type == "radiobutton") comp_handle = alloy_create_radiobutton(parent);
        else if (type == "combobox") comp_handle = alloy_create_combobox(parent);
        else if (type == "slider") comp_handle = alloy_create_slider(parent);
        else if (type == "spinner") comp_handle = alloy_create_spinner(parent);
        else if (type == "progressbar") comp_handle = alloy_create_progressbar(parent);
        else if (type == "tabview") comp_handle = alloy_create_tabview(parent);
        else if (type == "listview") comp_handle = alloy_create_listview(parent);
        else if (type == "treeview") comp_handle = alloy_create_treeview(parent);
        else if (type == "webview") comp_handle = alloy_create_webview(parent);
        else if (type == "vstack") comp_handle = alloy_create_vstack(parent);
        else if (type == "hstack") comp_handle = alloy_create_hstack(parent);
        else if (type == "scrollview") comp_handle = alloy_create_scrollview(parent);
        else if (type == "switch") comp_handle = alloy_create_switch(parent);
        else if (type == "separator") comp_handle = alloy_create_separator(parent);
        else if (type == "image") comp_handle = alloy_create_image(parent);
        else if (type == "menubar") comp_handle = alloy_create_menubar(parent);
        else if (type == "toolbar") comp_handle = alloy_create_toolbar(parent);
        else if (type == "statusbar") comp_handle = alloy_create_statusbar(parent);
        else if (type == "splitter") comp_handle = alloy_create_splitter(parent);
        else if (type == "dialog") comp_handle = alloy_create_dialog("Dialog", 400, 300);
        else if (type == "filedialog") comp_handle = alloy_create_filedialog(parent);
        else if (type == "colorpicker") comp_handle = alloy_create_colorpicker(parent);
        else if (type == "datepicker") comp_handle = alloy_create_datepicker(parent);
        else if (type == "timepicker") comp_handle = alloy_create_timepicker(parent);
        else if (type == "link") comp_handle = alloy_create_link(parent);
        else if (type == "chip") comp_handle = alloy_create_chip(parent);
        else if (type == "accordion") comp_handle = alloy_create_accordion(parent);
        else if (type == "codeeditor") comp_handle = alloy_create_codeeditor(parent);
        else if (type == "groupbox") comp_handle = alloy_create_groupbox(parent);
        else if (type == "popover") comp_handle = alloy_create_popover(parent);
        else if (type == "badge") comp_handle = alloy_create_badge(parent);
        else if (type == "card") comp_handle = alloy_create_card(parent);
        else if (type == "rating") comp_handle = alloy_create_rating(parent);
        else if (type == "menu") comp_handle = alloy_create_menu(parent);
        else if (type == "contextmenu") comp_handle = alloy_create_contextmenu(parent);
        else if (type == "divider") comp_handle = alloy_create_divider(parent);
        else if (type == "loading_indicator") comp_handle = alloy_create_loading_indicator(parent);
        else if (type == "richtexteditor") comp_handle = alloy_create_richtexteditor(parent);

        auto* comp = (::alloy::detail::Component*)comp_handle;
        comp->webview_ptr = browser;
        comp->runtime_id = std::to_string((uintptr_t)comp);
        browser->resolve(seq, 0, json_escape(comp->runtime_id));
    }, nullptr);

    browser->bind("__alloy_gui_set_text", [this, browser](const std::string& seq, const std::string& req, void*) {
        auto h = (alloy_component_t)std::stoull(json_parse(req, "", 0));
        std::string text = json_parse(req, "", 1);
        alloy_set_text(h, text.c_str());
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_signal_create_str", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string val = json_parse(req, "", 1);
        this->m_signals[id] = (void*)alloy_signal_create_str(val.c_str());
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_signal_set_str", [this, browser](const std::string& seq, const std::string& req, void*) {
        std::string id = json_parse(req, "", 0);
        std::string val = json_parse(req, "", 1);
        alloy_signal_set_str((alloy_signal_t)this->m_signals[id], val.c_str());
        browser->resolve(seq, 0, "");
    }, nullptr);

    browser->bind("__alloy_gui_bind_property", [this, browser](const std::string& seq, const std::string& req, void*) {
        auto h = (alloy_component_t)std::stoull(json_parse(req, "", 0));
        std::string p_s = json_parse(req, "", 1);
        int prop = p_s.empty() ? 0 : std::stoi(p_s);
        std::string sig_id = json_parse(req, "", 2);
        alloy_bind_property(h, (alloy_prop_id_t)prop, (alloy_signal_t)this->m_signals[sig_id]);
        browser->resolve(seq, 0, "");
    }, nullptr);
}

void AlloyRuntime::on_process_exit(const std::string& id, int code, AlloyProcess::ResourceUsage usage) {
    if (!m_webview) return;
    auto* browser = static_cast<::webview::webview*>(this->m_webview);
    browser->dispatch([this, browser, id, code, usage]() {
        std::stringstream ss;
        ss << "{"
           << "\"maxRSS\":" << (long long)usage.maxRSS << ","
           << "\"cpuTime\":{\"user\":" << (long long)usage.cpuTime.user << ",\"system\":" << (long long)usage.cpuTime.system << "}"
           << "}";
        browser->eval("window.__alloy_on_exit(" + json_escape(id) + ", " + std::to_string(code) + ", " + ss.str() + ")");
        this->m_processes.erase(id);
    });
}

} // namespace detail
} // namespace webview
