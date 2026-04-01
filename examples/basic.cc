#include "webview/webview.h"

#include <iostream>

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main() {
#endif
  try {
    // Dual Engine: logic in MQuickJS, browser APIs in hidden WebView
    webview::webview w(false, nullptr);
    w.set_title("Basic Example (WebView Provider)");
    w.set_size(0, 0, WEBVIEW_HINT_FIXED); // Hidden-ish or small for provider

    // Run basic logic in MQuickJS
    std::string logic = "Alloy.log('Hello from MQuickJS logic engine!');";
    w.eval("eval(" + webview::detail::json_escape(logic) + ")");

    w.run();
  } catch (const webview::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
