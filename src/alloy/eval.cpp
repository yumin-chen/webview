#include "eval.hpp"
#include "bindings.hpp"

namespace alloy {

Evaluator::Evaluator() {
    // In a real implementation, we would allocate memory and call JS_NewContext
    m_ctx = (JSContext*)malloc(1024); // Mocked allocation
}

Evaluator::~Evaluator() {
    free(m_ctx);
}

std::string Evaluator::secure_eval(const std::string& script) {
    // In a real implementation:
    // JSValue val = JS_Eval(m_ctx, script.c_str(), script.size(), "<secureEval>", JS_EVAL_RETVAL);
    // return JS_ToCString(m_ctx, val, ...);

    return "MicroQuickJS Execution Mock: " + script;
}

} // namespace alloy
