#include "eval.hpp"

namespace alloy {

Evaluator::Evaluator() {
    // Initialize MicroQuickJS context
    // m_ctx = JS_NewContext(...);
}

Evaluator::~Evaluator() {
    // JS_FreeContext(m_ctx);
}

std::string Evaluator::secure_eval(const std::string& script) {
    // JS_Eval(m_ctx, script.c_str(), ...);
    return "Result for: " + script;
}

} // namespace alloy
