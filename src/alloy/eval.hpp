#ifndef ALLOY_EVAL_HPP
#define ALLOY_EVAL_HPP

#include <string>
#include "../../core/mquickjs/mquickjs.h"

namespace alloy {

class Evaluator {
    JSContext *m_ctx;
public:
    Evaluator();
    ~Evaluator();
    std::string secure_eval(const std::string& script);
};

} // namespace alloy

#endif
