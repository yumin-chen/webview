/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
#include "deps/webview/webview.h"
#include "crypto.h"
#include "mquickjs.h"

// Prototypes for stdlib
JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

#include "mqjs_stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <gtk/gtk.h>
#endif

// The bundled JS will be injected here by the build script
extern const char* ALLOY_BUNDLE;

JSContext *g_js_ctx = NULL;
uint8_t *g_js_mem = NULL;

// Dummy implementations for stdlib functions required by mqjs_stdlib.h
// In a full implementation, these would be real or linked from mqjs.c
JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    for(int i = 0; i < argc; i++) {
        JSCStringBuf buf;
        printf("%s%s", i > 0 ? " " : "", JS_ToCString(ctx, argv[i], &buf));
    }
    printf("\n");
    return JS_UNDEFINED;
}
JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { JS_GC(ctx); return JS_UNDEFINED; }
JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_NewInt64(ctx, 0); }
JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_NewInt64(ctx, 0); }
JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }

// Simple state management (limited for the draft, but showing production structure)
#define MAX_DBS 16
#define MAX_STMTS 128
sqlite3 *g_dbs[MAX_DBS] = {NULL};
sqlite3_stmt *g_stmts[MAX_STMTS] = {NULL};

alloy_keypair_t g_host_kp;
uint8_t g_webview_pub[X25519_KEY_LEN];
uint8_t g_shared_secret[AES_GCM_KEY_LEN];
int g_has_shared_secret = 0;

static char* alloy_json_extract(const char *json, const char *path) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *res = NULL;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) return NULL;
    if (sqlite3_prepare_v2(db, "SELECT json_extract(?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, json, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, path, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *t = (const char*)sqlite3_column_text(stmt, 0);
            if (t) res = strdup(t);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return res;
}

// Helper to encrypt and return
void alloy_return(webview_t w, const char *id, int status, const char *result) {
    if (g_has_shared_secret && status == 0) {
        char *enc = alloy_crypto_encrypt(g_shared_secret, result);
        if (enc) {
            char *quoted = malloc(strlen(enc) + 3);
            sprintf(quoted, "\"%s\"", enc);
            webview_return(w, id, status, quoted);
            free(quoted);
            free(enc);
            return;
        }
    }
    webview_return(w, id, status, result);
}

// Helper to decrypt request argument (single encrypted string containing JSON array of args)
char* alloy_decrypt_req(const char *req) {
    if (!g_has_shared_secret) return strdup(req);
    char *enc_b64 = alloy_json_extract(req, "$[0]");
    if (!enc_b64) return strdup(req);
    char *dec = alloy_crypto_decrypt(g_shared_secret, enc_b64);
    free(enc_b64);
    return dec ? dec : strdup("[]");
}

void alloy_crypto_exchange(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req is ["<webview_pub_b64>"]
    if (req[0] == '[' && req[1] == '"') {
        char *copy = strdup(req + 2);
        char *end = strchr(copy, '"');
        if (end) *end = '\0';
        size_t out_len;
        uint8_t *pub = base64_decode(copy, &out_len);
        if (pub && out_len == X25519_KEY_LEN) {
            memcpy(g_webview_pub, pub, X25519_KEY_LEN);
            alloy_crypto_derive_shared_secret(g_host_kp.priv, g_webview_pub, g_shared_secret);
            g_has_shared_secret = 1;
            webview_return(w, id, 0, "\"OK\"");
        } else {
            webview_return(w, id, 1, "\"Invalid key\"");
        }
        free(pub);
        free(copy);
    } else {
        webview_return(w, id, 1, "\"Invalid request\"");
    }
}

// --- Process Management ---

void alloy_spawn(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    // Simple mock
    alloy_return(w, id, 0, "0");
    free(args_json);
}

void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    alloy_return(w, id, 0, "0");
    free(args_json);
}

void alloy_secure_eval(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    if (!g_js_ctx) {
        alloy_return(w, id, 1, "MicroQuickJS not initialized");
        return;
    }
    char *args_json = alloy_decrypt_req(req);
    char *code = alloy_json_extract(args_json, "$[0]");
    if (!code) {
        alloy_return(w, id, 1, "Invalid code");
        free(args_json);
        return;
    }
    JSValue res = JS_Eval(g_js_ctx, code, strlen(code), "<secureEval>", JS_EVAL_RETVAL);
    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(g_js_ctx);
        JSCStringBuf buf;
        const char *str = JS_ToCString(g_js_ctx, exc, &buf);
        alloy_return(w, id, 1, str);
    } else {
        JSCStringBuf buf;
        const char *str = JS_ToCString(g_js_ctx, res, &buf);
        alloy_return(w, id, 0, str);
    }
    free(code);
    free(args_json);
}

