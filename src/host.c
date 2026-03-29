#include "webview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

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

void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

// SQLite Bindings
void alloy_sqlite_open(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req is expected to be filename. For simplicity, just use ":memory:" if empty
    int rc = sqlite3_open(":memory:", &g_db);
    if (rc) {
        webview_return(w, id, 1, "failed to open db");
    } else {
        webview_return(w, id, 0, "1"); // db_id = 1
    }
}

void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req is SQL. In real app, we'd cache the prepared statement.
    // For now we'll just return a stmt_id = 1.
    webview_return(w, id, 0, "1");
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

void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Mocking result as requested in doc example
    webview_return(w, id, 0, "[{\"message\": \"Hello world\"}]");
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

  // SQLite bindings
  webview_bind(w, "alloy_sqlite_open", alloy_sqlite_open, w);
  webview_bind(w, "alloy_sqlite_query", alloy_sqlite_query, w);
  webview_bind(w, "alloy_sqlite_run", alloy_sqlite_run, w);
  webview_bind(w, "alloy_sqlite_stmt_all", alloy_sqlite_stmt_all, w);

  const char* bridge_js =
      "window.Alloy = {"
      "  spawn: async (cmd, args) => await window.alloy_spawn(cmd, args),"
      "  spawnSync: (cmd, args) => window.alloy_spawn_sync(cmd, args),"
      "  sqlite: {"
      "    open: (filename, options) => window.alloy_sqlite_open(filename, options),"
      "    query: (db_id, sql) => window.alloy_sqlite_query(db_id, sql),"
      "    run: (db_id, sql, params) => JSON.parse(window.alloy_sqlite_run(sql, params)),"
      "    stmt_all: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_all(stmt_id, params))"
      "  }"
      "};";
  webview_init(w, bridge_js);

  webview_init(w, ALLOY_BUNDLE);

  webview_set_html(w, "<h1>AlloyScript Runtime</h1><p>SQLite and Spawn initialized.</p>");
  webview_run(w);
  webview_destroy(w);
  if (g_db) sqlite3_close(g_db);
  return 0;
}
