#ifndef MQUICKJS_BUILD_H
#define MQUICKJS_BUILD_H

#include "mquickjs.h"

typedef struct JSPropDef JSPropDef;
typedef struct JSClassDef JSClassDef;

enum {
    JS_DEF_PROP_DOUBLE,
    JS_DEF_PROP_STRING,
    JS_DEF_PROP_UNDEFINED,
    JS_DEF_PROP_NULL,
    JS_DEF_CFUNC,
    JS_DEF_CGETSET,
    JS_DEF_CLASS,
    JS_DEF_END,
};

struct JSPropDef {
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
        const JSClassDef *class1;
    } u;
};

struct JSClassDef {
    const char *name;
    int length;
    const char *func_name;
    int class_id;
    const JSPropDef *class_props;
    const JSPropDef *proto_props;
    const JSClassDef *parent_class;
    const char *finalizer_name;
    const char *class_id_str; // For ROM builder
    const char *cproto_name;
};

#define JS_CFUNC_DEF(name, length, func1) { JS_DEF_CFUNC, name, .u.func = { length, "0", "generic", #func1 } }
#define JS_CFUNC_MAGIC_DEF(name, length, func1, magic) { JS_DEF_CFUNC, name, .u.func = { length, #magic, "generic_magic", #func1 } }
#define JS_CFUNC_SPECIAL_DEF(name, length, proto, func1) { JS_DEF_CFUNC, name, .u.func = { length, "0", #proto, #func1 } }
#define JS_PROP_DOUBLE_DEF(name, val, flags) { JS_DEF_PROP_DOUBLE, name, .u.f64 = val }
#define JS_PROP_STRING_DEF(name, val, flags) { JS_DEF_PROP_STRING, name, .u.str = val }
#define JS_PROP_UNDEFINED_DEF(name, flags) { JS_DEF_PROP_UNDEFINED, name }
#define JS_PROP_NULL_DEF(name, flags) { JS_DEF_PROP_NULL, name }
#define JS_PROP_END { JS_DEF_END }

#define JS_CLASS_DEF(name, length, ctor, class_id, class_props, proto_props, parent, finalizer) \
    { name, length, #ctor, class_id, class_props, proto_props, parent, #finalizer, #class_id, "generic" }

int build_atoms(const char *stdlib_name, const JSPropDef *global_obj,
                const JSPropDef *c_function_decl, int argc, char **argv);

#endif
