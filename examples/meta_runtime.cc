#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"
#include "webview/detail/base64.hh"
#include <iostream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> parse_json_array(const std::string& json) {
    std::vector<std::string> result;
    for (int i = 0; ; ++i) {
        const char *value;
        size_t valuesz;
        if (webview::detail::json_parse_c(json.c_str(), json.length(), nullptr, i, &value, &valuesz) != 0) break;
        if (value[0] == '"') {
            int n = webview::detail::json_unescape(value, valuesz, nullptr);
            if (n >= 0) {
                char *decoded = new char[n + 1];
                webview::detail::json_unescape(value, valuesz, decoded);
                result.push_back(std::string(decoded, n));
                delete[] decoded;
            }
        } else result.push_back(std::string(value, valuesz));
    }
    return result;
}
}

int main() {
    try {
        auto w = std::make_shared<webview::webview>(true, nullptr);
        w->set_title("MetaScript Runtime");
        w->set_size(800, 600, WEBVIEW_HINT_NONE);

        auto mgr = std::make_shared<webview::meta::SubprocessManager>(w.get());

        w->bind("__meta_spawn", [mgr, w](const std::string& id, const std::string& req, void*) {
            auto handle = webview::detail::json_parse(req, "", 0);
            auto cmd_json = webview::detail::json_parse(req, "", 1);
            auto opts_json = webview::detail::json_parse(req, "", 2);
            auto res = mgr->spawn(handle, parse_json_array(cmd_json), opts_json);
            w->resolve(id, 0, res);
        }, nullptr);

        w->bind("__meta_spawnSync", [mgr, w](const std::string& id, const std::string& req, void*) {
            auto cmd_json = webview::detail::json_parse(req, "", 0);
            auto opts_json = webview::detail::json_parse(req, "", 1);
            auto res = mgr->spawnSync(parse_json_array(cmd_json), opts_json);
            w->resolve(id, 0, res);
        }, nullptr);

        w->bind("__meta_write", [mgr](const std::string& req) -> std::string {
            auto handle = webview::detail::json_parse(req, "", 0);
            auto data_b64 = webview::detail::json_parse(req, "", 1);
            if (!handle.empty()) mgr->writeStdin(handle, webview::detail::base64_decode(data_b64));
            return "";
        });

        w->bind("__meta_closeStdin", [mgr](const std::string& req) -> std::string {
            auto handle = webview::detail::json_parse(req, "", 0);
            if (!handle.empty()) mgr->closeStdin(handle);
            return "";
        });

        w->bind("__meta_kill", [mgr](const std::string& req) -> std::string {
            auto handle = webview::detail::json_parse(req, "", 0);
            auto sig_str = webview::detail::json_parse(req, "", 1);
            if (!handle.empty()) {
                int sig = SIGTERM;
                if (sig_str == "SIGKILL" || sig_str == "9") sig = SIGKILL;
                mgr->killProcess(handle, sig);
            }
            return "";
        });

        w->bind("__meta_resize", [mgr](const std::string& req) -> std::string {
            auto handle = webview::detail::json_parse(req, "", 0);
            auto cols_str = webview::detail::json_parse(req, "", 1);
            auto rows_str = webview::detail::json_parse(req, "", 2);
            if (!handle.empty() && !cols_str.empty() && !rows_str.empty()) {
                mgr->resizeTerminal(handle, std::stoi(cols_str), std::stoi(rows_str));
            }
            return "";
        });

        w->bind("__meta_cleanup", [mgr](const std::string& req) -> std::string {
            auto handle = webview::detail::json_parse(req, "", 0);
            if (!handle.empty()) mgr->cleanup(handle);
            return "";
        });

        const std::string html = R"html(
<!DOCTYPE html>
<html>
<head>
    <style>
        body { font-family: sans-serif; margin: 20px; }
        pre { background: #eee; padding: 10px; overflow: auto; max-height: 200px; border-radius: 4px; border: 1px solid #ccc; }
        .terminal { background: #000; color: #0f0; font-family: monospace; white-space: pre-wrap; height: 300px; }
        input { width: 100%; padding: 8px; box-sizing: border-box; }
        button { padding: 8px 16px; cursor: pointer; margin-bottom: 10px; }
        h2 { margin-top: 20px; }
    </style>
</head>
<body>
    <h1>MetaScript Runtime</h1>

    <div>
        <h2>Process Spawning</h2>
        <button onclick="testSpawn()">Test meta.spawn(['ls', '-l', '/'])</button>
        <pre id="spawnOutput"></pre>
    </div>

    <div>
        <h2>Synchronous Execution</h2>
        <button onclick="testSpawnSync()">Test meta.spawnSync(['uname', '-a'])</button>
        <pre id="syncOutput"></pre>
    </div>

    <div>
        <h2>Interactive Terminal</h2>
        <button onclick="testTerminal()">Start bash session</button>
        <pre id="termOutput" class="terminal"></pre>
        <input type="text" id="termInput" placeholder="Type command and press Enter..." onkeypress="if(event.key==='Enter') sendTerm()">
    </div>

    <script>
        async function testSpawn() {
            const out = document.getElementById('spawnOutput');
            out.textContent = 'Spawning...';
            try {
                const proc = window.meta.spawn(['ls', '-l', '/']);
                const text = await proc.stdout.text();
                out.textContent = text;
                const exitCode = await proc.exited;
                out.textContent += '\n---\nExited with code: ' + exitCode;
            } catch (e) {
                out.textContent = 'Error: ' + e.message;
            }
        }

        async function testSpawnSync() {
            const out = document.getElementById('syncOutput');
            out.textContent = 'Running...';
            try {
                const res = await window.meta.spawnSync(['uname', '-a']);
                out.textContent = JSON.stringify(res, null, 2);
            } catch (e) {
                out.textContent = 'Error: ' + e.message;
            }
        }

        let termProc = null;
        async function testTerminal() {
            const out = document.getElementById('termOutput');
            out.textContent = 'Terminal started.\n';
            termProc = window.meta.spawn(['bash'], {
                terminal: {
                    cols: 80,
                    rows: 24,
                    data(terminal, data) {
                        out.textContent += new TextDecoder().decode(data);
                        out.scrollTop = out.scrollHeight;
                    },
                    exit(terminal, code) {
                        out.textContent += '\n[Terminal Session Ended]';
                    }
                }
            });
        }

        function sendTerm() {
            const input = document.getElementById('termInput');
            if (termProc) {
                termProc.terminal.write(input.value + '\n');
                input.value = '';
            }
        }
    </script>
</body>
</html>
)html";

        w->init(webview::detail::meta_js);
        w->set_html(html);
        w->run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
