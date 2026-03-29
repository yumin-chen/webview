#include "shell_parser.hpp"
#include <stdexcept>

namespace alloy {

shell_pipeline shell_parser::parse(const std::vector<shell_token>& tokens) {
    shell_pipeline pipeline;
    shell_command current_cmd;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];

        if (token.type == shell_token_type::pipe) {
            pipeline.commands.push_back(current_cmd);
            current_cmd = shell_command();
        } else if (token.type == shell_token_type::redir_in ||
                   token.type == shell_token_type::redir_out ||
                   token.type == shell_token_type::redir_append ||
                   token.type == shell_token_type::redir_err ||
                   token.type == shell_token_type::redir_all) {
            if (i + 1 >= tokens.size()) {
                throw std::runtime_error("Missing redirection target");
            }
            const auto& next = tokens[++i];
            shell_redirection redir;
            redir.type = token.type;
            redir.target = next.value;
            redir.is_placeholder = (next.type == shell_token_type::placeholder);
            current_cmd.redirections.push_back(redir);
        } else if (token.type == shell_token_type::placeholder) {
            current_cmd.arg_placeholders.push_back(current_cmd.args.size());
            current_cmd.args.push_back(token.value);
        } else if (token.type == shell_token_type::word) {
            current_cmd.args.push_back(token.value);
        }
    }

    if (!current_cmd.args.empty() || !current_cmd.redirections.empty()) {
        pipeline.commands.push_back(current_cmd);
    }

    return pipeline;
}

} // namespace alloy
