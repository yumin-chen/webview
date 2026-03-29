#include "webview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// The bundled JS will be injected here by the build script
extern const char* ALLOY_BUNDLE;

/**
 * Basic JSON-lite parser to extract command and args from:
 * ["command", ["arg1", "arg2"]]
 * In a real-world scenario, a proper JSON library like cJSON should be used.
 */
void alloy_spawn(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // For now, we return 0 as a stub, but we could implement fork/exec here
    webview_return(w, id, 0, "0");
}

void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Stub: simulate successful exit
    webview_return(w, id, 0, "0");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
  (void)hInst;
  (void)hPrevInst;
  (void)lpCmdLine;
  (void)nCmdShow;
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Runtime");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

  webview_bind(w, "alloy_spawn", alloy_spawn, w);
  webview_bind(w, "alloy_spawn_sync", alloy_spawn_sync, w);

  // Initialize Alloy bridge in JS
  const char* bridge_js =
      "window.Alloy = {"
      "  spawn: async (cmd, args) => await window.alloy_spawn(cmd, args),"
      "  spawnSync: (cmd, args) => window.alloy_spawn_sync(cmd, args)"
      "};";
  webview_init(w, bridge_js);

  // Inject the bundled AlloyScript
  webview_init(w, ALLOY_BUNDLE);

  webview_set_html(w, "<h1>AlloyScript Runtime</h1><p>Ready.</p>");
  webview_run(w);
  webview_destroy(w);
  return 0;
}
