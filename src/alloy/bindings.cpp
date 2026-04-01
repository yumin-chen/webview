#include "bindings.hpp"
#include "streams.hpp"
#include "fs.hpp"
#include "crypto.hpp"
#include "mquickjs_priv.h"
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

static JSValue mqjs_binding_wrapper(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // This wrapper would ideally find the C++ BindingFn and call it.
    // For this prototype, we'll assume a simplified registration.
    return JS_UNDEFINED;
}

void bind_global(webview_t w, JSContext *ctx, const char *name, BindingFn fn) {
    BindingContext *bc = new BindingContext{fn, w, ctx};
    if (w) webview_bind(w, name, webview_binding_wrapper, bc);

    if (ctx) {
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue func = JS_NewCFunction(ctx, mqjs_binding_wrapper, name, 1);
        // JS_SetPropertyStr(ctx, global_obj, name, func);
    }
}

// Native implementations for binary helpers
static JSValue js_bytes_to_base64(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // OpenSSL base64 encoding would go here
    return JS_NewString(ctx, "base64_mock");
}

static JSValue js_bytes_to_hex(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Hex encoding
    return JS_NewString(ctx, "hex_mock");
}

void register_alloy_runtime(webview_t w, JSContext *ctx) {
    if (ctx) {
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue alloy_obj = JS_NewObject(ctx);

        JSValue base64_fn = JS_NewCFunction(ctx, js_bytes_to_base64, "bytesToBase64", 1);
        JS_SetPropertyStr(ctx, alloy_obj, "bytesToBase64", base64_fn);

        JSValue hex_fn = JS_NewCFunction(ctx, js_bytes_to_hex, "bytesToHex", 1);
        JS_SetPropertyStr(ctx, alloy_obj, "bytesToHex", hex_fn);

        JSValue read_file_fn = JS_NewCFunction(ctx, FS::js_read_file, "_readFile", 1);
        JS_SetPropertyStr(ctx, alloy_obj, "_readFile", read_file_fn);

        JS_SetPropertyStr(ctx, global_obj, "Alloy", alloy_obj);
    }
}

} // namespace alloy
