#ifndef ALLOY_SHELL_LEXER_HPP
#define ALLOY_SHELL_LEXER_HPP

#include <string>
#include <vector>

namespace alloy {

enum class shell_token_type {
    word,
    pipe,          // |
    redir_in,      // <
    redir_out,     // >
    redir_append,  // >>
    redir_err,     // 2>
    redir_all,     // &>
    placeholder    // ${idx}
};

struct shell_token {
    shell_token_type type;
    std::string value;
};

class shell_lexer {
public:
    static std::vector<shell_token> tokenize(const std::string& input);
};

} // namespace alloy

#endif // ALLOY_SHELL_LEXER_HPP
