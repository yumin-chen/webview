#include "bridge.h"
#include <string.h>

void alloy_mqjs_bind_global(JSContext *ctx, const char *name, JSValue val) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, name, val);
}

// Helper to wrap AlloyCFunction into MicroQuickJS JSCFunction
static JSValue alloy_mqjs_func_wrapper(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, int magic) {
    // In a more complex implementation, we'd need to retrieve the AlloyCFunction from magic or params
    return JS_UNDEFINED;
}

void alloy_mqjs_bind_func(JSContext *ctx, const char *name, AlloyCFunction func, int argc) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    // This is a simplified binding. MicroQuickJS usually requires static tables.
    // For dynamic binding, we might need to extend MicroQuickJS or use its internal API more deeply.
    // Placeholder for dynamic function binding:
    // JSValue f = JS_NewCFunction(ctx, (JSCFunction *)func, name, argc);
    // JS_SetPropertyStr(ctx, global_obj, name, f);
}
