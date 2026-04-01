#ifndef ALLOY_IPC_H
#define ALLOY_IPC_H

#include <stddef.h>

/**
 * Secure IPC between C host and WebView.
 * WebView is treated as hostile. All messages are encrypted.
 */

typedef struct {
    unsigned char *data;
    size_t length;
} AlloyMessage;

// Simple end-to-end encryption placeholder
void alloy_ipc_encrypt(AlloyMessage *msg, const unsigned char *key);
void alloy_ipc_decrypt(AlloyMessage *msg, const unsigned char *key);

// Secure dispatch logic
void alloy_ipc_send_to_webview(AlloyMessage *msg);
void alloy_ipc_receive_from_webview(AlloyMessage *msg);

#endif // ALLOY_IPC_H
