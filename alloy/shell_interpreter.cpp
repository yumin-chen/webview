#include "shell_interpreter.hpp"
#include "shell_builtins.hpp"
#include "subprocess.hpp"
#include <sstream>
#include <iostream>
#include <filesystem>
#include <future>

namespace alloy {

shell_result shell_interpreter::execute(const shell_pipeline& pipeline, const std::map<std::string, std::string>& env, const std::string& cwd, const std::vector<std::string>& placeholders) {
    shell_result result = {0, "", ""};

    if (pipeline.commands.empty()) return result;

    // Simple implementation: sequential execution for now,
    // real shell would use pipes between processes.
    // For this design, we'll simulate a single pipeline.

    std::string current_input = "";

    for (size_t i = 0; i < pipeline.commands.size(); ++i) {
        const auto& cmd = pipeline.commands[i];
        std::vector<std::string> args = cmd.args;

        // Resolve placeholders in args
        for (int idx : cmd.arg_placeholders) {
            int p_idx = std::stoi(args[idx]);
            if (p_idx < placeholders.size()) {
                args[idx] = placeholders[p_idx];
            }
        }

        std::stringstream in(current_input);
        std::stringstream out;
        std::stringstream err;

        if (shell_builtins::is_builtin(args[0])) {
            result.exit_code = shell_builtins::execute(args[0], std::vector<std::string>(args.begin() + 1, args.end()), in, out, err);
        } else {
            // Use subprocess for external commands
            spawn_options options;
            options.cwd = cwd;
            options.env = env;
            options.stdout_mode = "pipe";
            options.stderr_mode = "pipe";
            options.stdin_mode = "pipe";

            subprocess proc(args, options);
            proc.stdin_write(current_input);
            proc.stdin_end();

            auto res = proc.wait_sync();
            result.exit_code = res.exit_code;
            out << res.stdout_data;
            err << res.stderr_data;
        }

        current_input = out.str();
        if (i == pipeline.commands.size() - 1) {
            result.stdout_data = out.str();
            result.stderr_data += err.str();
        }
    }

    return result;
}

} // namespace alloy
