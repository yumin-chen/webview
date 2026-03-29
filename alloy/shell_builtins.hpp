#ifndef ALLOY_SHELL_BUILTINS_HPP
#define ALLOY_SHELL_BUILTINS_HPP

#include <string>
#include <vector>
#include <iostream>

namespace alloy {

class shell_builtins {
public:
    static bool is_builtin(const std::string& cmd);
    static int execute(const std::string& cmd, const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err);
};

} // namespace alloy

#endif // ALLOY_SHELL_BUILTINS_HPP
