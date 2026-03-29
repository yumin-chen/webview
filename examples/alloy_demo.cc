#include "webview/webview.h"
#include <iostream>
#include <string>

const std::string html = R"html(
<!DOCTYPE html>
<html>
<body>
    <h1>AlloyScript Demo</h1>
    <button onclick="runLs()">Run ls -l</button>
    <button onclick="runPwd()">Run pwd (Sync)</button>
    <button onclick="runBash()">Run interactive bash</button>
    <pre id="output"></pre>
    <script>
        const output = document.getElementById('output');
        function log(msg) {
            output.textContent += msg + '\n';
        }

        async function runLs() {
            log('Running ls -l...');
            const proc = Alloy.spawn(['ls', '-l']);
            const reader = proc.stdout.getReader();
            const decoder = new TextDecoder();
            while (true) {
                const {done, value} = await reader.read();
                if (done) break;
                log(decoder.decode(value));
            }
            const code = await proc.exited;
            log('Process exited with code: ' + code);
            const usage = proc.resourceUsage();
            log('Max RSS: ' + usage.maxRSS + ' bytes');
        }

        function runPwd() {
            log('Running pwd (Sync)...');
            const result = Alloy.spawnSync(['pwd']);
            const decoder = new TextDecoder();
            log('Output: ' + decoder.decode(result.stdout));
            log('Success: ' + result.success);
        }

        async function runBash() {
            log('Running bash via PTY...');
            const proc = Alloy.spawn(['bash'], {
                terminal: { cols: 80, rows: 24 }
            });

            proc.terminal._onData = (data) => {
                const decoder = new TextDecoder();
                log('PTY: ' + decoder.decode(data));
            };

            proc.terminal.write('echo "Hello from PTY!"\n');
            proc.terminal.write('exit\n');

            const code = await proc.exited;
            log('Bash exited with code: ' + code);
        }
    </script>
</body>
</html>
)html";

int main() {
    try {
        webview_t w = webview_create(1, nullptr);
        webview_set_title(w, "Alloy Demo");
        webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);
        webview_set_html(w, html.c_str());
        webview_run(w);
        webview_destroy(w);
    } catch (...) {
        return 1;
    }
    return 0;
}
