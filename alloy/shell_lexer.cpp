#include "shell_lexer.hpp"
#include <sstream>
#include <cctype>

namespace alloy {

std::vector<shell_token> shell_lexer::tokenize(const std::string& input) {
    std::vector<shell_token> tokens;
    size_t i = 0;
    while (i < input.size()) {
        char c = input[i];
        if (std::isspace(c)) {
            i++;
            continue;
        }

        if (c == '|') {
            tokens.push_back({shell_token_type::pipe, "|"});
            i++;
        } else if (c == '<') {
            tokens.push_back({shell_token_type::redir_in, "<"});
            i++;
        } else if (c == '>') {
            if (i + 1 < input.size() && input[i + 1] == '>') {
                tokens.push_back({shell_token_type::redir_append, ">>"});
                i += 2;
            } else {
                tokens.push_back({shell_token_type::redir_out, ">"});
                i++;
            }
        } else if (c == '2' && i + 1 < input.size() && input[i + 1] == '>') {
             tokens.push_back({shell_token_type::redir_err, "2>"});
             i += 2;
        } else if (c == '&' && i + 1 < input.size() && input[i + 1] == '>') {
             tokens.push_back({shell_token_type::redir_all, "&>"});
             i += 2;
        } else if (c == '$' && i + 1 < input.size() && input[i + 1] == '{') {
            size_t start = i;
            i += 2;
            std::string idx;
            while (i < input.size() && std::isdigit(input[i])) {
                idx += input[i++];
            }
            if (i < input.size() && input[i] == '}') {
                tokens.push_back({shell_token_type::placeholder, idx});
                i++;
            } else {
                // Fallback as word if not valid placeholder
                tokens.push_back({shell_token_type::word, input.substr(start, i - start)});
            }
        } else {
            // Word tokenization (handling quotes and escapes)
            std::string word;
            bool in_single_quote = false;
            bool in_double_quote = false;
            while (i < input.size()) {
                char cur = input[i];
                if (!in_single_quote && !in_double_quote) {
                    if (std::isspace(cur) || cur == '|' || cur == '<' || cur == '>' || (cur == '$' && i + 1 < input.size() && input[i+1] == '{')) break;
                }

                if (cur == '\'' && !in_double_quote) {
                    in_single_quote = !in_single_quote;
                    i++;
                } else if (cur == '"' && !in_single_quote) {
                    in_double_quote = !in_double_quote;
                    i++;
                } else if (cur == '\\' && !in_single_quote && i + 1 < input.size()) {
                    word += input[i + 1];
                    i += 2;
                } else {
                    word += cur;
                    i++;
                }
            }
            if (!word.empty()) {
                tokens.push_back({shell_token_type::word, word});
            }
        }
    }
    return tokens;
}

} // namespace alloy
