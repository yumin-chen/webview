#include "webview/webview.h"
#include <iostream>
#include <string>

const char html[] = R"html(
<!DOCTYPE html>
<html>
<body>
    <h1>AlloyScript Demo</h1>
    <button onclick="runCommand()">Run 'ls -l'</button>
    <pre id="output"></pre>
    <script>
        async function runCommand() {
            const output = document.getElementById('output');
            output.textContent = "Running...\n";
            try {
                const proc = alloy.spawn(["ls", "-l"]);
                const text = await proc.stdout.text();
                output.textContent += text;
                const exitCode = await proc.exited;
                output.textContent += "\nProcess exited with code: " + exitCode;

                const usage = proc.resourceUsage();
                if (usage) {
                    output.textContent += "\nMax RSS: " + usage.maxRSS + " KB (normalized)";
                }
            } catch (e) {
                output.textContent += "\nError: " + e.message;
            }
        }
    </script>
</body>
</html>
)html";

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main() {
#endif
  try {
    webview::webview w(true, nullptr);
    w.set_title("AlloyScript Demo");
    w.set_size(800, 600, WEBVIEW_HINT_NONE);
    w.set_html(html);
    w.run();
  } catch (const webview::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return 0;
}
