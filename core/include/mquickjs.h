/*
 * MIT License
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ALLOY_ENGINE_MQUICKJS_H
#define ALLOY_ENGINE_MQUICKJS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JSContext JSContext;
typedef uint32_t JSValue;

#define JS_TAG_INT            0
#define JS_TAG_BOOL           1
#define JS_TAG_NULL           2
#define JS_TAG_UNDEFINED      3
#define JS_TAG_UNINITIALIZED  4
#define JS_TAG_CATCH_OFFSET   5
#define JS_TAG_EXCEPTION      6
#define JS_TAG_SHORT_FUNC     7
#define JS_TAG_STRING_CHAR    8

#define JS_VALUE_GET_TAG(v) ((v) & 0xf)
#define JS_VALUE_GET_INT(v) ((int32_t)(v) >> 1)
#define JS_VALUE_IS_INT(v) (((v) & 1) == 0)

#define JS_UNDEFINED JS_TAG_UNDEFINED
#define JS_NULL JS_TAG_NULL

typedef struct JSGCRef {
    struct JSGCRef *prev;
    JSValue val;
} JSGCRef;

JSContext *JS_NewContext(void *mem_start, size_t mem_size, const void *stdlib_def);
void JS_FreeContext(JSContext *ctx);
JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags);
JSValue JS_GetGlobalObject(JSContext *ctx);
JSValue JS_NewObject(JSContext *ctx);
JSValue JS_GetPropertyStr(JSContext *ctx, JSValue this_obj, const char *prop);
int JS_SetPropertyStr(JSContext *ctx, JSValue this_obj, const char *prop, JSValue val);
JSValue JS_NewCFunction(JSContext *ctx, JSValue (*func)(JSContext *, JSValue *, int, JSValue *), const char *name, int length);
const char *JS_ToCString(JSContext *ctx, JSValue val);
JSValue JS_GetException(JSContext *ctx);

JSValue *JS_PushGCRef(JSContext *ctx, JSGCRef *ref);
JSValue JS_PopGCRef(JSContext *ctx, JSGCRef *ref);
JSValue *JS_AddGCRef(JSContext *ctx, JSGCRef *ref);
void JS_DeleteGCRef(JSContext *ctx, JSGCRef *ref);

#define JS_EVAL_TYPE_GLOBAL (0 << 0)
#define JS_PROP_CONFIGURABLE (1 << 0)
#define JS_PROP_ENUMERABLE (1 << 1)

int JS_DefinePropertyStr(JSContext *ctx, JSValue this_obj, const char *prop, JSValue val, int flags);
const char *JS_GetCFunctionName(JSContext *ctx, JSValue val);

#ifdef __cplusplus
}
#endif

#endif /* ALLOY_ENGINE_MQUICKJS_H */
