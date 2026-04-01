#include "alloy/api.h"
#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  // Dual-engine example: MicroQuickJS core with hidden Service WebView
  // Initialize top-level native window (safe C process)
  alloy_component_t win = alloy_create_window("Alloy basic.c (Dual Engine)", 800, 600);

  // The service webview is hidden by default in the host runtime
  // but we can add components to this window directly.
  alloy_component_t btn = alloy_create_button(win);
  alloy_set_text(btn, "Click me (Native)");

  printf("Alloy basic.c example started.\n");

  alloy_run(win);
  alloy_destroy(win);
  return 0;
}
