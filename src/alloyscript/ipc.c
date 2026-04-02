#include "ipc.h"
#include <string.h>
#include <stdlib.h>

void alloy_ipc_encrypt(AlloyMessage *msg, const unsigned char *key) {
    // Basic XOR encryption for placeholder
    for (size_t i = 0; i < msg->length; i++) {
        msg->data[i] ^= key[i % 32];
    }
}

void alloy_ipc_decrypt(AlloyMessage *msg, const unsigned char *key) {
    // XOR is symmetric
    alloy_ipc_encrypt(msg, key);
}

void alloy_ipc_send_to_webview(AlloyMessage *msg) {
    // In actual implementation, this would use webview_eval or similar
}

void alloy_ipc_receive_from_webview(AlloyMessage *msg) {
    // In actual implementation, this would handle webview callbacks
}
