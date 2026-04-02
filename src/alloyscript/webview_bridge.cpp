#include "bridge.h"
#include "../webview/webview.h"
#include <string>
#include <functional>

void alloy_webview_bind(webview_t w, const char *name, webview_bind_fn_t fn, void *arg) {
    webview_bind(w, name, fn, arg);
}
