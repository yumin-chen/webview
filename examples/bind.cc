#include "webview/webview.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

constexpr const auto html =
    R"html(
<div>
  <button id="increment">+</button>
  <button id="decrement">−</button>
  <span>Counter: <span id="counterResult">0</span></span>
</div>
<hr />
<div>
  <button id="compute">Compute</button>
  <span>Result: <span id="computeResult">(not started)</span></span>
</div>
<script type="module">
  const getElements = ids => Object.assign({}, ...ids.map(
    id => ({ [id]: document.getElementById(id) })));
  const ui = getElements([
    "increment", "decrement", "counterResult", "compute",
    "computeResult"
  ]);
  ui.increment.addEventListener("click", async () => {
    ui.counterResult.textContent = await window.count(1);
  });
  ui.decrement.addEventListener("click", async () => {
    ui.counterResult.textContent = await window.count(-1);
  });
  ui.compute.addEventListener("click", async () => {
    ui.compute.disabled = true;
    ui.computeResult.textContent = "(pending)";
    ui.computeResult.textContent = await window.compute(6, 7);
    ui.compute.disabled = false;
  });
</script>)html";

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main() {
#endif
  try {
    long count = 0;

    webview::webview w(true, nullptr);
    w.set_title("Bind Example (Dual Engine)");
    w.set_size(480, 320, WEBVIEW_HINT_NONE);

    // Global Bindings registered for both engines
    w.bind_global("count", [&](const std::string &id, const std::string &req, void*) {
      auto direction = std::stol(webview::detail::json_parse(req, "", 0));
      count += direction;
      w.return_result(id, 0, std::to_string(count));
    }, nullptr);

    w.bind_global("compute", [&](const std::string &id, const std::string &req, void*) {
      std::thread([&, id] {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        w.return_result(id, 0, "42");
      }).detach();
    }, nullptr);

    // Logic in MQuickJS orchestrates the WebView UI
    std::string logic = R"js(
        Alloy.log('Logic engine started.');
        // Initial setup or periodic tasks in safe engine
    )js";
    w.eval("eval(" + webview::detail::json_escape(logic) + ")");

    w.set_html(html);
    w.run();
  } catch (const webview::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
