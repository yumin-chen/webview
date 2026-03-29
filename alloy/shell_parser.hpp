#ifndef ALLOY_SHELL_PARSER_HPP
#define ALLOY_SHELL_PARSER_HPP

#include "shell_lexer.hpp"
#include <vector>
#include <string>

namespace alloy {

struct shell_redirection {
    shell_token_type type;
    std::string target; // filename or placeholder index
    bool is_placeholder = false;
};

struct shell_command {
    std::vector<std::string> args;
    std::vector<shell_redirection> redirections;
    std::vector<int> arg_placeholders; // Indices in args that are placeholders
};

struct shell_pipeline {
    std::vector<shell_command> commands;
};

class shell_parser {
public:
    static shell_pipeline parse(const std::vector<shell_token>& tokens);
};

} // namespace alloy

#endif // ALLOY_SHELL_PARSER_HPP
