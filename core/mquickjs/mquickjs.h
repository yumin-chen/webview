#ifndef MQUICKJS_H
#define MQUICKJS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JSContext JSContext;
typedef uint32_t JSValue;
typedef int BOOL;

#define JS_VALUE_GET_TAG(v) ((v) & 7)
#define JS_VALUE_GET_INT(v) ((int32_t)(v) >> 1)
#define JS_VALUE_IS_PTR(v) (((v) & 1) != 0)

#define JS_TAG_INT 0
#define JS_TAG_SPECIAL 2
#define JS_TAG_SHORT_FLOAT 4
#define JS_TAG_STRING_CHAR 6

#define JS_TAG_NULL (JS_TAG_SPECIAL | (0 << 2))
#define JS_TAG_UNDEFINED (JS_TAG_SPECIAL | (1 << 2))
#define JS_TAG_BOOL (JS_TAG_SPECIAL | (2 << 2))
#define JS_TAG_UNINITIALIZED (JS_TAG_SPECIAL | (3 << 2))
#define JS_TAG_EXCEPTION (JS_TAG_SPECIAL | (4 << 2))
#define JS_TAG_CATCH_OFFSET (JS_TAG_SPECIAL | (5 << 2))
#define JS_TAG_SHORT_FUNC (JS_TAG_SPECIAL | (6 << 2))

#define JS_UNDEFINED JS_TAG_UNDEFINED
#define JS_NULL JS_TAG_NULL
#define JS_FALSE (JS_TAG_BOOL | (0 << 3))
#define JS_TRUE (JS_TAG_BOOL | (1 << 3))
#define JS_EXCEPTION (JS_TAG_EXCEPTION | (0 << 3))

typedef JSValue JSCFunction(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
typedef JSValue JSCFunctionMagic(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, int magic);
typedef JSValue JSCFunctionParams(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, JSValue params);

typedef union JSCFunctionUnion {
    JSCFunction *generic;
    JSCFunctionMagic *generic_magic;
    JSCFunctionParams *generic_params;
    double (*f_f)(double);
} JSCFunctionUnion;

typedef struct JSCFunctionDef {
    JSCFunctionUnion func;
    JSValue name;
    uint8_t def_type;
    uint8_t arg_count;
    int magic;
} JSCFunctionDef;

typedef void JSCFinalizer(JSContext *ctx, void *opaque);
typedef void JSWriteFunc(void *opaque, const void *buf, size_t buf_len);

typedef struct JSGCRef {
    struct JSGCRef *prev;
    JSValue val;
} JSGCRef;

struct JSSTDLibraryDef;

JSContext *JS_NewContext(void *mem_start, size_t mem_size, const struct JSSTDLibraryDef *stdlib_def);
void JS_FreeContext(JSContext *ctx);
JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags);
JSValue JS_Run(JSContext *ctx, JSValue val);
JSValue JS_Parse(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags);

#define JS_EVAL_RETVAL (1 << 0)
#define JS_EVAL_REPL   (1 << 1)
#define JS_EVAL_STRIP_COL (1 << 2)

JSValue JS_NewInt32(JSContext *ctx, int32_t val);
JSValue JS_NewInt64(JSContext *ctx, int64_t val);
JSValue JS_NewFloat64(JSContext *ctx, double d);
JSValue JS_NewString(JSContext *ctx, const char *str);
JSValue JS_NewStringLen(JSContext *ctx, const char *buf, size_t len);
JSValue JS_NewArray(JSContext *ctx, int initial_len);
JSValue JS_NewObject(JSContext *ctx);

JSValue JS_GetGlobalObject(JSContext *ctx);
JSValue JS_GetProperty(JSContext *ctx, JSValue obj, JSValue prop);
JSValue JS_GetPropertyStr(JSContext *ctx, JSValue this_obj, const char *str);
JSValue JS_GetPropertyUint32(JSContext *ctx, JSValue obj, uint32_t idx);
JSValue JS_SetPropertyInternal(JSContext *ctx, JSValue this_obj, JSValue prop, JSValue val, BOOL allow_tail_call);
JSValue JS_SetPropertyStr(JSContext *ctx, JSValue this_obj, const char *str, JSValue val);
JSValue JS_SetPropertyUint32(JSContext *ctx, JSValue this_obj, uint32_t idx, JSValue val);
JSValue JS_DefinePropertyValue(JSContext *ctx, JSValue obj, JSValue prop, JSValue val);

JSValue JS_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name, int arg_count);

int JS_ToInt32(JSContext *ctx, int *pres, JSValue val);
int JS_ToNumber(JSContext *ctx, double *pres, JSValue val);

typedef struct JSCStringBuf {
    uint8_t buf[256];
} JSCStringBuf;

const char *JS_ToCString(JSContext *ctx, JSValue val, struct JSCStringBuf *buf);

JSValue *JS_AddGCRef(JSContext *ctx, JSGCRef *ref);
void JS_DeleteGCRef(JSContext *ctx, JSGCRef *ref);

void JS_GC(JSContext *ctx);

typedef int JSInterruptHandler(JSContext *ctx, void *opaque);
void JS_SetInterruptHandler(JSContext *ctx, JSInterruptHandler *interrupt_handler);

#ifdef __cplusplus
}
#endif

#endif
