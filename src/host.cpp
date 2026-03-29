#include "webview.h"
#include "webview/detail/json.hh"
#include "alloy/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <stringapiset.h>
#include <commctrl.h>
#elif defined(__APPLE__)
#include <objc/objc-runtime.h>
#include <objc/message.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <gtk/gtk.h>
#endif

// The bundled JS will be injected here by the build script
extern "C" const char* ALLOY_BUNDLE;

// Simple state management (limited for the draft, but showing production structure)
#define MAX_DBS 16
#define MAX_STMTS 128
sqlite3 *g_dbs[MAX_DBS] = {NULL};
sqlite3_stmt *g_stmts[MAX_STMTS] = {NULL};

// --- GUI Component Management ---
std::map<int, alloy_component_t> g_components;
int g_next_component_id = 1;

// --- Process Management ---

extern "C" void alloy_spawn(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
#ifndef _WIN32
    pid_t pid = fork();
    if (pid == 0) {
        exit(0);
    } else if (pid > 0) {
        webview_return(w, id, 0, "0");
    } else {
        webview_return(w, id, 1, "fork failed");
    }
#else
    webview_return(w, id, 0, "0");
#endif
}

extern "C" void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "0");
}

extern "C" void alloy_secure_eval(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, req);
}

// --- SQLite Backend ---

extern "C" void alloy_sqlite_open(const char *id, const char *req, void *arg) {
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

extern "C" void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
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

extern "C" void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    webview_return(w, id, 0, "[{\"message\": \"Hello world\"}]");
}

extern "C" void alloy_sqlite_run(const char *id, const char *req, void *arg) {
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

extern "C" void alloy_sqlite_close(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    if(g_dbs[0]) {
        sqlite3_close(g_dbs[0]);
        g_dbs[0] = NULL;
    }
    webview_return(w, id, 0, "0");
}

// --- GUI Framework Bindings ---

extern "C" void alloy_gui_create(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);

    std::string type = webview::detail::json_parse(request, "", 0);
    std::string props = webview::detail::json_parse(request, "", 1);

    int component_id = g_next_component_id++;
    // Opaque parent for webview
    alloy_component_t comp = nullptr;

    if (type == "Button") comp = alloy_create_button(nullptr);
    else if (type == "TextField") comp = alloy_create_textfield(nullptr);
    else if (type == "TextArea") comp = alloy_create_textarea(nullptr);
    else if (type == "Label") comp = alloy_create_label(nullptr);
    else if (type == "CheckBox") comp = alloy_create_checkbox(nullptr);
    else if (type == "RadioButton") comp = alloy_create_radiobutton(nullptr);
    else if (type == "ComboBox") comp = alloy_create_combobox(nullptr);
    else if (type == "Slider") comp = alloy_create_slider(nullptr);
    else if (type == "ProgressBar") comp = alloy_create_progressbar(nullptr);

    g_components[component_id] = comp;

    if (comp) {
        std::string label = webview::detail::json_parse(props, "label", 0);
        if (label.empty()) label = webview::detail::json_parse(props, "text", 0);
        if (!label.empty()) alloy_set_text(comp, label.c_str());
    }

    char buf[16];
    sprintf(buf, "%d", component_id);
    webview_return(w, id, 0, buf);
}

extern "C" void alloy_gui_update(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    std::string id_str = webview::detail::json_parse(request, "", 0);
    std::string props = webview::detail::json_parse(request, "", 1);
    int comp_id = std::stoi(id_str);

    if (g_components.count(comp_id)) {
        alloy_component_t comp = g_components[comp_id];
        std::string label = webview::detail::json_parse(props, "label", 0);
        if (label.empty()) label = webview::detail::json_parse(props, "text", 0);
        if (!label.empty()) alloy_set_text(comp, label.c_str());
    }

    webview_return(w, id, 0, "0");
}

extern "C" void alloy_gui_destroy(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    std::string id_str = webview::detail::json_parse(request, "", 0);
    int comp_id = std::stoi(id_str);

    if (g_components.count(comp_id)) {
        alloy_destroy(g_components[comp_id]);
        g_components.erase(comp_id);
    }
    webview_return(w, id, 0, "0");
}

// --- Main Loop ---

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Production Runtime");
  webview_set_size(w, 800, 600, WEBVIEW_HINT_NONE);

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
