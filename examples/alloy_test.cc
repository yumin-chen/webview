#include "webview/webview.h"
#include <iostream>
#include <string>

constexpr const auto html = R"html(
<!DOCTYPE html>
<html>
<body>
  <h1>Alloy Test</h1>
  <div id="output"></div>
  <button onclick="testSpawn()">Test Spawn (ls)</button>
  <button onclick="testSpawnSync()">Test SpawnSync (whoami)</button>
  <button onclick="testTerminal()">Test Terminal (bash)</button>

  <script>
    async function testSpawn() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Spawning ls...</p>';
      const proc = await Alloy.spawn(["ls", "-l"]);
      if (!proc) {
        output.innerHTML += '<p style="color: red">Spawn failed!</p>';
        return;
      }

      const reader = proc.stdout.getReader();
      while (true) {
        const {done, value} = await reader.read();
        if (done) break;
        output.innerHTML += '<pre>' + new TextDecoder().decode(value) + '</pre>';
      }
      const exitCode = await proc.exited;
      output.innerHTML += '<p>Process exited with code: ' + exitCode + '</p>';
    }

    function testSpawnSync() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Running whoami sync...</p>';
      const result = Alloy.spawnSync(["whoami"]);
      output.innerHTML += '<p>Result: ' + (result.stdout || "Error") + '</p>';
    }

    async function testTerminal() {
        const output = document.getElementById('output');
        output.innerHTML += '<p>Creating terminal...</p>';
        const term = new Alloy.Terminal({
            cols: 80,
            rows: 24,
            data(t, data) {
                output.innerHTML += '<pre style="color: blue">' + new TextDecoder().decode(data) + '</pre>';
            },
            exit(t, code) {
                output.innerHTML += '<p>Terminal exited with code: ' + code + '</p>';
            }
        });
        await term.init();
        if (term.id === "null") {
            output.innerHTML += '<p style="color: red">Terminal creation failed!</p>';
            return;
        }
        output.innerHTML += '<p>Terminal created with ID: ' + term.id + '</p>';
        term.write("echo 'Hello from PTY'\n");
        term.write("exit\n");
    }
  </script>
</body>
</html>
)html";

int main() {
    try {
        // webview::webview is not shared_ptr compatible out of the box with engine_base shared_from_this()
        // since webview::webview is a wrapper around engine_base.
        // Actually, webview::webview uses a new backend, which inherits from engine_base.
        // In gtk_webkit_engine: public engine_base.
        // The problem is engine_base::shared_from_this() requires the object to be owned by a shared_ptr.
        // The current C API and webview.h don't use shared_ptr.
        // This is a major issue with the shared_from_this() approach if we don't change how webview is instantiated.

        webview::webview w(true, nullptr);
        w.set_title("Alloy Test");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);
        w.set_html(html);
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
