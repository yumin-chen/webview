/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
#ifndef ALLOY_CRYPTO_H
#define ALLOY_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#define AES_GCM_IV_LEN 12
#define AES_GCM_TAG_LEN 16
#define AES_GCM_KEY_LEN 32
#define X25519_KEY_LEN 32

typedef struct {
    uint8_t priv[X25519_KEY_LEN];
    uint8_t pub[X25519_KEY_LEN];
} alloy_keypair_t;

int alloy_crypto_generate_keypair(alloy_keypair_t *kp);
int alloy_crypto_derive_shared_secret(const uint8_t priv[X25519_KEY_LEN], const uint8_t pub[X25519_KEY_LEN], uint8_t shared[X25519_KEY_LEN]);

char* base64_encode(const uint8_t* buffer, size_t length);
uint8_t* base64_decode(const char* input, size_t* out_len);

// returns allocated base64 string or NULL
char* alloy_crypto_encrypt(const uint8_t key[AES_GCM_KEY_LEN], const char *plaintext);
// returns allocated plaintext string or NULL
char* alloy_crypto_decrypt(const uint8_t key[AES_GCM_KEY_LEN], const char *ciphertext_b64);

#endif
