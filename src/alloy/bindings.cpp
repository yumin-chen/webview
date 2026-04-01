#include "bindings.hpp"
#include "streams.hpp"
#include <map>

namespace alloy {

struct BindingContext {
    BindingFn fn;
    webview_t w;
    JSContext *ctx;
};

static void webview_binding_wrapper(const char *seq, const char *req, void *arg) {
    BindingContext *bc = static_cast<BindingContext*>(arg);
    std::string result = bc->fn(req);
    webview_return(bc->w, seq, 0, result.c_str());
}

void bind_global(webview_t w, JSContext *ctx, const char *name, BindingFn fn) {
    BindingContext *bc = new BindingContext{fn, w, ctx};
    if (w) webview_bind(w, name, webview_binding_wrapper, bc);
    if (ctx) {
        // Registering as a global function in MicroQuickJS
        // Placeholder for JS_NewCFunction call
    }
}

void register_alloy_runtime(webview_t w, JSContext *ctx) {
    // 1. Define window.Alloy object
    // 2. Define window.Alloy.ArrayBufferSink class

    if (w) {
        // WebView side is usually handled via script injection
    }

    if (ctx) {
        // MicroQuickJS side:
        // JSValue global_obj = JS_GetGlobalObject(ctx);
        // JSValue alloy_obj = JS_NewObject(ctx);
        // JS_SetPropertyStr(ctx, alloy_obj, "ArrayBufferSink", ...);
        // JS_SetPropertyStr(ctx, global_obj, "Alloy", alloy_obj);
    }
}

} // namespace alloy
