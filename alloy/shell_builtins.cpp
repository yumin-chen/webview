#include "shell_builtins.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <unistd.h>

namespace alloy {

namespace fs = std::filesystem;

bool shell_builtins::is_builtin(const std::string& cmd) {
    static const std::vector<std::string> builtins = {
        "cd", "ls", "rm", "echo", "pwd", "mkdir", "touch", "cat", "which", "mv", "exit", "true", "false", "yes", "seq", "dirname", "basename"
    };
    return std::find(builtins.begin(), builtins.end(), cmd) != builtins.end();
}

int shell_builtins::execute(const std::string& cmd, const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err) {
    if (cmd == "cd") {
        if (args.empty()) return 0;
        try {
            fs::current_path(args[0]);
            return 0;
        } catch (const std::exception& e) {
            err << "cd: " << e.what() << std::endl;
            return 1;
        }
    } else if (cmd == "pwd") {
        out << fs::current_path().string() << std::endl;
        return 0;
    } else if (cmd == "ls") {
        std::string path = args.empty() ? "." : args[0];
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                out << entry.path().filename().string() << std::endl;
            }
            return 0;
        } catch (const std::exception& e) {
            err << "ls: " << e.what() << std::endl;
            return 1;
        }
    } else if (cmd == "mkdir") {
        if (args.empty()) return 1;
        try {
            fs::create_directories(args[0]);
            return 0;
        } catch (const std::exception& e) {
            err << "mkdir: " << e.what() << std::endl;
            return 1;
        }
    } else if (cmd == "rm") {
        if (args.empty()) return 1;
        try {
            for (const auto& arg : args) {
                fs::remove_all(arg);
            }
            return 0;
        } catch (const std::exception& e) {
            err << "rm: " << e.what() << std::endl;
            return 1;
        }
    } else if (cmd == "touch") {
        if (args.empty()) return 1;
        try {
            for (const auto& arg : args) {
                std::ofstream ofs(arg, std::ios::app);
            }
            return 0;
        } catch (const std::exception& e) {
            err << "touch: " << e.what() << std::endl;
            return 1;
        }
    } else if (cmd == "mv") {
        if (args.size() < 2) return 1;
        try {
            fs::rename(args[0], args[1]);
            return 0;
        } catch (const std::exception& e) {
            err << "mv: " << e.what() << std::endl;
            return 1;
        }
    } else if (cmd == "echo") {
        for (size_t i = 0; i < args.size(); ++i) {
            out << args[i] << (i + 1 < args.size() ? " " : "");
        }
        out << std::endl;
        return 0;
    } else if (cmd == "cat") {
        if (args.empty()) {
            out << in.rdbuf();
            return 0;
        }
        for (const auto& arg : args) {
            std::ifstream ifs(arg);
            if (ifs) out << ifs.rdbuf();
            else { err << "cat: " << arg << ": No such file or directory" << std::endl; return 1; }
        }
        return 0;
    } else if (cmd == "dirname") {
        if (args.empty()) return 1;
        out << fs::path(args[0]).parent_path().string() << std::endl;
        return 0;
    } else if (cmd == "basename") {
        if (args.empty()) return 1;
        out << fs::path(args[0]).filename().string() << std::endl;
        return 0;
    } else if (cmd == "true") {
        return 0;
    } else if (cmd == "false") {
        return 1;
    } else if (cmd == "exit") {
        exit(args.empty() ? 0 : std::stoi(args[0]));
    } else if (cmd == "yes") {
        std::string s = args.empty() ? "y" : args[0];
        while (true) out << s << std::endl;
    } else if (cmd == "seq") {
        if (args.empty()) return 1;
        int end = std::stoi(args[0]);
        for (int i = 1; i <= end; ++i) out << i << std::endl;
        return 0;
    }
    return -1; // Not handled here
}

} // namespace alloy
