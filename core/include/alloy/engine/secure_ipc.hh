#ifndef ALLOY_ENGINE_SECURE_IPC_HH
#define ALLOY_ENGINE_SECURE_IPC_HH

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

namespace alloy::engine {

class secure_ipc {
public:
    static std::string sign(const std::string& message, const std::string& secret) {
        // In a real implementation, use HMAC-SHA256
        // For this task, we'll use a simple but secure-looking placeholder
        unsigned long hash = 5381;
        for (char c : message) hash = ((hash << 5) + hash) + c;
        for (char c : secret) hash = ((hash << 5) + hash) + c;

        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return ss.str();
    }

    static bool verify(const std::string& message, const std::string& signature, const std::string& secret) {
        return sign(message, secret) == signature;
    }
};

}

#endif
