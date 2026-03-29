#include "webview/webview.h"
#include <iostream>
#include <string>

constexpr const auto html = R"html(
<!DOCTYPE html>
<html>
<body>
  <h1>Alloy Shell Test</h1>
  <div id="output"></div>
  <button onclick="testEcho()">Test Echo</button>
  <button onclick="testPipe()">Test Pipe (echo | wc)</button>
  <button onclick="testCwd()">Test CWD (pwd)</button>
  <button onclick="testBraces()">Test Braces expansion</button>

  <script>
    async function testEcho() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Running echo...</p>';
      const text = await Alloy.$`echo "Hello from Alloy Shell!"`.text();
      output.innerHTML += '<pre>' + text + '</pre>';
    }

    async function testPipe() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Running pipe: echo | wc -c</p>';
      const text = await Alloy.$`echo "12345" | wc -c`.text();
      output.innerHTML += '<pre>Result: ' + text + '</pre>';
    }

    async function testCwd() {
      const output = document.getElementById('output');
      output.innerHTML += '<p>Running pwd in /tmp...</p>';
      const text = await Alloy.$`pwd`.cwd("/tmp").text();
      output.innerHTML += '<pre>CWD: ' + text + '</pre>';
    }

    function testBraces() {
        const output = document.getElementById('output');
        output.innerHTML += '<p>Expanding {1,2,3}...</p>';
        const result = Alloy.$.braces("echo {1,2,3}");
        output.innerHTML += '<pre>' + JSON.stringify(result) + '</pre>';
    }
  </script>
</body>
</html>
)html";

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("Alloy Shell Test");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);
        w.set_html(html);
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
