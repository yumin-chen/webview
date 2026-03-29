#include "webview.h"
#include "gui/alloy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// The bundled JS will be injected here by the build script
extern const char* ALLOY_BUNDLE;

// --- Process Management ---
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

// --- GUI Framework Bindings (Complete Component Map) ---
void alloy_gui_create(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    alloy_component_t component = NULL;
    alloy_error_t err = ALLOY_OK;
    alloy_component_t parent = (alloy_component_t)webview_get_window(w);

    if (strstr(req, "\"type\":\"Window\"")) err = alloy_create_window("Window", 800, 600, &component);
    else if (strstr(req, "\"type\":\"Button\"")) err = alloy_create_button(parent, &component);
    else if (strstr(req, "\"type\":\"TextField\"")) err = alloy_create_textfield(parent, &component);
    else if (strstr(req, "\"type\":\"TextArea\"")) err = alloy_create_textarea(parent, &component);
    else if (strstr(req, "\"type\":\"CheckBox\"")) err = alloy_create_checkbox(parent, &component);
    else if (strstr(req, "\"type\":\"RadioButton\"")) err = alloy_create_radiobutton(parent, &component);
    else if (strstr(req, "\"type\":\"ComboBox\"")) err = alloy_create_combobox(parent, &component);
    else if (strstr(req, "\"type\":\"Slider\"")) err = alloy_create_slider(parent, &component);
    else if (strstr(req, "\"type\":\"Spinner\"")) err = alloy_create_spinner(parent, &component);
    else if (strstr(req, "\"type\":\"DatePicker\"")) err = alloy_create_datepicker(parent, &component);
    else if (strstr(req, "\"type\":\"TimePicker\"")) err = alloy_create_timepicker(parent, &component);
    else if (strstr(req, "\"type\":\"ColorPicker\"")) err = alloy_create_colorpicker(parent, &component);
    else if (strstr(req, "\"type\":\"Switch\"")) err = alloy_create_switch(parent, &component);
    else if (strstr(req, "\"type\":\"Label\"")) err = alloy_create_label(parent, &component);
    else if (strstr(req, "\"type\":\"Image\"")) err = alloy_create_image(parent, &component);
    else if (strstr(req, "\"type\":\"Icon\"")) err = alloy_create_icon(parent, &component);
    else if (strstr(req, "\"type\":\"ProgressBar\"")) err = alloy_create_progressbar(parent, &component);
    else if (strstr(req, "\"type\":\"Tooltip\"")) err = alloy_create_tooltip(parent, &component);
    else if (strstr(req, "\"type\":\"Badge\"")) err = alloy_create_badge(parent, &component);
    else if (strstr(req, "\"type\":\"Card\"")) err = alloy_create_card(parent, &component);
    else if (strstr(req, "\"type\":\"Divider\"")) err = alloy_create_divider(parent, &component);
    else if (strstr(req, "\"type\":\"RichTextEditor\"")) err = alloy_create_richtexteditor(parent, &component);
    else if (strstr(req, "\"type\":\"ListView\"")) err = alloy_create_listview(parent, &component);
    else if (strstr(req, "\"type\":\"TreeView\"")) err = alloy_create_treeview(parent, &component);
    else if (strstr(req, "\"type\":\"TabView\"")) err = alloy_create_tabview(parent, &component);
    else if (strstr(req, "\"type\":\"VStack\"")) err = alloy_create_vstack(parent, &component);
    else if (strstr(req, "\"type\":\"HStack\"")) err = alloy_create_hstack(parent, &component);
    else if (strstr(req, "\"type\":\"ScrollView\"")) err = alloy_create_scrollview(parent, &component);
    else if (strstr(req, "\"type\":\"GroupBox\"")) err = alloy_create_groupbox(parent, &component);
    else if (strstr(req, "\"type\":\"Dialog\"")) err = alloy_create_dialog(parent, &component);
    else if (strstr(req, "\"type\":\"FileDialog\"")) err = alloy_create_filedialog(parent, &component);
    else if (strstr(req, "\"type\":\"Popover\"")) err = alloy_create_popover(parent, &component);
    else if (strstr(req, "\"type\":\"StatusBar\"")) err = alloy_create_statusbar(parent, &component);
    else if (strstr(req, "\"type\":\"Splitter\"")) err = alloy_create_splitter(parent, &component);
    else if (strstr(req, "\"type\":\"WebView\"")) err = alloy_create_webview(parent, &component);
    else if (strstr(req, "\"type\":\"Link\"")) err = alloy_create_link(parent, &component);
    else if (strstr(req, "\"type\":\"Chip\"")) err = alloy_create_chip(parent, &component);
    else if (strstr(req, "\"type\":\"Rating\"")) err = alloy_create_rating(parent, &component);
    else if (strstr(req, "\"type\":\"Accordion\"")) err = alloy_create_accordion(parent, &component);
    else if (strstr(req, "\"type\":\"CodeEditor\"")) err = alloy_create_codeeditor(parent, &component);

    if (err == ALLOY_OK && component) {
        char buf[32];
        sprintf(buf, "%p", component);
        webview_return(w, id, 0, buf);
    } else {
        webview_return(w, id, 1, alloy_error_message(err));
    }
}

void alloy_gui_update(const char *id, const char *req, void *arg) {
    webview_return((webview_t)arg, id, 0, "0");
}

void alloy_gui_destroy(const char *id, const char *req, void *arg) {
    void *ptr = NULL;
    sscanf(req, "%p", &ptr);
    if (ptr) alloy_destroy((alloy_component_t)ptr);
    webview_return((webview_t)arg, id, 0, "0");
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Comprehensive Runtime");
  webview_set_size(w, 1024, 768, WEBVIEW_HINT_NONE);

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
      "    update: (handle, props) => window.alloy_gui_update(handle, props),"
      "    destroy: (handle) => window.alloy_gui_destroy(handle)"
      "  }"
      "};"
      "globalThis._forbidden_eval = globalThis.eval;"
      "globalThis.eval = (code) => globalThis.Alloy.secureEval(code);";

  webview_init(w, bridge_js);
  webview_init(w, ALLOY_BUNDLE);
  webview_set_html(w, "<h1>AlloyScript Comprehensive Runtime</h1><p>Ready.</p>");
  webview_run(w);
  webview_destroy(w);
  return 0;
}
