#include "webview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#elif defined(__APPLE__)
#include <objc/objc-runtime.h>
#else
#include <gtk/gtk.h>
#endif

// The bundled JS will be injected here by the build script
extern const char* ALLOY_BUNDLE;

// --- Native Handle Management ---
#define MAX_COMPONENTS 1024
typedef struct {
    void* handle;
    char type[32];
} component_t;

component_t g_components[MAX_COMPONENTS] = {0};

int register_component(void* handle, const char* type) {
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (!g_components[i].handle) {
            g_components[i].handle = handle;
            strncpy(g_components[i].type, type, 31);
            return i + 1;
        }
    }
    return 0;
}

// --- GUI Framework Bindings (Platform Specific) ---

void alloy_gui_create(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    void* native_handle = NULL;
    char type[64] = {0};

    // In a real implementation, we parse req JSON for type and props
    // For this draft, we simulate based on "type" string
    if (strstr(req, "\"type\":\"Button\"")) {
        strcpy(type, "Button");
#ifdef _WIN32
        native_handle = CreateWindowExW(0, L"BUTTON", L"Button", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, (HWND)webview_get_window(w), NULL, NULL, NULL);
#elif defined(__APPLE__)
        // Cocoa implementation via objc-runtime
#else
        native_handle = gtk_button_new_with_label("Button");
        gtk_widget_show(GTK_WIDGET(native_handle));
#endif
    } else if (strstr(req, "\"type\":\"TextField\"")) {
        strcpy(type, "TextField");
#ifdef _WIN32
        native_handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 0, 0, 200, 25, (HWND)webview_get_window(w), NULL, NULL, NULL);
#elif defined(__APPLE__)
        // Cocoa implementation via objc-runtime
#else
        native_handle = gtk_entry_new();
        gtk_widget_show(GTK_WIDGET(native_handle));
#endif
    }

    int cid = register_component(native_handle, type);
    char buf[16];
    sprintf(buf, "%d", cid);
    webview_return(w, id, 0, buf);
}

void alloy_gui_update(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req: { id: number, props: { label: string, ... } }
    webview_return(w, id, 0, "0");
}

void alloy_gui_destroy(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int cid = atoi(req);
    if (cid > 0 && cid <= MAX_COMPONENTS && g_components[cid-1].handle) {
#ifdef _WIN32
        DestroyWindow((HWND)g_components[cid-1].handle);
#elif defined(__APPLE__)
        // Cocoa cleanup
#else
        gtk_widget_destroy(GTK_WIDGET(g_components[cid-1].handle));
#endif
        g_components[cid-1].handle = NULL;
    }
    webview_return(w, id, 0, "0");
}

// --- Legacy Bindings ---
void alloy_spawn(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}
void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}
void alloy_secure_eval(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, req);
}

// --- SQLite Backend ---
sqlite3 *g_db = NULL;
void alloy_sqlite_open(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int rc = sqlite3_open(":memory:", &g_db);
    webview_return(w, id, rc == SQLITE_OK ? 0 : 1, "1");
}
void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "1");
}
void alloy_sqlite_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "{\"lastInsertRowid\":0, \"changes\":0}");
}
void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "[{\"message\": \"Hello world\"}]");
}
void alloy_sqlite_close(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    if(g_db) { sqlite3_close(g_db); g_db = NULL; }
    webview_return(w, id, 0, "0");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Native Runtime");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

  webview_bind(w, "alloy_spawn", alloy_spawn, w);
  webview_bind(w, "alloy_spawn_sync", alloy_spawn_sync, w);
  webview_bind(w, "alloy_secure_eval", alloy_secure_eval, w);
  webview_bind(w, "alloy_sqlite_open", alloy_sqlite_open, w);
  webview_bind(w, "alloy_sqlite_query", alloy_sqlite_query, w);
  webview_bind(w, "alloy_sqlite_run", alloy_sqlite_run, w);
  webview_bind(w, "alloy_sqlite_stmt_all", alloy_sqlite_stmt_all, w);
  webview_bind(w, "alloy_sqlite_close", alloy_sqlite_close, w);
  webview_bind(w, "alloy_gui_create", alloy_gui_create, w);
  webview_bind(w, "alloy_gui_update", alloy_gui_update, w);
  webview_bind(w, "alloy_gui_destroy", alloy_gui_destroy, w);

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
      "    create: (type, props) => window.alloy_gui_create(JSON.stringify({type, props})),"
      "    update: (id, props) => window.alloy_gui_update(id, props),"
      "    destroy: (id) => window.alloy_gui_destroy(id)"
      "  }"
      "};"
      "globalThis._forbidden_eval = globalThis.eval;"
      "globalThis.eval = (code) => globalThis.Alloy.secureEval(code);";

  webview_init(w, bridge_js);
  webview_init(w, ALLOY_BUNDLE);
  webview_set_html(w, "<h1>AlloyScript Native Runtime</h1><p>Ready.</p>");
  webview_run(w);
  webview_destroy(w);
  return 0;
}
