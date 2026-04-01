#include "crypto.hpp"
#include <openssl/rand.h>
#include <openssl/kdf.h>

namespace alloy {

std::vector<uint8_t> Crypto::generate_x25519_keypair(std::vector<uint8_t>& pub_key) {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_keygen_init(pctx);
    EVP_PKEY_keygen(pctx, &pkey);

    size_t pub_len = 0;
    EVP_PKEY_get_raw_public_key(pkey, NULL, &pub_len);
    pub_key.resize(pub_len);
    EVP_PKEY_get_raw_public_key(pkey, pub_key.data(), &pub_len);

    size_t priv_len = 0;
    EVP_PKEY_get_raw_private_key(pkey, NULL, &priv_len);
    std::vector<uint8_t> priv_key(priv_len);
    EVP_PKEY_get_raw_private_key(pkey, priv_key.data(), &priv_len);

    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(pctx);
    return priv_key;
}

std::vector<uint8_t> Crypto::derive_shared_secret(const std::vector<uint8_t>& priv_key, const std::vector<uint8_t>& peer_pub) {
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, priv_key.data(), priv_key.size());
    EVP_PKEY *peer_pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, peer_pub.data(), peer_pub.size());

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pkey, NULL);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_derive_set_peer(ctx, peer_pkey);

    size_t secret_len;
    EVP_PKEY_derive(ctx, NULL, &secret_len);
    std::vector<uint8_t> secret(secret_len);
    EVP_PKEY_derive(ctx, secret.data(), &secret_len);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peer_pkey);
    EVP_PKEY_free(pkey);
    return secret;
}

std::string Crypto::encrypt(const std::string& plaintext, const std::vector<uint8_t>& key) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> iv(12);
    RAND_bytes(iv.data(), iv.size());

    int len;
    std::vector<uint8_t> ciphertext(plaintext.size() + 16);
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key.data(), iv.data());
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (const uint8_t*)plaintext.data(), plaintext.size());
    int ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    uint8_t tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    EVP_CIPHER_CTX_free(ctx);

    std::string result((char*)iv.data(), iv.size());
    result.append((char*)tag, 16);
    result.append((char*)ciphertext.data(), ciphertext_len);
    return result;
}

std::string Crypto::decrypt(const std::string& data, const std::vector<uint8_t>& key) {
    if (data.size() < 28) return "";

    const uint8_t* iv = (const uint8_t*)data.data();
    const uint8_t* tag = (const uint8_t*)data.data() + 12;
    const uint8_t* ciphertext = (const uint8_t*)data.data() + 28;
    int ciphertext_len = data.size() - 28;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key.data(), iv);

    std::vector<uint8_t> plaintext(ciphertext_len);
    int len;
    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len);
    int plaintext_len = len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag);
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);

    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        plaintext_len += len;
        return std::string((char*)plaintext.data(), plaintext_len);
    }
    return "";
}

} // namespace alloy
