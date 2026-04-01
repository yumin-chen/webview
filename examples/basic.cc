#include "webview/webview.h"

#include <iostream>

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main() {
#endif
  try {
    webview::webview w(false, nullptr);
    w.set_title("AlloyScript Basic Example");
    w.set_size(480, 320, WEBVIEW_HINT_NONE);

    // Demonstrate secure host execution using Alloy.secureEval
    w.dispatch([&]() {
        w.eval("Alloy.secureEval('console.log(\"Hello from MicroQuickJS host!\"); Alloy.sqlite.open(\":memory:\");')");
    });

    w.set_html("<html><body><h1>Dual Engine Architecture</h1><p>WebView is a capacity provider.</p></body></html>");
    w.run();
  } catch (const webview::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
