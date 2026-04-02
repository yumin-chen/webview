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
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mquickjs_build.h"

/* simple class example */

static const JSPropDef js_rectangle_proto[] = {
    JS_CGETSET_DEF("x", js_rectangle_get_x, NULL ),
    JS_CGETSET_DEF("y", js_rectangle_get_y, NULL ),
    JS_PROP_END,
};

static const JSPropDef js_rectangle[] = {
    JS_CFUNC_DEF("getClosure", 1, js_rectangle_getClosure ),
    JS_CFUNC_DEF("call", 2, js_rectangle_call ),
    JS_PROP_END,
};

static const JSClassDef js_rectangle_class =
    JS_CLASS_DEF("Rectangle", 2, js_rectangle_constructor, JS_CLASS_RECTANGLE, js_rectangle, js_rectangle_proto, NULL, js_rectangle_finalizer);

static const JSPropDef js_filled_rectangle_proto[] = {
    JS_CGETSET_DEF("color", js_filled_rectangle_get_color, NULL ),
    JS_PROP_END,
};

/* inherit from Rectangle */
static const JSClassDef js_filled_rectangle_class =
    JS_CLASS_DEF("FilledRectangle", 3, js_filled_rectangle_constructor, JS_CLASS_FILLED_RECTANGLE, NULL, js_filled_rectangle_proto, &js_rectangle_class, js_filled_rectangle_finalizer);

/* include the full standard library too */

#define CONFIG_CLASS_EXAMPLE
#include "mqjs_stdlib.c"
