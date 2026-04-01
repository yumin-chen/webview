#ifndef ALLOY_BRIDGE_H
#define ALLOY_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../microquickjs/mquickjs.h"

// Common interface for binding global objects/functions to engines
typedef JSValue (*AlloyCFunction)(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

void alloy_mqjs_bind_global(JSContext *ctx, const char *name, JSValue val);
void alloy_mqjs_bind_func(JSContext *ctx, const char *name, AlloyCFunction func, int argc);

#ifdef __cplusplus
}
#endif

#endif // ALLOY_BRIDGE_H
