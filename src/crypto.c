/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <string.h>

int alloy_crypto_generate_keypair(alloy_keypair_t *kp) {
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!pctx) return -1;
    if (EVP_PKEY_keygen_init(pctx) <= 0) { EVP_PKEY_CTX_free(pctx); return -1; }
    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) { EVP_PKEY_CTX_free(pctx); return -1; }

    size_t len = X25519_KEY_LEN;
    EVP_PKEY_get_raw_public_key(pkey, kp->pub, &len);
    EVP_PKEY_get_raw_private_key(pkey, kp->priv, &len);

    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(pctx);
    return 0;
}

int alloy_crypto_derive_shared_secret(const uint8_t priv[X25519_KEY_LEN], const uint8_t pub[X25519_KEY_LEN], uint8_t shared[X25519_KEY_LEN]) {
    EVP_PKEY *priv_key = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, priv, X25519_KEY_LEN);
    EVP_PKEY *pub_key = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, pub, X25519_KEY_LEN);
    if (!priv_key || !pub_key) return -1;

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(priv_key, NULL);
    if (!ctx) return -1;
    if (EVP_PKEY_derive_init(ctx) <= 0) return -1;
    if (EVP_PKEY_derive_set_peer(ctx, pub_key) <= 0) return -1;

    size_t shared_len = X25519_KEY_LEN;
    if (EVP_PKEY_derive(ctx, shared, &shared_len) <= 0) return -1;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(priv_key);
    EVP_PKEY_free(pub_key);
    return 0;
}

char* base64_encode(const uint8_t* buffer, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    char* res = (char*)malloc(bufferPtr->length + 1);
    memcpy(res, bufferPtr->data, bufferPtr->length);
    res[bufferPtr->length] = '\0';

    BIO_free_all(bio);
    return res;
}

uint8_t* base64_decode(const char* input, size_t* out_len) {
    BIO *bio, *b64;
    int decodeLen = strlen(input);
    uint8_t* buffer = (uint8_t*)malloc(decodeLen);

    bio = BIO_new_mem_buf(input, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    *out_len = BIO_read(bio, buffer, decodeLen);

    BIO_free_all(bio);
    return buffer;
}

char* alloy_crypto_encrypt(const uint8_t key[AES_GCM_KEY_LEN], const char *plaintext) {
    EVP_CIPHER_CTX *ctx = NULL;
    int len;
    int plaintext_len = strlen(plaintext);
    uint8_t iv[AES_GCM_IV_LEN];
    char *res = NULL;
    RAND_bytes(iv, AES_GCM_IV_LEN);

    uint8_t *ciphertext = malloc(plaintext_len + AES_GCM_TAG_LEN + AES_GCM_IV_LEN);
    if (!ciphertext) return NULL;
    memcpy(ciphertext, iv, AES_GCM_IV_LEN);

    if(!(ctx = EVP_CIPHER_CTX_new())) goto err;
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) goto err;
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) goto err;

    if(1 != EVP_EncryptUpdate(ctx, ciphertext + AES_GCM_IV_LEN, &len, (uint8_t*)plaintext, plaintext_len)) goto err;
    int ciphertext_len = len;

    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + AES_GCM_IV_LEN + len, &len)) goto err;
    ciphertext_len += len;

    uint8_t tag[AES_GCM_TAG_LEN];
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_LEN, tag)) goto err;
    memcpy(ciphertext + AES_GCM_IV_LEN + ciphertext_len, tag, AES_GCM_TAG_LEN);

    res = base64_encode(ciphertext, AES_GCM_IV_LEN + ciphertext_len + AES_GCM_TAG_LEN);

err:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return res;
}

char* alloy_crypto_decrypt(const uint8_t key[AES_GCM_KEY_LEN], const char *ciphertext_b64) {
    EVP_CIPHER_CTX *ctx = NULL;
    int len;
    size_t enc_len;
    uint8_t *enc_data = base64_decode(ciphertext_b64, &enc_len);
    if (!enc_data || enc_len < AES_GCM_IV_LEN + AES_GCM_TAG_LEN) { free(enc_data); return NULL; }

    uint8_t iv[AES_GCM_IV_LEN];
    memcpy(iv, enc_data, AES_GCM_IV_LEN);

    int ciphertext_len = enc_len - AES_GCM_IV_LEN - AES_GCM_TAG_LEN;
    uint8_t *plaintext = malloc(ciphertext_len + 1);
    char *res = NULL;
    if (!plaintext) { free(enc_data); return NULL; }

    if(!(ctx = EVP_CIPHER_CTX_new())) goto err;
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) goto err;
    if(1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) goto err;

    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, enc_data + AES_GCM_IV_LEN, ciphertext_len)) goto err;
    int decrypted_len = len;

    uint8_t tag[AES_GCM_TAG_LEN];
    memcpy(tag, enc_data + AES_GCM_IV_LEN + ciphertext_len, AES_GCM_TAG_LEN);
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_LEN, tag)) goto err;

    if(EVP_DecryptFinal_ex(ctx, plaintext + len, &len) <= 0) goto err;
    decrypted_len += len;
    plaintext[decrypted_len] = '\0';
    res = (char*)plaintext;

err:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (!res) free(plaintext);
    free(enc_data);
    return res;
}
