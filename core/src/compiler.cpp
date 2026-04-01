#include "alloy/api.h"
#include <stdlib.h>
#include <string.h>
#include <string>

// MicroQuickJS (mquickjs) mock implementation for bytecode compilation.
// In the final build, this would link against the actual mquickjs core JS_Compile.
extern "C" {

struct transpiler_impl {
    char *options;
};

alloy_transpiler_t alloy_transpiler_create(const char *options_json) {
    auto t = new transpiler_impl();
    t->options = strdup(options_json ? options_json : "{}");
    return static_cast<alloy_transpiler_t>(t);
}

alloy_error_t alloy_transpiler_transform(alloy_transpiler_t t,
                                          const char *code,
                                          const char *loader,
                                          char **out_result) {
    if (!t || !code || !out_result) return ALLOY_ERROR_INVALID_ARGUMENT;

    // Automatic polyfilling for AlloyScript target:
    // If target is "AlloyScript", we wrap modern APIs to use the proxy.
    // In a real implementation, this would use a parser to transform AST nodes.
    std::string res = "// alloy:polyfill\n";
    std::string source(code);

    if (source.find("fetch(") != std::string::npos) {
        res += "const fetch = (url) => Alloy.browserApiProxy({api:'fetch', args:[url]});\n";
    }
    if (source.find("async ") != std::string::npos) {
        res += "// async/await polyfilled via Service WebView bridge\n";
    }

    res += "// loader: " + std::string(loader ? loader : "js") + "\n" + source;
    *out_result = strdup(res.c_str());
    return ALLOY_OK;
}

alloy_error_t alloy_decompile_bytecode(const unsigned char *bytecode,
                                        size_t len,
                                        char **out_js) {
    if (!bytecode || !out_js) return ALLOY_ERROR_INVALID_ARGUMENT;

    // Simulation of bytecode reconstruction: remove "mquickjs_bytecode:" prefix
    std::string bc_str((const char*)bytecode, len);
    std::string prefix = "mquickjs_bytecode:";
    if (bc_str.find(prefix) == 0) {
        *out_js = strdup(bc_str.substr(prefix.length()).c_str());
    } else {
        *out_js = strdup(bc_str.c_str());
    }

    return ALLOY_OK;
}

alloy_error_t alloy_transpiler_scan(alloy_transpiler_t t,
                                     const char *code,
                                     char **out_json_result) {
    if (!t || !code || !out_json_result) return ALLOY_ERROR_INVALID_ARGUMENT;
    // Mock scan result
    *out_json_result = strdup("{\"exports\":[], \"imports\":[]}");
    return ALLOY_OK;
}

// MicroQuickJS bytecode compiler implementation
alloy_error_t alloy_build_bytecode(const char *source,
                                    unsigned char **out_bytecode,
                                    size_t *out_len) {
    if (!source || !out_bytecode || !out_len) return ALLOY_ERROR_INVALID_ARGUMENT;

    // In production, this calls the MicroQuickJS core API:
    // JSContext *ctx = ...;
    // JSValue obj = JS_Compile(ctx, source, strlen(source), "<input>", JS_PARSE_MODE_MODULE);
    // *out_bytecode = JS_WriteObject(ctx, out_len, obj, JS_WRITE_OBJ_BYTECODE);

    std::string mock_bc = "mquickjs_bytecode:" + std::string(source);
    *out_len = mock_bc.length();
    *out_bytecode = (unsigned char*)malloc(*out_len);
    memcpy(*out_bytecode, mock_bc.data(), *out_len);

    return ALLOY_OK;
}

void alloy_transpiler_destroy(alloy_transpiler_t t) {
    if (!t) return;
    auto impl = static_cast<transpiler_impl*>(t);
    free(impl->options);
    delete impl;
}

}
