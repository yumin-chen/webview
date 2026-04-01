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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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

#define __exception __attribute__((warn_unused_result))

#define JS_STACK_SLACK  16   /* additional free space on the stack */
#define JS_MIN_FREE_SIZE 512
#define JS_MIN_CRITICAL_FREE_SIZE (JS_MIN_FREE_SIZE - 256)
#define JS_MAX_LOCAL_VARS 65535
#define JS_MAX_FUNC_STACK_SIZE 65535
#define JS_MAX_ARGC 65535
#define JS_MAX_CALL_RECURSE 8

#define JS_VALUE_IS_BOTH_INT(a, b) ((((a) | (b)) & 1) == 0)

static __maybe_unused const char *js_mtag_name[JS_MTAG_COUNT] = {
    "free",
    "object",
    "float64",
    "string",
    "func_bytecode",
    "value_array",
    "byte_array",
    "varref",
};

#define FRAME_CF_ARGC_MASK      0xffff
#define FRAME_CF_POP_RET        (1 << 17)
#define FRAME_CF_PC_ADD1        (1 << 18)

#define JS_MB_PAD(n)  (JSW * 8 - (n))

typedef struct {
    JS_MB_HEADER;
    JSWord dummy: JS_MB_PAD(JS_MTAG_BITS);
} JSMemBlockHeader;

typedef struct {
    JS_MB_HEADER;
    JSWord size: JS_MB_PAD(JS_MTAG_BITS);
} JSFreeBlock;

#if JSW == 8
#define JS_STRING_LEN_MAX 0x7ffffffe
#else
#define JS_STRING_LEN_MAX ((1 << (32 - JS_MTAG_BITS - 3)) - 1)
#endif

typedef struct {
    JS_MB_HEADER;
    JSWord is_unique: 1;
    JSWord is_ascii: 1;
    JSWord is_numeric: 1;
    JSWord len: JS_MB_PAD(JS_MTAG_BITS + 3);
    uint8_t buf[];
} JSString;

typedef struct {
    JSWord string_buf[sizeof(JSString) / sizeof(JSWord)];
    uint8_t buf[5];
} JSStringCharBuf;

#define JS_BYTE_ARRAY_SIZE_MAX ((1 << (32 - JS_MTAG_BITS)) - 1)

typedef struct {
    JS_MB_HEADER;
    JSWord size: JS_MB_PAD(JS_MTAG_BITS);
    uint8_t buf[];
} JSByteArray;

#define JS_VALUE_ARRAY_SIZE_MAX ((1 << (32 - JS_MTAG_BITS)) - 1)

typedef struct {
    JS_MB_HEADER;
    JSWord size: JS_MB_PAD(JS_MTAG_BITS);
    JSValue arr[];
} JSValueArray;

typedef struct JSVarRef {
    JS_MB_HEADER;
    JSWord is_detached : 1;
    JSWord dummy: JS_MB_PAD(JS_MTAG_BITS + 1);
    union {
        JSValue value;
        struct {
            JSValue next;
            JSValue *pvalue;
        };
    } u;
} JSVarRef;

typedef struct {
    JS_MB_HEADER;
    JSWord dummy: JS_MB_PAD(JS_MTAG_BITS);
#ifdef JS_PTR64
    struct {
        double dval;
    } u;
#else
    struct __attribute__((packed)) {
        double dval;
    } u;
#endif
} JSFloat64;

struct JSContext {
    uint8_t *heap_base;
    uint8_t *heap_free;
    uint8_t *stack_top;
    JSValue *stack_bottom;
    JSValue *sp;
    JSValue *fp;
    uint32_t min_free_size;
    BOOL in_out_of_memory : 8;
    uint8_t n_rom_atom_tables;
    uint16_t class_count;
    int16_t interrupt_counter;
    JSValue current_exception;
    JSValue global_obj;
    void *opaque;
    // ... Simplified for AlloyScript integration
};

