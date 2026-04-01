/*
 * Micro QuickJS private definitions
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
#ifndef MQUICKJS_PRIV_H
#define MQUICKJS_PRIV_H

#include <stdlib.h>
#include <stdint.h>
#include "mquickjs.h"
#include "cutils.h"

#if defined(__x86_64__) || defined(__arm64__) || defined(__aarch64__)
#define JSW 8
#define JS_PTR64
#else
#define JSW 4
#endif

typedef uintptr_t JSWord;

#define JS_MTAG_BITS 4
#define JS_MTAG_COUNT (1 << JS_MTAG_BITS)

typedef enum {
    JS_MTAG_FREE,
    JS_MTAG_OBJECT,
    JS_MTAG_FLOAT64,
    JS_MTAG_STRING,
    JS_MTAG_FUNCTION_BYTECODE,
    JS_MTAG_VALUE_ARRAY,
    JS_MTAG_BYTE_ARRAY,
    JS_MTAG_VARREF,
} JSMTagEnum;

#define JS_MB_HEADER  \
    JSWord gc_mark: 1; \
    JSWord mtag: (JS_MTAG_BITS - 1)

#define JS_VALUE_FROM_PTR(ptr) (JSValue)((uintptr_t)(ptr))
#define JS_VALUE_TO_PTR(v) (void *)((uintptr_t)(v))
#define JS_IsPtr(v) (((v) & 1) != 0)
#define JS_IS_ROM_PTR(ctx, ptr) (0)

#define JS_VALUE_GET_SPECIAL_TAG(v) ((v) & 0xf)
#define JS_VALUE_GET_SPECIAL_VALUE(v) ((int32_t)(v) >> 4)
#define JS_VALUE_MAKE_SPECIAL(tag, v) ((tag) | ((v) << 4))

#define JS_TAG_SPECIAL 1

#define JS_EX_CALL 1

typedef struct {
    uint32_t magic;
    uint16_t version;
    uintptr_t base_addr;
    JSValue unique_strings;
    JSValue main_func;
} JSBytecodeHeader;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t base_addr;
    JSValue unique_strings;
    JSValue main_func;
} JSBytecodeHeader32;

#define JS_BYTECODE_MAGIC 0x514a5301

#endif /* MQUICKJS_PRIV_H */
