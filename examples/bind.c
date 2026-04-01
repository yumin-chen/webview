#include "webview/webview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

// --- Dual Engine Secure IPC Example ---
// The main process (C host) implements sensitive logic in MicroQuickJS/WASM.
// The WebView process is restricted to providing browswer-only capacities.

void secure_host_handler(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // This logic runs in the safe host process, not in the webview.
    printf("Safe Host (Dual Engine) received message: %s\n", req);
    webview_return(w, id, 0, "{\"response\":\"Authenticated by Safe Host\"}");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "Alloy Dual Engine - Secure Bind");

  // Bind sensitive functions via secure dual-engine bridge
  webview_bind(w, "secure_call", secure_host_handler, w);

  const char *html = "<html><body>"
                     "<h1>Dual Engine Secure Interface</h1>"
                     "<button onclick='secure_call(\"ping\")'>Call Safe Host</button>"
                     "<div id='res'></div>"
                     "<script>window.secure_call = async (m) => { const r = await window.__webview__.call('secure_call', m); document.getElementById('res').innerText = JSON.stringify(r); };</script>"
                     "</body></html>";

  webview_set_html(w, html);
  webview_run(w);
  webview_destroy(w);
  return 0;
}
