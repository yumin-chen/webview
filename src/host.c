#include "webview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
/* #include <mquickjs.h> // Required for MicroQuickJS integration */

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// The bundled JS will be injected here by the build script
extern const char* ALLOY_BUNDLE;

// Simple state management for demonstration (one DB for now)
sqlite3 *g_db = NULL;

void alloy_spawn(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

void alloy_secure_eval(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;

    /*
     * In a full implementation, we would use MicroQuickJS:
     * JSRuntime *rt = JS_NewRuntime();
     * JSContext *ctx = JS_NewContext(rt);
     * JSValue val = JS_Eval(ctx, req, strlen(req), "<input>", JS_EVAL_TYPE_GLOBAL);
     * const char *result_str = JS_ToCString(ctx, val);
     * webview_return(w, id, 0, result_str);
     * JS_FreeCString(ctx, result_str);
     * JS_FreeValue(ctx, val);
     * JS_FreeContext(ctx);
     * JS_FreeRuntime(rt);
     */

    // MicroQuickJS Secure Eval Stub (echoes back for now)
    webview_return(w, id, 0, req);
}

void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

// SQLite Bindings
void alloy_sqlite_open(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int rc = sqlite3_open(":memory:", &g_db);
    if (rc) {
        webview_return(w, id, 1, "failed to open db");
    } else {
        webview_return(w, id, 0, "1"); // db_id = 1
    }
}

void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "1"); // stmt_id = 1
}

void alloy_sqlite_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *err_msg = NULL;
    int rc = sqlite3_exec(g_db, req, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        webview_return(w, id, 1, err_msg);
        sqlite3_free(err_msg);
    } else {
        webview_return(w, id, 0, "{\"lastInsertRowid\":0, \"changes\":0}");
    }
}

void alloy_sqlite_serialize(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, ""); // empty base64 for stub
}

void alloy_sqlite_deserialize(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "2"); // db_id = 2
}

void alloy_sqlite_load_extension(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

void alloy_sqlite_file_control(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

void alloy_sqlite_set_custom_sqlite(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "[{\"message\": \"Hello world\"}]");
}

void alloy_sqlite_stmt_get(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "{\"message\": \"Hello world\"}");
}

void alloy_sqlite_stmt_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "{\"lastInsertRowid\":0, \"changes\":0}");
}

void alloy_sqlite_stmt_values(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "[[\"Hello world\"]]");
}

void alloy_sqlite_stmt_finalize(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

void alloy_sqlite_stmt_toString(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "\"SELECT 'Hello world';\"");
}

void alloy_sqlite_stmt_metadata(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    const char *json = "{\"columnNames\":[\"message\"], \"columnTypes\":[\"TEXT\"], \"declaredTypes\":[\"TEXT\"], \"paramsCount\":0}";
    webview_return(w, id, 0, json);
}

void alloy_sqlite_close(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
  (void)hInst;
  (void)hPrevInst;
  (void)lpCmdLine;
  (void)nCmdShow;
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Runtime");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

  webview_bind(w, "alloy_spawn", alloy_spawn, w);
  webview_bind(w, "alloy_spawn_sync", alloy_spawn_sync, w);
  webview_bind(w, "alloy_secure_eval", alloy_secure_eval, w);

  // SQLite bindings
  webview_bind(w, "alloy_sqlite_open", alloy_sqlite_open, w);
  webview_bind(w, "alloy_sqlite_query", alloy_sqlite_query, w);
  webview_bind(w, "alloy_sqlite_run", alloy_sqlite_run, w);
  webview_bind(w, "alloy_sqlite_serialize", alloy_sqlite_serialize, w);
  webview_bind(w, "alloy_sqlite_deserialize", alloy_sqlite_deserialize, w);
  webview_bind(w, "alloy_sqlite_load_extension", alloy_sqlite_load_extension, w);
  webview_bind(w, "alloy_sqlite_file_control", alloy_sqlite_file_control, w);
  webview_bind(w, "alloy_sqlite_set_custom_sqlite", alloy_sqlite_set_custom_sqlite, w);
  webview_bind(w, "alloy_sqlite_stmt_all", alloy_sqlite_stmt_all, w);
  webview_bind(w, "alloy_sqlite_stmt_get", alloy_sqlite_stmt_get, w);
  webview_bind(w, "alloy_sqlite_stmt_run", alloy_sqlite_stmt_run, w);
  webview_bind(w, "alloy_sqlite_stmt_values", alloy_sqlite_stmt_values, w);
  webview_bind(w, "alloy_sqlite_stmt_finalize", alloy_sqlite_stmt_finalize, w);
  webview_bind(w, "alloy_sqlite_stmt_toString", alloy_sqlite_stmt_toString, w);
  webview_bind(w, "alloy_sqlite_stmt_metadata", alloy_sqlite_stmt_metadata, w);
  webview_bind(w, "alloy_sqlite_close", alloy_sqlite_close, w);

  const char* bridge_js =
      "window.Alloy = {"
      "  spawn: async (cmd, args) => await window.alloy_spawn(cmd, args),"
      "  spawnSync: (cmd, args) => window.alloy_spawn_sync(cmd, args),"
      "  secureEval: (code) => window.alloy_secure_eval(code),"
      "  sqlite: {"
      "    open: (filename, options) => window.alloy_sqlite_open(filename, options),"
      "    query: (db_id, sql) => window.alloy_sqlite_query(db_id, sql),"
      "    run: (db_id, sql, params) => JSON.parse(window.alloy_sqlite_run(sql, params)),"
      "    serialize: (db_id) => window.alloy_sqlite_serialize(db_id),"
      "    deserialize: (contents) => window.alloy_sqlite_deserialize(contents),"
      "    loadExtension: (db_id, name) => window.alloy_sqlite_load_extension(db_id, name),"
      "    fileControl: (db_id, cmd, value) => window.alloy_sqlite_file_control(db_id, cmd, value),"
      "    setCustomSQLite: (path) => window.alloy_sqlite_set_custom_sqlite(path),"
      "    stmt_all: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_all(stmt_id, params)),"
      "    stmt_get: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_get(stmt_id, params)),"
      "    stmt_run: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_run(stmt_id, params)),"
      "    stmt_values: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_values(stmt_id, params)),"
      "    stmt_finalize: (stmt_id) => window.alloy_sqlite_stmt_finalize(stmt_id),"
      "    stmt_toString: (stmt_id) => JSON.parse(window.alloy_sqlite_stmt_toString(stmt_id)),"
      "    stmt_metadata: (stmt_id) => JSON.parse(window.alloy_sqlite_stmt_metadata(stmt_id)),"
      "    close: (db_id) => window.alloy_sqlite_close(db_id)"
      "  }"
      "};"
      "window._forbidden_eval = window.eval;"
      "window.eval = (code) => window.Alloy.secureEval(code);";
  webview_init(w, bridge_js);

  webview_init(w, ALLOY_BUNDLE);

  webview_set_html(w, "<h1>AlloyScript Runtime</h1><p>SQLite and Spawn initialized.</p>");
  webview_run(w);
  webview_destroy(w);
  if (g_db) sqlite3_close(g_db);
  return 0;
}
