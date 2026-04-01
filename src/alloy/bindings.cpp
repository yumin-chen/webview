#include "bindings.hpp"
#include <map>

namespace alloy {

struct BindingContext {
    BindingFn fn;
    webview_t w;
    JSContext *ctx;
};

// Webview binding wrapper
static void webview_binding_wrapper(const char *seq, const char *req, void *arg) {
    BindingContext *bc = static_cast<BindingContext*>(arg);
    std::string result = bc->fn(req);
    webview_return(bc->w, seq, 0, result.c_str());
}

// MicroQuickJS binding wrapper placeholder (needs integration with mquickjs API)
/*
static JSValue mqjs_binding_wrapper(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // ... call bc->fn ...
    return JS_UNDEFINED;
}
*/

void bind_global(webview_t w, JSContext *ctx, const char *name, BindingFn fn) {
    BindingContext *bc = new BindingContext{fn, w, ctx};

    if (w) {
        webview_bind(w, name, webview_binding_wrapper, bc);
    }

    if (ctx) {
        // TODO: Register with MicroQuickJS using JS_NewCFunction or similar
        // This requires the full mquickjs.h API to be visible
    }
}

} // namespace alloy
