#include "alloy/api.h"
#include <stdlib.h>
#include <string.h>

// MicroQuickJS (mquickjs) mock implementation for bytecode compilation.
// In the final build, this would link against the actual mquickjs core.
extern "C" {

alloy_error_t alloy_build_bytecode(const char *source,
                                    unsigned char **out_bytecode,
                                    size_t *out_len) {
    if (!source || !out_bytecode || !out_len) return ALLOY_ERROR_INVALID_ARGUMENT;

    // Simulation of bytecode generation: "compiled:" + original source
    const char *prefix = "compiled:";
    size_t source_len = strlen(source);
    size_t prefix_len = strlen(prefix);
    *out_len = prefix_len + source_len;
    *out_bytecode = (unsigned char*)malloc(*out_len);

    memcpy(*out_bytecode, prefix, prefix_len);
    memcpy(*out_bytecode + prefix_len, source, source_len);

    return ALLOY_OK;
}

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
    // Mock transformation: wrap code with comments
    std::string res = "// loader: " + std::string(loader ? loader : "js") + "\n" + std::string(code);
    *out_result = strdup(res.c_str());
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

void alloy_transpiler_destroy(alloy_transpiler_t t) {
    if (!t) return;
    auto impl = static_cast<transpiler_impl*>(t);
    free(impl->options);
    delete impl;
}

}