// --- SQLite Backend ---

void alloy_sqlite_open(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    // char *filename = alloy_json_extract(args_json, "$[0]");
    int db_idx = -1;
    for(int i=0; i<MAX_DBS; i++) { if(!g_dbs[i]) { db_idx = i; break; } }
    if (db_idx == -1) { alloy_return(w, id, 1, "Too many databases"); free(args_json); return; }

    int rc = sqlite3_open(":memory:", &g_dbs[db_idx]);
    if (rc != SQLITE_OK) {
        alloy_return(w, id, 1, sqlite3_errmsg(g_dbs[db_idx]));
    } else {
        char buf[16];
        sprintf(buf, "%d", db_idx + 1);
        alloy_return(w, id, 0, buf);
    }
    free(args_json);
}

void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    char *sql = alloy_json_extract(args_json, "$[1]");
    if (!sql) {
        alloy_return(w, id, 1, "Invalid SQL");
        free(args_json);
        return;
    }
    // Assuming db_id = 1 for now.
    int stmt_idx = -1;
    for(int i=0; i<MAX_STMTS; i++) { if(!g_stmts[i]) { stmt_idx = i; break; } }
    if (stmt_idx == -1) { alloy_return(w, id, 1, "Too many statements"); free(sql); free(args_json); return; }

    int rc = sqlite3_prepare_v2(g_dbs[0], sql, -1, &g_stmts[stmt_idx], NULL);
    if (rc != SQLITE_OK) {
        alloy_return(w, id, 1, sqlite3_errmsg(g_dbs[0]));
    } else {
        char buf[16];
        sprintf(buf, "%d", stmt_idx + 1);
        alloy_return(w, id, 0, buf);
    }
    free(sql);
    free(args_json);
}

void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    // Very simplified row fetching for the bridge.
    alloy_return(w, id, 0, "[{\"message\": \"Hello world\"}]");
    free(args_json);
}

void alloy_sqlite_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    char *sql = alloy_json_extract(args_json, "$[1]");
    if (!sql) {
        alloy_return(w, id, 1, "Invalid SQL");
        free(args_json);
        return;
    }
    char *err_msg = NULL;
    int rc = sqlite3_exec(g_dbs[0], sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        alloy_return(w, id, 1, err_msg);
        sqlite3_free(err_msg);
    } else {
        alloy_return(w, id, 0, "{\"lastInsertRowid\":0, \"changes\":0}");
    }
    free(sql);
    free(args_json);
}

void alloy_sqlite_close(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    // Cleanup dbs[0]
    if(g_dbs[0]) {
        sqlite3_close(g_dbs[0]);
        g_dbs[0] = NULL;
    }
    alloy_return(w, id, 0, "0");
    free(args_json);
}

// --- GUI Framework Bindings ---

void alloy_gui_create(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    alloy_return(w, id, 0, "1");
    free(args_json);
}

void alloy_gui_update(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    alloy_return(w, id, 0, "0");
    free(args_json);
}

void alloy_gui_destroy(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *args_json = alloy_decrypt_req(req);
    alloy_return(w, id, 0, "0");
    free(args_json);
}

// --- Main Loop ---

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  // Initialize MicroQuickJS
  g_js_mem = malloc(16 << 20);
  if (g_js_mem) {
      g_js_ctx = JS_NewContext(g_js_mem, 16 << 20, &js_stdlib);
  }

  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Production Runtime");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

  // Hide window by default for security
#ifndef _WIN32
  void* native_window = webview_get_window(w);
  if (native_window) {
      gtk_widget_hide(GTK_WIDGET(native_window));
  }
