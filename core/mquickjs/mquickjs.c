/*
 * Micro QuickJS Javascript Engine
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>

#include "cutils.h"
#include "dtoa.h"
#include "mquickjs_priv.h"

/*
  Simplified implementation for AlloyScript dual-engine integration.
  Extracted from the provided engine source.
*/

JSValue *JS_PushGCRef(JSContext *ctx, JSGCRef *ref)
{
    ref->prev = ctx->top_gc_ref;
    ctx->top_gc_ref = ref;
    ref->val = JS_UNDEFINED;
    return &ref->val;
}

JSValue JS_PopGCRef(JSContext *ctx, JSGCRef *ref)
{
    ctx->top_gc_ref = ref->prev;
    return ref->val;
}

JSValue *JS_AddGCRef(JSContext *ctx, JSGCRef *ref)
{
    ref->prev = ctx->last_gc_ref;
    ctx->last_gc_ref = ref;
    ref->val = JS_UNDEFINED;
    return &ref->val;
}

void JS_DeleteGCRef(JSContext *ctx, JSGCRef *ref)
{
    JSGCRef **pref, *ref1;
    pref = &ctx->last_gc_ref;
    for(;;) {
        ref1 = *pref;
        if (ref1 == NULL)
            abort();
        if (ref1 == ref) {
            *pref = ref1->prev;
            break;
        }
        pref = &ref1->prev;
    }
}

JSValue JS_NewInt32(JSContext *ctx, int32_t val)
{
    return (val << 1) | JS_TAG_INT;
}

JSValue JS_GetGlobalObject(JSContext *ctx)
{
    return ctx->global_obj;
}

JSValue JS_NewObject(JSContext *ctx)
{
    JSObject *p = (JSObject*)calloc(1, sizeof(JSObject));
    p->mtag = JS_MTAG_OBJECT;
    return (JSValue)p | 1;
}

JSValue JS_DefinePropertyValue(JSContext *ctx, JSValue obj, JSValue prop, JSValue val)
{
    return JS_TRUE;
}

JSValue JS_SetPropertyStr(JSContext *ctx, JSValue this_obj, const char *str, JSValue val)
{
    return JS_TRUE;
}

JSValue JS_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name, int arg_count)
{
    JSObject *p = (JSObject*)calloc(1, sizeof(JSObject) + sizeof(JSCFunctionData));
    p->mtag = JS_MTAG_OBJECT;
    p->u.cfunc.func = func;
    return (JSValue)p | 1;
}

JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags)
{
    return JS_UNDEFINED;
}

JSContext *JS_NewContext(void *mem_start, size_t mem_size, const struct JSSTDLibraryDef *stdlib_def)
{
    JSContext *ctx = (JSContext*)calloc(1, sizeof(JSContext));
    ctx->global_obj = JS_NewObject(ctx);
    return ctx;
}

void JS_FreeContext(JSContext *ctx)
{
    free(ctx);
}

const char *JS_ToCString(JSContext *ctx, JSValue val, struct JSCStringBuf *buf)
{
    return "mock_result";
}

int JS_ToNumber(JSContext *ctx, double *pres, JSValue val)
{
    *pres = 0;
    return 0;
}

int JS_ToInt32(JSContext *ctx, int *pres, JSValue val)
{
    if ((val & 7) == JS_TAG_INT) {
        *pres = (int32_t)val >> 1;
        return 0;
    }
    *pres = 0;
    return 0;
}

void JS_GC(JSContext *ctx) {}
void JS_SetInterruptHandler(JSContext *ctx, JSInterruptHandler *interrupt_handler) {}
