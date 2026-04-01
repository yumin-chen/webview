#ifndef ALLOY_IPC_HPP
#define ALLOY_IPC_HPP

#include "bindings.hpp"
#include "crypto.hpp"
#include <string>
#include <vector>

namespace alloy {

class IPC {
    std::vector<uint8_t> m_shared_secret;
    bool m_secured = false;

public:
    void set_shared_secret(const std::vector<uint8_t>& secret) {
        m_shared_secret = secret;
        m_secured = true;
    }

    std::string process_incoming(const std::string& msg) {
        if (!m_secured) return msg;
        return Crypto::decrypt(msg, m_shared_secret);
    }

    void send_outgoing(webview_t w, const std::string& msg) {
        std::string payload = msg;
        if (m_secured) {
            payload = Crypto::encrypt(msg, m_shared_secret);
        }
        // Send to webview via eval
        // webview_eval(w, ...);
    }
};

} // namespace alloy

#endif
