#ifndef MQUICKJS_PRIV_H
#define MQUICKJS_PRIV_H

#include "cutils.h"
#include "list.h"
#include "mquickjs.h"

#define JSW 8

typedef uintptr_t JSWord;

#define JS_MTAG_BITS 4

#define JS_MB_HEADER \
    JSWord gc_mark: 1; \
    JSWord mtag: (JS_MTAG_BITS - 1)

typedef enum {
    JS_MTAG_FREE,
    JS_MTAG_OBJECT,
    JS_MTAG_FLOAT64,
    JS_MTAG_STRING,
    JS_MTAG_FUNCTION_BYTECODE,
    JS_MTAG_VALUE_ARRAY,
    JS_MTAG_BYTE_ARRAY,
    JS_MTAG_VARREF,
    JS_MTAG_COUNT,
} JSMemTagEnum;

struct JSContext {
    JSValue *sp;
    JSValue *fp;
    struct JSGCRef *top_gc_ref;
    struct JSGCRef *last_gc_ref;
    JSValue global_obj;
};

typedef struct JSCFunctionData {
    JSCFunction *func;
} JSCFunctionData;

typedef struct JSObject {
    JS_MB_HEADER;
    uint8_t class_id;
    uint32_t extra_size;
    JSValue proto;
    JSValue props;
    union {
        JSCFunctionData cfunc;
    } u;
} JSObject;

#endif
