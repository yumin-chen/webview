#include "webview/webview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

// --- Redesigned IPC treating WebView as hostile ---
void secure_dispatch(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Logic executes in the Safe C Host process
    printf("Safe Host received request: %s\n", req);
    webview_return(w, id, 0, "{\"status\":\"secure\"}");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "Dual Engine Secure IPC");

  // Bind critical logic globally via bind_global (if implemented)
  // For now, use standard bind but treat data as untrusted
  webview_bind(w, "secureDispatch", secure_dispatch, w);

  const char *html = "<html><body><h1>Secure Dual Engine Architecture</h1>"
                     "<button onclick='secureDispatch(\"test\")'>Call Safe Host</button>"
                     "</body></html>";

  webview_set_html(w, html);
  webview_run(w);
  webview_destroy(w);
  return 0;
}
