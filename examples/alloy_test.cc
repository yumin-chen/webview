#include "webview/webview.h"
#include <iostream>
#include <string>

constexpr const auto html = R"html(
<!DOCTYPE html>
<html>
<body>
  <h1>Alloy Spawn API Test</h1>
  <div id="output"></div>
  <button onclick="testSpawnVersion()">Test Spawn (Alloy --version)</button>
  <button onclick="testSpawnCat()">Test Spawn (cat with stdin)</button>

  <script>
    async function testSpawnVersion() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Spawning Alloy --version...</p>';

      // In this test environment, we'll use 'ls --version' as a proxy for 'Alloy --version'
      const proc = await Alloy.spawn(["ls", "--version"]);

      const text = await proc.stdout.text();
      output.innerHTML += '<pre>' + text + '</pre>';

      const exitCode = await proc.exited;
      const usage = proc.resourceUsage();
      output.innerHTML += '<p>Exited with code: ' + exitCode + '</p>';
      output.innerHTML += '<p>Max RSS: ' + usage.maxRSS + ' bytes</p>';
    }

    async function testSpawnCat() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Spawning cat...</p>';
      const proc = await Alloy.spawn(["cat"]);

      proc.stdin.write("Hello from Alloy stdin!\n");
      // Simulated flush/end logic would go here

      const reader = proc.stdout.getReader();
      const {value} = await reader.read();
      output.innerHTML += '<pre>Cat echoed: ' + new TextDecoder().decode(value) + '</pre>';

      proc.kill();
      await proc.exited;
      output.innerHTML += '<p>Cat process killed.</p>';
    }
  </script>
</body>
</html>
)html";

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("Alloy Spawn API Test");
        w.set_size(1024, 768, WEBVIEW_HINT_NONE);
        w.set_html(html);
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
