#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"
#include <iostream>
#include <string>
#include <vector>

const std::string html = R"html(
<!DOCTYPE html>
<html>
<head>
    <style>
        body { font-family: sans-serif; }
        pre { background: #eee; padding: 10px; overflow: auto; max-height: 200px; }
        .terminal { background: #000; color: #0f0; font-family: monospace; }
    </style>
</head>
<body>
    <h1>MetaScript Runtime Test</h1>

    <div>
        <button onclick="testSpawn()">Test meta.spawn(['ls', '-l'])</button>
        <pre id="spawnOutput"></pre>
    </div>

    <div>
        <button onclick="testSpawnSync()">Test meta.spawnSync(['uname', '-a'])</button>
        <pre id="syncOutput"></pre>
    </div>

    <div>
        <button onclick="testTerminal()">Test Terminal (bash)</button>
        <pre id="termOutput" class="terminal"></pre>
        <input type="text" id="termInput" placeholder="Type command here..." onkeypress="if(event.key==='Enter') sendTerm()">
    </div>

    <script>
        async function testSpawn() {
            const out = document.getElementById('spawnOutput');
            out.textContent = 'Spawning...';
            try {
                const proc = await window.meta.spawn(['ls', '-l', '/']);
                const text = await proc.stdout.text();
                out.textContent = text;
                const exitCode = await proc.exited;
                out.textContent += '\nExited with: ' + exitCode;
            } catch (e) {
                out.textContent = 'Error: ' + e.message;
            }
        }

        async function testSpawnSync() {
            const out = document.getElementById('syncOutput');
            out.textContent = 'Running sync...';
            const res = await window.meta.spawnSync(['uname', '-a']);
            out.textContent = JSON.stringify(res, null, 2);
        }

        let termProc = null;
        async function testTerminal() {
            const out = document.getElementById('termOutput');
            out.textContent = 'Starting bash...\n';
            termProc = await window.meta.spawn(['bash'], {
                terminal: {
                    cols: 80,
                    rows: 24,
                    data(terminal, data) {
                        out.textContent += new TextDecoder().decode(data);
                        out.scrollTop = out.scrollHeight;
                    },
                    exit(terminal, code) {
                        out.textContent += '\n[Terminal Exited]';
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

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("MetaScript Runtime");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);

        webview::meta::SubprocessManager mgr(&w);

        // Bindings for the JS runtime
        w.bind("__meta_spawn", [&](const std::string& id, const std::string& req, void*) {
            auto cmd_json = webview::detail::json_parse(req, "", 0);
            auto opts_json = webview::detail::json_parse(req, "", 1);

            std::vector<std::string> cmd;
            for (int i = 0; ; ++i) {
                auto s = webview::detail::json_parse(cmd_json, "", i);
                if (s.empty() && i > 0) break;
                if (s.empty()) break;
                cmd.push_back(s);
            }

            auto res = mgr.spawn(cmd, opts_json);
            w.resolve(id, 0, res);
        }, nullptr);

        w.bind("__meta_spawnSync", [&](const std::string& id, const std::string& req, void*) {
            auto cmd_json = webview::detail::json_parse(req, "", 0);
            auto opts_json = webview::detail::json_parse(req, "", 1);

            std::vector<std::string> cmd;
            for (int i = 0; ; ++i) {
                auto s = webview::detail::json_parse(cmd_json, "", i);
                if (s.empty() && i > 0) break;
                if (s.empty()) break;
                cmd.push_back(s);
            }

            auto res = mgr.spawnSync(cmd, opts_json);
            w.resolve(id, 0, res);
        }, nullptr);

        w.bind("__meta_write", [&](const std::string& req) -> std::string {
            auto pid = std::stoi(webview::detail::json_parse(req, "", 0));
            auto data = webview::detail::json_parse(req, "", 1);
            mgr.writeStdin(pid, data);
            return "";
        });

        w.bind("__meta_closeStdin", [&](const std::string& req) -> std::string {
            auto pid = std::stoi(webview::detail::json_parse(req, "", 0));
            mgr.closeStdin(pid);
            return "";
        });

        w.bind("__meta_kill", [&](const std::string& req) -> std::string {
            auto pid = std::stoi(webview::detail::json_parse(req, "", 0));
            auto sig_str = webview::detail::json_parse(req, "", 1);
            int sig = SIGTERM;
            if (sig_str == "SIGKILL") sig = SIGKILL;
            mgr.killProcess(pid, sig);
            return "";
        });

        w.bind("__meta_resize", [&](const std::string& req) -> std::string {
            auto pid = std::stoi(webview::detail::json_parse(req, "", 0));
            auto cols = std::stoi(webview::detail::json_parse(req, "", 1));
            auto rows = std::stoi(webview::detail::json_parse(req, "", 2));
            mgr.resizeTerminal(pid, cols, rows);
            return "";
        });

        w.init(webview::detail::meta_js);
        w.set_html(html);
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
