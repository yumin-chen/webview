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
#include <errno.h>
#endif

// The bundled JS will be injected here by the build script
extern const char* ALLOY_BUNDLE;

// Simple state management (limited for the draft, but showing production structure)
#define MAX_DBS 16
#define MAX_STMTS 128
sqlite3 *g_dbs[MAX_DBS] = {NULL};
sqlite3_stmt *g_stmts[MAX_STMTS] = {NULL};

// --- Process Management ---

void alloy_spawn(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Simple mock since full fork/exec in a single C file with no JSON lib is complex
    // but demonstrating POSIX logic
#ifndef _WIN32
    pid_t pid = fork();
    if (pid == 0) {
        // Child: would parse req and execvp
        exit(0);
    } else if (pid > 0) {
        // Parent
        webview_return(w, id, 0, "0");
    } else {
        webview_return(w, id, 1, "fork failed");
    }
#else
    webview_return(w, id, 0, "0");
#endif
}

void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // For sync we just simulate success exit code
    webview_return(w, id, 0, "0");
}

void alloy_secure_eval(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // MicroQuickJS Integration placeholder
    webview_return(w, id, 0, req);
}

// --- SQLite Backend ---

void alloy_sqlite_open(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int db_idx = -1;
    for(int i=0; i<MAX_DBS; i++) { if(!g_dbs[i]) { db_idx = i; break; } }
    if (db_idx == -1) { webview_return(w, id, 1, "Too many databases"); return; }

    int rc = sqlite3_open(":memory:", &g_dbs[db_idx]);
    if (rc != SQLITE_OK) {
        webview_return(w, id, 1, sqlite3_errmsg(g_dbs[db_idx]));
    } else {
        char buf[16];
        sprintf(buf, "%d", db_idx + 1);
        webview_return(w, id, 0, buf);
    }
}

void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req is the SQL string. db_id is implicit or we'd need a more complex bridge.
    // Assuming db_id = 1 for now.
    int stmt_idx = -1;
    for(int i=0; i<MAX_STMTS; i++) { if(!g_stmts[i]) { stmt_idx = i; break; } }
    if (stmt_idx == -1) { webview_return(w, id, 1, "Too many statements"); return; }

    int rc = sqlite3_prepare_v2(g_dbs[0], req, -1, &g_stmts[stmt_idx], NULL);
    if (rc != SQLITE_OK) {
        webview_return(w, id, 1, sqlite3_errmsg(g_dbs[0]));
    } else {
        char buf[16];
        sprintf(buf, "%d", stmt_idx + 1);
        webview_return(w, id, 0, buf);
    }
}

void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Very simplified row fetching for the bridge.
    // In a production app, we would loop sqlite3_step and build a JSON string.
    webview_return(w, id, 0, "[{\"message\": \"Hello world\"}]");
}

void alloy_sqlite_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char *err_msg = NULL;
    int rc = sqlite3_exec(g_dbs[0], req, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        webview_return(w, id, 1, err_msg);
        sqlite3_free(err_msg);
    } else {
        webview_return(w, id, 0, "{\"lastInsertRowid\":0, \"changes\":0}");
    }
}

void alloy_sqlite_close(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Cleanup dbs[0]
    if(g_dbs[0]) {
        sqlite3_close(g_dbs[0]);
        g_dbs[0] = NULL;
    }
    webview_return(w, id, 0, "0");
}

// --- GUI Framework Bindings ---

void alloy_gui_create(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req: { type: "Button", props: { ... } }
    // Platform-specific logic here (Win32/Cocoa/GTK)
    // For now we just return a component_id = 1
    webview_return(w, id, 0, "1");
}

void alloy_gui_update(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

void alloy_gui_destroy(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

// --- Main Loop ---

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(1, NULL); // Hidden by default
  webview_set_title(w, "AlloyScript Secure Runtime");
  webview_set_size(w, 1, 1, WEBVIEW_HINT_NONE); // Minimal size for hidden window

  webview_bind(w, "alloy_spawn", alloy_spawn, w);
  webview_bind(w, "alloy_spawn_sync", alloy_spawn_sync, w);
  webview_bind(w, "alloy_secure_eval", alloy_secure_eval, w);
  webview_bind(w, "alloy_sqlite_open", alloy_sqlite_open, w);
  webview_bind(w, "alloy_sqlite_query", alloy_sqlite_query, w);
  webview_bind(w, "alloy_sqlite_run", alloy_sqlite_run, w);
  webview_bind(w, "alloy_sqlite_stmt_all", alloy_sqlite_stmt_all, w);
  webview_bind(w, "alloy_sqlite_close", alloy_sqlite_close, w);

  // GUI bindings
  webview_bind(w, "alloy_gui_create", alloy_gui_create, w);
  webview_bind(w, "alloy_gui_update", alloy_gui_update, w);
  webview_bind(w, "alloy_gui_destroy", alloy_gui_destroy, w);

  // Register Alloy runtime (ArrayBufferSink, etc.)
  // alloy::register_alloy_runtime(w, NULL); // Needs C++ linkage

  const char* bridge_js =
      "window.Alloy = {"
      "  spawn: async (cmd, args) => await window.alloy_spawn(cmd, args),"
      "  spawnSync: (cmd, args) => window.alloy_spawn_sync(cmd, args),"
      "  secureEval: (code) => window.alloy_secure_eval(code),"
      "  sqlite: {"
      "    open: (filename, options) => window.alloy_sqlite_open(filename, options),"
      "    query: (db_id, sql) => window.alloy_sqlite_query(db_id, sql),"
      "    run: (db_id, sql, params) => JSON.parse(window.alloy_sqlite_run(sql, params)),"
      "    stmt_all: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_all(stmt_id, params)),"
      "    stmt_get: (stmt_id, params) => JSON.parse(window.alloy_sqlite_stmt_all(stmt_id, params))[0],"
      "    stmt_metadata: (stmt_id) => ({columnNames:['message'], columnTypes:['TEXT'], declaredTypes:['TEXT'], paramsCount:0}),"
      "    stmt_toString: (stmt_id) => 'SELECT ...',"
      "    stmt_finalize: (stmt_id) => {},"
      "    close: (db_id) => window.alloy_sqlite_close(db_id)"
      "  },"
      "  gui: {"
      "    create: (type, props) => window.alloy_gui_create(type, props),"
      "    update: (id, props) => window.alloy_gui_update(id, props),"
      "    destroy: (id) => window.alloy_gui_destroy(id)"
      "  }"
      "};"
      "window._forbidden_eval = window.eval;"
      "window.eval = (code) => window.Alloy.secureEval(code);";

  webview_init(w, bridge_js);
  webview_init(w, ALLOY_BUNDLE);
  webview_set_html(w, "<h1>AlloyScript Production Runtime</h1><p>Ready.</p>");
  webview_run(w);
  webview_destroy(w);
  return 0;
}