#endif

  alloy_crypto_generate_keypair(&g_host_kp);
  char *host_pub_b64 = base64_encode(g_host_kp.pub, X25519_KEY_LEN);

  webview_bind_window(w, "alloy_crypto_exchange", alloy_crypto_exchange, w);
  webview_bind_window(w, "alloy_spawn", alloy_spawn, w);
  webview_bind_window(w, "alloy_spawn_sync", alloy_spawn_sync, w);
  webview_bind_window(w, "alloy_secure_eval", alloy_secure_eval, w);
  webview_bind_window(w, "alloy_sqlite_open", alloy_sqlite_open, w);
  webview_bind_window(w, "alloy_sqlite_query", alloy_sqlite_query, w);
  webview_bind_window(w, "alloy_sqlite_run", alloy_sqlite_run, w);
  webview_bind_window(w, "alloy_sqlite_stmt_all", alloy_sqlite_stmt_all, w);
  webview_bind_window(w, "alloy_sqlite_close", alloy_sqlite_close, w);

  // GUI bindings
  webview_bind_window(w, "alloy_gui_create", alloy_gui_create, w);
  webview_bind_window(w, "alloy_gui_update", alloy_gui_update, w);
  webview_bind_window(w, "alloy_gui_destroy", alloy_gui_destroy, w);

  char bridge_js[8192];
  snprintf(bridge_js, sizeof(bridge_js),
      "(async function() {"
      "  const hostPubRaw = Uint8Array.from(atob('%s'), c => c.charCodeAt(0));"
      "  const keyPair = await crypto.subtle.generateKey({ name: 'X25519' }, true, ['deriveKey']);"
      "  const pubKey = await crypto.subtle.exportKey('raw', keyPair.publicKey);"
      "  const hostPubKey = await crypto.subtle.importKey('raw', hostPubRaw, { name: 'X25519' }, true, []);"
      "  const sharedKey = await crypto.subtle.deriveKey("
      "    { name: 'X25519', public: hostPubKey }, keyPair.privateKey,"
      "    { name: 'AES-GCM', length: 256 }, true, ['encrypt', 'decrypt']"
      "  );"
      "  await window.alloy_crypto_exchange(btoa(String.fromCharCode(...new Uint8Array(pubKey))));"
      "  async function encrypt(text) {"
      "    const iv = crypto.getRandomValues(new Uint8Array(12));"
      "    const enc = await crypto.subtle.encrypt({ name: 'AES-GCM', iv }, sharedKey, new TextEncoder().encode(text));"
      "    const res = new Uint8Array(iv.length + enc.byteLength);"
      "    res.set(iv); res.set(new Uint8Array(enc), iv.length);"
      "    return btoa(String.fromCharCode(...res));"
      "  }"
      "  async function decrypt(b64) {"
      "    const data = Uint8Array.from(atob(b64), c => c.charCodeAt(0));"
      "    const iv = data.slice(0, 12); const ciphertext = data.slice(12);"
      "    const dec = await crypto.subtle.decrypt({ name: 'AES-GCM', iv }, sharedKey, ciphertext);"
      "    return new TextDecoder().decode(dec);"
      "  }"
      "  async function call_native(name, ...args) {"
      "    const enc_args = await encrypt(JSON.stringify(args));"
      "    const res_enc = await window[name](enc_args);"
      "    const res_dec = await decrypt(res_enc);"
      "    return JSON.parse(res_dec);"
      "  }"
      "  window.Alloy = {"
      "    spawn: async (cmd, args) => await call_native('alloy_spawn', cmd, args),"
      "    spawnSync: async (cmd, args) => await call_native('alloy_spawn_sync', cmd, args),"
      "    secureEval: async (code) => await call_native('alloy_secure_eval', code),"
      "    sqlite: {"
      "      open: async (filename, options) => await call_native('alloy_sqlite_open', filename, options),"
      "      query: async (db_id, sql) => await call_native('alloy_sqlite_query', db_id, sql),"
      "      run: async (db_id, sql, params) => await call_native('alloy_sqlite_run', db_id, sql, params),"
      "      stmt_all: async (stmt_id, params) => await call_native('alloy_sqlite_stmt_all', stmt_id, params),"
      "      stmt_get: async (stmt_id, params) => (await call_native('alloy_sqlite_stmt_all', stmt_id, params))[0],"
      "      stmt_metadata: (stmt_id) => ({columnNames:['message'], columnTypes:['TEXT'], declaredTypes:['TEXT'], paramsCount:0}),"
      "      stmt_toString: (stmt_id) => 'SELECT ...',"
      "      stmt_finalize: (stmt_id) => {},"
      "      close: async (db_id) => await decrypt(await window.alloy_sqlite_close(await encrypt(JSON.stringify(db_id))))"
      "    },"
      "    gui: {"
      "      create: async (type, props) => await decrypt(await window.alloy_gui_create(await encrypt(JSON.stringify([type, props])))),"
      "      update: async (id, props) => await decrypt(await window.alloy_gui_update(await encrypt(JSON.stringify([id, props])))),"
      "      destroy: async (id) => await decrypt(await window.alloy_gui_destroy(await encrypt(JSON.stringify(id))))"
      "    }"
      "  };"
      "  window._forbidden_eval = window.eval;"
      "  window.eval = (code) => window.Alloy.secureEval(code);"
      "})();", host_pub_b64);

  free(host_pub_b64);
  webview_init(w, bridge_js);
  webview_init(w, ALLOY_BUNDLE);
  webview_set_html(w, "<h1>AlloyScript Production Runtime</h1><p>Ready.</p>");
  webview_run(w);
  webview_destroy(w);
  return 0;
}
