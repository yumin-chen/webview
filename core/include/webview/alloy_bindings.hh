#ifndef WEBVIEW_ALLOY_BINDINGS_HH
#define WEBVIEW_ALLOY_BINDINGS_HH

#include "alloy.hh"
#include "json.hh"
#include <string>
#include <vector>

namespace webview {
namespace detail {

inline std::vector<std::string> parse_alloy_json_array(const std::string& json) {
    std::vector<std::string> res;
    for (int i = 0; ; ++i) {
        auto val = json_parse(json, "", i);
        if (val.empty()) {
            const char* v; size_t vsz;
            if (json_parse_c(json.c_str(), json.length(), nullptr, i, &v, &vsz) != 0) break;
            res.push_back(val);
        } else res.push_back(val);
    }
    return res;
}

inline void setup_alloy_bindings(alloy_runtime& runtime, std::function<void(const std::string&, std::function<void(const std::string&, const std::string&, void*)>, void*)> bind_fn) {
    bind_fn("__alloy_spawn", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto proc_id = json_parse(req, "", 0);
        auto cmd_json = json_parse(req, "", 1);
        auto opts_json = json_parse(req, "", 2);
        std::vector<std::string> cmd = parse_alloy_json_array(cmd_json);
        spawn_options opts;
        opts.stdin_type = json_parse(opts_json, "stdin", -1);
        if (opts.stdin_type.empty()) opts.stdin_type = "none";
        opts.stdout_type = json_parse(opts_json, "stdout", -1);
        if (opts.stdout_type.empty()) opts.stdout_type = "pipe";
        opts.stderr_type = json_parse(opts_json, "stderr", -1);
        if (opts.stderr_type.empty()) opts.stderr_type = "inherit";
        opts.cwd = json_parse(opts_json, "cwd", -1);
        opts.has_ipc = !json_parse(opts_json, "ipc", -1).empty();
        auto terminal_json = json_parse(opts_json, "terminal", -1);
        if (!terminal_json.empty()) {
            opts.use_terminal = true;
            auto cols_str = json_parse(terminal_json, "cols", -1);
            if (!cols_str.empty()) opts.terminal.cols = std::stoi(cols_str);
            auto rows_str = json_parse(terminal_json, "rows", -1);
            if (!rows_str.empty()) opts.terminal.rows = std::stoi(rows_str);
        }
        int pid = runtime.spawn(proc_id, cmd, opts);
        return "{\"pid\":" + std::to_string(pid) + "}";
    }, nullptr);

    bind_fn("__alloy_spawnSync", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto cmd_json = json_parse(req, "", 0);
        auto opts_json = json_parse(req, "", 1);
        std::vector<std::string> cmd = parse_alloy_json_array(cmd_json);
        spawn_options opts;
        opts.cwd = json_parse(opts_json, "cwd", -1);
        auto res = runtime.spawn_sync(cmd, opts);
        return "{\"exitCode\":" + std::to_string(res.exit_code) + ",\"stdout\":" + json_escape(base64_encode(res.stdout_data)) + ",\"stderr\":" + json_escape(base64_encode(res.stderr_data)) + ",\"resourceUsage\":" + res.usage_json + ",\"success\":" + (res.exit_code == 0 ? "true" : "false") + ",\"isBase64\":true}";
    }, nullptr);

    bind_fn("__alloy_kill", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto proc_id = json_parse(req, "", 0);
        auto signal_str = json_parse(req, "", 1);
        int sig = 15; if (signal_str == "SIGKILL" || signal_str == "9") sig = 9;
        runtime.kill(proc_id, sig);
    }, nullptr);

    bind_fn("__alloy_stdin_write", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto proc_id = json_parse(req, "", 0);
        auto data = json_parse(req, "", 1);
        runtime.stdin_write(proc_id, data);
    }, nullptr);

    bind_fn("__alloy_stdin_close", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto proc_id = json_parse(req, "", 0);
        runtime.stdin_close(proc_id);
    }, nullptr);

    bind_fn("__alloy_ipc_send", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto proc_id = json_parse(req, "", 0);
        auto message = json_parse(req, "", 1);
        runtime.ipc_send(proc_id, message);
    }, nullptr);

    bind_fn("__alloy_ipc_disconnect", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto proc_id = json_parse(req, "", 0);
        runtime.ipc_disconnect(proc_id);
    }, nullptr);

    bind_fn("__alloy_terminal_write", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto term_id = json_parse(req, "", 0);
        auto data = json_parse(req, "", 1);
        runtime.stdin_write(term_id, data);
    }, nullptr);

    bind_fn("__alloy_terminal_resize", [&runtime](const std::string& /*id*/, const std::string& req, void* /*arg*/) {
        auto term_id = json_parse(req, "", 0);
        int cols = std::stoi(json_parse(req, "", 1));
        int rows = std::stoi(json_parse(req, "", 2));
        runtime.terminal_resize(term_id, cols, rows);
    }, nullptr);
}

} // namespace detail
} // namespace webview

#endif // WEBVIEW_ALLOY_BINDINGS_HH
