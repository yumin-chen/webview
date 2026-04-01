#ifndef ALLOY_CRYPTO_HPP
#define ALLOY_CRYPTO_HPP

#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

namespace alloy {

class Crypto {
public:
    static std::vector<uint8_t> generate_x25519_keypair(std::vector<uint8_t>& pub_key);
    static std::vector<uint8_t> derive_shared_secret(const std::vector<uint8_t>& priv_key, const std::vector<uint8_t>& peer_pub);
    static std::string encrypt(const std::string& plaintext, const std::vector<uint8_t>& key);
    static std::string decrypt(const std::string& ciphertext_with_iv_tag, const std::vector<uint8_t>& key);
};

} // namespace alloy

#endif
