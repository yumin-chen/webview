#ifndef MQUICKJS_PRIV_H
#define MQUICKJS_PRIV_H

#include "cutils.h"
#include "list.h"
#include "mquickjs.h"

#define JSW 8 // Assuming 64-bit for now

typedef uintptr_t JSWord;

#define JS_MB_HEADER \
    JSWord gc_mark: 1; \
    JSWord mtag: (JS_MTAG_BITS - 1)

#define JS_MTAG_BITS 4

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

// ... add more private definitions from the code

#endif
