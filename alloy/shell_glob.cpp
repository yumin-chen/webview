#include <string>
#include <vector>
#include <filesystem>
#include <regex>

namespace alloy {

namespace fs = std::filesystem;

class shell_glob {
public:
    static std::vector<std::string> expand(const std::string& pattern) {
        std::vector<std::string> results;
        if (pattern.find('*') == std::string::npos && pattern.find('?') == std::string::npos) {
            results.push_back(pattern);
            return results;
        }

        // Extremely simplified glob to regex conversion for demonstration
        std::string regex_str = pattern;
        size_t pos = 0;
        while ((pos = regex_str.find("*", pos)) != std::string::npos) {
            regex_str.replace(pos, 1, ".*");
            pos += 2;
        }
        std::regex re(regex_str);

        for (const auto& entry : fs::directory_iterator(".")) {
            std::string filename = entry.path().filename().string();
            if (std::regex_match(filename, re)) {
                results.push_back(filename);
            }
        }

        if (results.empty()) results.push_back(pattern);
        return results;
    }

    static std::string escape(const std::string& s) {
        std::string res;
        for (char c : s) {
            if (c == '$' || c == '`' || c == '"' || c == '\\' || c == '\'' || c == ' ') {
                res += '\\';
            }
            res += c;
        }
        return res;
    }
};

} // namespace alloy
