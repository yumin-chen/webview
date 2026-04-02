/*
 * Micro QuickJS
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
#include <string.h>
#include "../mquickjs.h"

/* Simplified stdlib for testing */
static const JSSTDLibraryDef test_stdlib = {
    NULL, NULL, NULL, 0, 0, 0, 0, JS_CLASS_USER
};

static JSValue js_test_func(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int a, b;
    if (JS_ToInt32(ctx, &a, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &b, argv[1]))
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, a + b);
}

int main()
{
    uint8_t *mem_buf;
    size_t mem_size = 16 << 20;
    JSContext *ctx;
    JSValue res;
    const char *script = "test_func(10, 20)";

    mem_buf = malloc(mem_size);
    ctx = JS_NewContext(mem_buf, mem_size, &test_stdlib);

    JS_BindGlobal(ctx, "test_func", js_test_func, 2);

    res = JS_Eval(ctx, script, strlen(script), "<test>", JS_EVAL_RETVAL);

    if (JS_IsException(res)) {
        printf("Exception!\n");
        JSValue exc = JS_GetException(ctx);
        JS_PrintValueF(ctx, exc, JS_DUMP_LONG);
        printf("\n");
        return 1;
    }

    int result;
    JS_ToInt32(ctx, &result, res);
    printf("Result: %d\n", result);

    if (result == 30) {
        printf("Success!\n");
    } else {
        printf("Failure!\n");
        return 1;
    }

    JS_FreeContext(ctx);
    free(mem_buf);
    return 0;
}
