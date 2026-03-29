#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"
#include "webview/detail/base64.hh"
#include <iostream>
#include <string>
#include <vector>
#include <memory>

int main(int argc, char** argv) {
    // CLI Argument handling for Cron jobs
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--cron-title=") == 0) {
            std::string title = arg.substr(13);
            std::string period;
            std::string script_path;
            for (int j = 1; j < argc; ++j) {
                std::string a = argv[j];
                if (a.find("--cron-period=") == 0) period = a.substr(14);
                else if (a.find("--") != 0 && a.find("run") != 0) script_path = a;
            }
            std::cout << "Cron Job Executing: " << title << "\n";
            std::cout << "Schedule: " << period << "\n";
            std::cout << "Script: " << script_path << "\n";
            std::cout << "Result: Simulation of scheduled() handler success.\n";
            return 0;
        }
    }

    try {
        auto w = std::make_shared<webview::webview>(true, nullptr);
        w->set_title("MetaScript Runtime (Host Orchestrator)");
        w->set_size(800, 600, WEBVIEW_HINT_NONE);

        auto mgr = std::make_shared<webview::meta::SubprocessManager>(w.get());
        mgr->bind(*w);

        const std::string html = R"html(
<!DOCTYPE html>
<html>
<head>
    <style>
        body { font-family: sans-serif; margin: 20px; }
        pre { background: #eee; padding: 10px; overflow: auto; max-height: 200px; border-radius: 4px; border: 1px solid #ccc; }
        .terminal { background: #000; color: #0f0; font-family: monospace; white-space: pre-wrap; height: 300px; }
        input { width: 100%; padding: 8px; box-sizing: border-box; }
        button { padding: 8px 16px; cursor: pointer; margin-bottom: 10px; margin-right: 5px; }
        h2 { margin-top: 20px; border-bottom: 1px solid #ddd; padding-bottom: 5px; }
    </style>
</head>
<body>
    <h1>MetaScript Runtime</h1>
    <p>Architecture: C (Host Orchestrator) + WebView (Web/JS Runtime)</p>

    <div>
        <h2>Cron Support</h2>
        <button onclick="testCronParse()">Parse Cron expression</button>
        <button onclick="testCronRegister()">Register daily job</button>
        <button onclick="testCronRemove()">Remove daily job</button>
        <pre id="cronOutput"></pre>
    </div>

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
        <h2>Interactive Terminal (PTY)</h2>
        <button onclick="testTerminal()">Start bash session</button>
        <pre id="termOutput" class="terminal"></pre>
        <input type="text" id="termInput" placeholder="Type command and press Enter..." onkeypress="if(event.key==='Enter') sendTerm()">
    </div>

    <script>
        async function testCronParse() {
            const out = document.getElementById('cronOutput');
            const expr = "30 9 * * MON-FRI";
            const next = window.meta.cron.parse(expr);
            out.textContent = "Expression: " + expr + "\nNext match: " + next.toISOString();
        }

        async function testCronRegister() {
            const out = document.getElementById('cronOutput');
            try {
                await window.meta.cron("./worker.ts", "0 0 * * *", "daily-job");
                out.textContent = "Daily job registered successfully.\nRun 'crontab -l' to verify.";
            } catch (e) {
                out.textContent = "Registration failed: " + e.message;
            }
        }

        async function testCronRemove() {
            const out = document.getElementById('cronOutput');
            await window.meta.cron.remove("daily-job");
            out.textContent = "Daily job removal requested.";
        }

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
                out.textContent = JSON.stringify({
                  ...res,
                  stdout: new TextDecoder().decode(res.stdout),
                  stderr: new TextDecoder().decode(res.stderr)
                }, null, 2);
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
                    cols: 80, rows: 24,
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
