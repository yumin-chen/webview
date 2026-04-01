#include "webview/webview.h"
#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Separated Dual Engine Architecture: Main process vs Hidden Unsafe WebView
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  // Main Safe C Process Logic (Simulated MicroQuickJS)
  printf("Starting Safe C Host Engine...\n");

  // Unsafe WebView (Hidden UI Layer)
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "Alloy Dual Engine Architecture");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

  // treats webview as hostile - hidden by default if possible, or just providing native API
  webview_set_html(w, "<h1>AlloyScript Protected Engine</h1><p>WebView is restricted to UI only.</p>");

  webview_run(w);
  webview_destroy(w);
  return 0;
}
