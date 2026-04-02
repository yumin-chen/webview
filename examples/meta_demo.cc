#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/meta_js.hh"
#include <iostream>

#ifndef _WIN32
#include <signal.h>
#endif

const char html[] = R"html(
<!DOCTYPE html>
<html>
<body>
    <h1>MetaScript Demo</h1>
    <button onclick="runVersion()">Get Meta Version (spawn)</button>
    <button onclick="runCat()">Cat hello (pipe)</button>
    <button onclick="runSync()">Sync ls (spawnSync)</button>
    <pre id="output"></pre>
    <script>
        const output = document.getElementById('output');
        async function runVersion() {
            output.textContent = 'Running...';
            try {
                const proc = await meta.spawn(['uname', '-a']);
                output.textContent = await proc.stdout.text();
                console.log('Exit code:', await proc.exited);
            } catch (e) {
                output.textContent = 'Error: ' + e.message;
            }
        }

        async function runCat() {
            output.textContent = 'Running cat...';
            try {
                const proc = await meta.spawn(['cat'], { stdin: 'pipe' });
                proc.stdin.write('Hello from MetaScript!\n');
                output.textContent = await proc.stdout.text();
            } catch (e) {
                output.textContent = 'Error: ' + e.message;
            }
        }

        function runSync() {
            output.textContent = 'Running sync ls...';
            try {
                const result = meta.spawnSync(['ls', '-l']);
                output.textContent = result.stdout;
                console.log('Sync success:', result.success);
            } catch (e) {
                output.textContent = 'Error: ' + e.message;
            }
        }
    </script>
</body>
</html>
)html";

int main() {
    webview::webview w(true, nullptr);
    w.set_title("MetaScript Demo");
    w.set_size(800, 600, WEBVIEW_HINT_NONE);

    webview::MetaRuntime meta(w);

    w.bind("meta_spawn", [&](const std::string& id, const std::string& req, void* /*arg*/) {
        auto cmd_json = webview::detail::json_parse(req, "", 0);
        auto options_json = webview::detail::json_parse(req, "", 1);

        std::vector<std::string> cmd;
        for (int i = 0; ; ++i) {
            auto s = webview::detail::json_parse(cmd_json, "", i);
            if (s.empty() && i > 0) break;
            cmd.push_back(s);
        }

        std::string res = meta.spawn(cmd, options_json);
        w.resolve(id, 0, res);
    }, nullptr);

    w.bind("meta_write", [&](const std::string& req) -> std::string {
        int pid = std::stoi(webview::detail::json_parse(req, "", 0));
        std::string data = webview::detail::json_parse(req, "", 1);
        meta.write_to_stdin(pid, data);
        return "true";
    });

    w.bind("meta_kill", [&](const std::string& req) -> std::string {
        int pid = std::stoi(webview::detail::json_parse(req, "", 0));
#ifndef _WIN32
        meta.kill_process(pid, SIGTERM);
#else
        (void)pid;
#endif
        return "true";
    });

    w.set_sync_handler([&](const std::string& msg) -> std::string {
        auto method = webview::detail::json_parse(msg, "method", 0);
        auto params = webview::detail::json_parse(msg, "params", 0);
        if (method == "meta_spawnSync") {
            auto cmd_json = webview::detail::json_parse(params, "", 0);
            auto options_json = webview::detail::json_parse(params, "", 1);
            std::vector<std::string> cmd;
            for (int i = 0; ; ++i) {
                auto s = webview::detail::json_parse(cmd_json, "", i);
                if (s.empty() && i > 0) break;
                cmd.push_back(s);
            }
            return meta.spawnSync(cmd, options_json);
        }
        return "{}";
    });

    w.init(webview::META_JS);
    w.set_html(html);
    w.run();

    return 0;
}
