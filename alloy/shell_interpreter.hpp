#ifndef ALLOY_SHELL_INTERPRETER_HPP
#define ALLOY_SHELL_INTERPRETER_HPP

#include "shell_parser.hpp"
#include <string>
#include <map>

namespace alloy {

struct shell_result {
    int exit_code;
    std::string stdout_data;
    std::string stderr_data;
};

class shell_interpreter {
public:
    static shell_result execute(const shell_pipeline& pipeline, const std::map<std::string, std::string>& env, const std::string& cwd, const std::vector<std::string>& placeholders);
};

} // namespace alloy

#endif // ALLOY_SHELL_INTERPRETER_HPP