static int get_mblock_size(const void *ptr) {
    int mtag = ((JSMemBlockHeader *)ptr)->mtag;
    switch(mtag) {
    case JS_MTAG_OBJECT: return offsetof(JSObject, u) + ((JSObject*)ptr)->extra_size * JSW;
    case JS_MTAG_FLOAT64: return sizeof(JSFloat64);
    case JS_MTAG_STRING: return sizeof(JSString) + ((((JSString*)ptr)->len + JSW) & ~(JSW - 1));
    case JS_MTAG_BYTE_ARRAY: return sizeof(JSByteArray) + ((((JSByteArray*)ptr)->size + JSW - 1) & ~(JSW - 1));
    case JS_MTAG_VALUE_ARRAY: return sizeof(JSValueArray) + ((JSValueArray*)ptr)->size * sizeof(JSValue);
    case JS_MTAG_FREE: return sizeof(JSFreeBlock) + ((JSFreeBlock*)ptr)->size * sizeof(JSWord);
    case JS_MTAG_VARREF: return sizeof(JSVarRef) - (((JSVarRef*)ptr)->is_detached ? sizeof(JSValue) : 0);
    case JS_MTAG_FUNCTION_BYTECODE: return sizeof(JSFunctionBytecode);
    default: return 0;
    }
}

JSContext *JS_NewContext(void *mem_start, size_t mem_size, const void *stdlib_def) {
    JSContext *ctx = mem_start;
    memset(ctx, 0, sizeof(*ctx));
    ctx->stack_top = (uint8_t *)mem_start + mem_size;
    ctx->sp = (JSValue *)ctx->stack_top;
    ctx->heap_base = (uint8_t *)mem_start + sizeof(JSContext);
    ctx->heap_free = ctx->heap_base;
    ctx->min_free_size = JS_MIN_FREE_SIZE;
    return ctx;
}

void JS_FreeContext(JSContext *ctx) {}

JSValue JS_GetGlobalObject(JSContext *ctx) { return ctx->global_obj; }

JSValue JS_NewObject(JSContext *ctx) {
    JSObject *p = (JSObject *)malloc(sizeof(JSObject));
    memset(p, 0, sizeof(JSObject));
    p->mtag = JS_MTAG_OBJECT;
    return JS_VALUE_FROM_PTR(p);
}

void JS_SetContextOpaque(JSContext *ctx, void *opaque) { ctx->opaque = opaque; }
void *JS_GetContextOpaque(JSContext *ctx) { return ctx->opaque; }

JSValue JS_NewCFunction(JSContext *ctx, JSValue (*func)(JSContext *, JSValue *, int, JSValue *), const char *name, int length) {
    JSObject *p = (JSObject *)malloc(sizeof(JSObject));
    memset(p, 0, sizeof(JSObject));
    p->mtag = JS_MTAG_OBJECT;
    p->class_id = JS_CLASS_C_FUNCTION;
    // In a real implementation we would store the func pointer here.
    return JS_VALUE_FROM_PTR(p);
}

const char *JS_ToCString(JSContext *ctx, JSValue val) {
    if (!JS_IsPtr(val)) return "";
    JSMemBlockHeader *h = JS_VALUE_TO_PTR(val);
    if (h->mtag == JS_MTAG_STRING) return (const char *)((JSString *)h)->buf;
    return "";
}

JSValue JS_NewString(JSContext *ctx, const char *str) {
    int len = strlen(str);
    JSString *p = (JSString *)malloc(sizeof(JSString) + len + 1);
    p->mtag = JS_MTAG_STRING;
    p->len = len;
    memcpy(p->buf, str, len + 1);
    return JS_VALUE_FROM_PTR(p);
}

// Simplified stubs for the rest of the API to satisfy linking
JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags) { return JS_UNDEFINED; }
JSValue JS_GetPropertyStr(JSContext *ctx, JSValue this_obj, const char *prop) { return JS_UNDEFINED; }
int JS_SetPropertyStr(JSContext *ctx, JSValue this_obj, const char *prop, JSValue val) { return 0; }
JSValue JS_GetException(JSContext *ctx) { return JS_UNDEFINED; }
JSValue *JS_PushGCRef(JSContext *ctx, JSGCRef *ref) { return &ref->val; }
JSValue JS_PopGCRef(JSContext *ctx, JSGCRef *ref) { return ref->val; }
JSValue *JS_AddGCRef(JSContext *ctx, JSGCRef *ref) { return &ref->val; }
void JS_DeleteGCRef(JSContext *ctx, JSGCRef *ref) {}
int JS_DefinePropertyStr(JSContext *ctx, JSValue this_obj, const char *prop, JSValue val, int flags) { return 0; }
const char *JS_GetCFunctionName(JSContext *ctx, JSValue val) { return ""; }
