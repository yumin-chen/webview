#include "webview/webview.h"
#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

// --- Dual Engine Architecture: Main Process (Safe) vs Hidden WebView (Unsafe) ---
// This example demonstrates the separation of the host process from the browser runtime.

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  // 1. Safe Host Process: Executes MicroQuickJS (WASM logic when target is browser)
  printf("Starting AlloyScript Engine Host (MicroQuickJS/WASM)...\n");

  // 2. Unsafe WebView Process: Provides browser APIs and rendering
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "Alloy Dual Engine - Basic Separation");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

  // treats webview as a capacities provider only
  webview_set_html(w, "<h1>Safe Host Active</h1><p>Browser runtime is sandboxed and isolated from the main C process.</p>");

  webview_run(w);
  webview_destroy(w);
  return 0;
}
