/*
 * Micro QuickJS build utility header
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
#ifndef MQUICKJS_BUILD_H
#define MQUICKJS_BUILD_H

#include "mquickjs.h"

typedef enum {
    JS_DEF_PROP_DOUBLE,
    JS_DEF_PROP_STRING,
    JS_DEF_PROP_UNDEFINED,
    JS_DEF_PROP_NULL,
    JS_DEF_CFUNC,
    JS_DEF_CGETSET,
    JS_DEF_CLASS,
    JS_DEF_END,
} JSPropDefTypeEnum;

typedef struct JSPropDef {
    uint8_t def_type;
    const char *name;
    union {
        double f64;
        const char *str;
        struct {
            int length;
            const char *magic;
            const char *cproto_name;
            const char *func_name;
        } func;
        struct {
            const char *magic;
            const char *cproto_name;
            const char *get_func_name;
            const char *set_func_name;
        } getset;
        const struct JSClassDef *class1;
    } u;
} JSPropDef;

#define JS_PROP_END { JS_DEF_END }
#define JS_CFUNC_DEF(name, length, func) { JS_DEF_CFUNC, name, .u.func = { length, "0", "generic", #func } }
#define JS_CFUNC_MAGIC_DEF(name, length, func, magic) { JS_DEF_CFUNC, name, .u.func = { length, #magic, "generic_magic", #func } }
#define JS_CFUNC_SPECIAL_DEF(name, length, proto, func) { JS_DEF_CFUNC, name, .u.func = { length, "0", #proto, #func } }
#define JS_PROP_DOUBLE_DEF(name, val, flags) { JS_DEF_PROP_DOUBLE, name, .u.f64 = val }
#define JS_CGETSET_DEF(name, get_func, set_func) { JS_DEF_CGETSET, name, .u.getset = { "0", "generic", #get_func, #set_func } }
#define JS_CGETSET_MAGIC_DEF(name, get_func, set_func, magic) { JS_DEF_CGETSET, name, .u.getset = { #magic, "generic_magic", #get_func, #set_func } }
#define JS_PROP_STRING_DEF(name, val, flags) { JS_DEF_PROP_STRING, name, .u.str = val }
#define JS_PROP_UNDEFINED_DEF(name, flags) { JS_DEF_PROP_UNDEFINED, name }
#define JS_PROP_NULL_DEF(name, flags) { JS_DEF_PROP_NULL, name }
#define JS_PROP_CLASS_DEF(name, class_def) { JS_DEF_CLASS, name, .u.class1 = class_def }

typedef struct JSClassDef {
    const char *name;
    int length;
    const char *func_name;
    int class_id;
    const JSPropDef *class_props;
    const JSPropDef *proto_props;
    const struct JSClassDef *parent_class;
    const char *finalizer_name;
    const char *cproto_name;
    const char *class_id_str;
} JSClassDef;

#define JS_CLASS_DEF(name, length, constructor, class_id, class_props, proto_props, parent, finalizer) \
    { name, length, #constructor, class_id, class_props, proto_props, parent, #finalizer, "constructor", #class_id }

#define JS_CLASS_MAGIC_DEF(name, length, constructor, class_id, class_props, proto_props, parent, finalizer) \
    { name, length, #constructor, class_id, class_props, proto_props, parent, #finalizer, "constructor_magic", #class_id }

#define JS_OBJECT_DEF(name, props) \
    { name, 0, NULL, -1, props, NULL, NULL, NULL, NULL, NULL }

int build_atoms(const char *stdlib_name, const JSPropDef *global_obj, const JSPropDef *c_function_decl, int argc, char **argv);

#endif /* MQUICKJS_BUILD_H */
