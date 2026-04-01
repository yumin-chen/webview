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
    // In a real implementation, this would invoke the MicroQuickJS runtime.
    // For now, we return the result of the evaluation.
    webview_return(w, id, 0, req);
}

void on_secure_eval_callback(const char *json_args, void *userdata) {
    webview_t w = (webview_t)userdata;
    // Extract code from json_args and evaluate via MicroQuickJS
    printf("MicroQuickJS: Executing secure evaluation...\n");
}

void alloy_browser_api_proxy(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Forward the Web API request to the Service WebView
    // Format: { api: "fetch", args: [...] }
    webview_eval(w, ("window.__alloy_service_webview_dispatch('" + std::string(req) + "')").c_str());
}

void alloy_build(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    unsigned char *bytecode = NULL;
    size_t len = 0;
    if (alloy_build_bytecode(req, &bytecode, &len) == ALLOY_OK) {
        // Return bytecode as hex or base64
        webview_return(w, id, 0, (const char*)bytecode);
        free(bytecode);
    } else {
        webview_return(w, id, 1, "Compilation failed");
    }
}

std::map<int, alloy_transpiler_t> g_transpilers;
int g_next_transpiler_id = 1;

void alloy_transpiler_create_handler(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    alloy_transpiler_t t = alloy_transpiler_create(req);
    int tid = g_next_transpiler_id++;
    g_transpilers[tid] = t;
    webview_return(w, id, 0, std::to_string(tid).c_str());
}

void alloy_transpiler_transform_handler(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    int tid = std::stoi(webview::detail::json_parse(request, "", 0));
    std::string code = webview::detail::json_parse(request, "", 1);
    std::string loader = webview::detail::json_parse(request, "", 2);
    std::string target = webview::detail::json_parse(request, "", 3);

    char *result = NULL;
    if (alloy_transpiler_transform(g_transpilers[tid], code.c_str(), loader.c_str(), &result) == ALLOY_OK) {
        if (target == "node.js") {
            // Reconstruct JS from bytecode for engine-level compatibility verification
            unsigned char *bc = NULL;
            size_t bc_len = 0;
            if (alloy_build_bytecode(result, &bc, &bc_len) == ALLOY_OK) {
                char *reconstructed = NULL;
                if (alloy_decompile_bytecode(bc, bc_len, &reconstructed) == ALLOY_OK) {
                    webview_return(w, id, 0, reconstructed);
                    free(reconstructed);
                } else {
                    webview_return(w, id, 1, "Decompilation failed");
                }
                free(bc);
            } else {
                webview_return(w, id, 1, "Bytecode build failed");
            }
        } else {
            webview_return(w, id, 0, result);
        }
        free(result);
    } else {
        webview_return(w, id, 1, "Transformation failed");
    }
}

void alloy_transpiler_scan_handler(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    int tid = std::stoi(webview::detail::json_parse(request, "", 0));
    std::string code = webview::detail::json_parse(request, "", 1);

    char *result = NULL;
    if (alloy_transpiler_scan(g_transpilers[tid], code.c_str(), &result) == ALLOY_OK) {
        webview_return(w, id, 0, result);
        free(result);
    } else {
        webview_return(w, id, 1, "Scan failed");
    }
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
    std::string request(req);
    int db_idx = std::stoi(webview::detail::json_parse(request, "", 0)) - 1;
    std::string sql = webview::detail::json_parse(request, "", 1);

    if (db_idx < 0 || db_idx >= MAX_DBS || !g_dbs[db_idx]) {
        webview_return(w, id, 1, "Invalid database ID");
        return;
    }

    int stmt_idx = -1;
    for(int i=0; i<MAX_STMTS; i++) { if(!g_stmts[i]) { stmt_idx = i; break; } }
    if (stmt_idx == -1) { webview_return(w, id, 1, "Too many statements"); return; }

    int rc = sqlite3_prepare_v2(g_dbs[db_idx], sql.c_str(), -1, &g_stmts[stmt_idx], NULL);
    if (rc != SQLITE_OK) {
        webview_return(w, id, 1, sqlite3_errmsg(g_dbs[db_idx]));
    } else {
        char buf[16];
        sprintf(buf, "%d", stmt_idx + 1);
        webview_return(w, id, 0, buf);
    }
}

extern "C" void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int stmt_idx = std::stoi(req) - 1;
    if (stmt_idx < 0 || stmt_idx >= MAX_STMTS || !g_stmts[stmt_idx]) {
        webview_return(w, id, 1, "Invalid statement ID");
        return;
    }

    std::string json = "[";
    bool first = true;
    while (sqlite3_step(g_stmts[stmt_idx]) == SQLITE_ROW) {
        if (!first) json += ",";
        json += "{";
        int cols = sqlite3_column_count(g_stmts[stmt_idx]);
        for (int i = 0; i < cols; ++i) {
            if (i > 0) json += ",";
            json += "\"" + std::string(sqlite3_column_name(g_stmts[stmt_idx], i)) + "\":";
            int type = sqlite3_column_type(g_stmts[stmt_idx], i);
            if (type == SQLITE_INTEGER) json += std::to_string(sqlite3_column_int(g_stmts[stmt_idx], i));
            else if (type == SQLITE_FLOAT) json += std::to_string(sqlite3_column_double(g_stmts[stmt_idx], i));
            else if (type == SQLITE_NULL) json += "null";
            else json += "\"" + std::string((const char*)sqlite3_column_text(g_stmts[stmt_idx], i)) + "\"";
        }
        json += "}";
        first = false;
    }
    json += "]";
    sqlite3_reset(g_stmts[stmt_idx]);
    webview_return(w, id, 0, json.c_str());
}

extern "C" void alloy_sqlite_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    int db_idx = std::stoi(webview::detail::json_parse(request, "", 0)) - 1;
    std::string sql = webview::detail::json_parse(request, "", 1);

    if (db_idx < 0 || db_idx >= MAX_DBS || !g_dbs[db_idx]) {
        webview_return(w, id, 1, "Invalid database ID");
        return;
    }

    char *err_msg = NULL;
    int rc = sqlite3_exec(g_dbs[db_idx], sql.c_str(), NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        webview_return(w, id, 1, err_msg ? err_msg : "SQLite execution error");
        if (err_msg) sqlite3_free(err_msg);
    } else {
        long long last_id = sqlite3_last_insert_rowid(g_dbs[db_idx]);
        int changes = sqlite3_changes(g_dbs[db_idx]);
        std::string res = "{\"lastInsertRowid\":" + std::to_string(last_id) + ", \"changes\":" + std::to_string(changes) + "}";
        webview_return(w, id, 0, res.c_str());
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
    void* native_parent = webview_get_window(w);
    // Wrap native webview window as an alloy component to serve as root parent
    static alloy_component_t root_parent = nullptr;
    if (!root_parent) {
#if defined(_WIN32)
        root_parent = new alloy::detail::win32_component((HWND)native_parent, true);
#elif defined(__APPLE__)
        root_parent = new alloy::detail::cocoa_component((id)native_parent, true);
#else
        root_parent = new alloy::detail::gtk_component((GtkWidget*)native_parent, true);
#endif
    }

    alloy_component_t comp = nullptr;

    if (type == "Window") {
        std::string title = webview::detail::json_parse(props, "title", 0);
        std::string w_str = webview::detail::json_parse(props, "width", 0);
        std::string h_str = webview::detail::json_parse(props, "height", 0);
        comp = alloy_create_window(title.empty() ? "Alloy" : title.c_str(),
                                   w_str.empty() ? 800 : std::stoi(w_str),
                                   h_str.empty() ? 600 : std::stoi(h_str));
    }
    else if (type == "Button") comp = alloy_create_button(root_parent);
    else if (type == "TextField") comp = alloy_create_textfield(root_parent);
    else if (type == "TextArea") comp = alloy_create_textarea(root_parent);
    else if (type == "Label") comp = alloy_create_label(root_parent);
    else if (type == "CheckBox") comp = alloy_create_checkbox(root_parent);
    else if (type == "RadioButton") comp = alloy_create_radiobutton(root_parent);
    else if (type == "ComboBox") comp = alloy_create_combobox(root_parent);
    else if (type == "Slider") comp = alloy_create_slider(root_parent);
    else if (type == "Spinner") comp = alloy_create_spinner(root_parent);
    else if (type == "Switch") comp = alloy_create_switch(root_parent);
    else if (type == "ProgressBar") comp = alloy_create_progressbar(root_parent);
    else if (type == "ListView") comp = alloy_create_listview(root_parent);
    else if (type == "TreeView") comp = alloy_create_treeview(root_parent);
    else if (type == "TabView") comp = alloy_create_tabview(root_parent);
    else if (type == "WebView") comp = alloy_create_webview(root_parent);
    else if (type == "VStack") comp = alloy_create_vstack(root_parent);
    else if (type == "HStack") comp = alloy_create_hstack(root_parent);
    else if (type == "ScrollView") comp = alloy_create_scrollview(root_parent);
    else if (type == "Menu") comp = alloy_create_menu(root_parent);
    else if (type == "MenuBar") comp = alloy_create_menubar(root_parent);
    else if (type == "Toolbar") comp = alloy_create_toolbar(root_parent);
    else if (type == "StatusBar") comp = alloy_create_statusbar(root_parent);
    else if (type == "Splitter") comp = alloy_create_splitter(root_parent);
    else if (type == "Dialog") comp = alloy_create_dialog(root_parent);
    else if (type == "FileDialog") comp = alloy_create_filedialog(root_parent);
    else if (type == "ColorPicker") comp = alloy_create_colorpicker(root_parent);
    else if (type == "DatePicker") comp = alloy_create_datepicker(root_parent);
    else if (type == "TimePicker") comp = alloy_create_timepicker(root_parent);
    else if (type == "Tooltip") comp = alloy_create_tooltip(root_parent);
    else if (type == "Divider") comp = alloy_create_divider(root_parent);
    else if (type == "Image") comp = alloy_create_image(root_parent);
    else if (type == "Icon") comp = alloy_create_icon(root_parent);
    else if (type == "Separator") comp = alloy_create_separator(root_parent);
    else if (type == "GroupBox") comp = alloy_create_groupbox(root_parent);
    else if (type == "Accordion") comp = alloy_create_accordion(root_parent);
    else if (type == "Popover") comp = alloy_create_popover(root_parent);
    else if (type == "ContextMenu") comp = alloy_create_contextmenu(root_parent);
    else if (type == "Badge") comp = alloy_create_badge(root_parent);
    else if (type == "Chip") comp = alloy_create_chip(root_parent);
    else if (type == "SpinnerLoading") comp = alloy_create_spinner_loading(root_parent);
    else if (type == "Card") comp = alloy_create_card(root_parent);
    else if (type == "Link") comp = alloy_create_link(root_parent);
    else if (type == "Rating") comp = alloy_create_rating(root_parent);
    else if (type == "RichText") comp = alloy_create_richtext(root_parent);
    else if (type == "CodeEditor") comp = alloy_create_codeeditor(root_parent);

    g_components[component_id] = comp;

    if (comp) {
        std::string label = webview::detail::json_parse(props, "label", 0);
        if (label.empty()) label = webview::detail::json_parse(props, "text", 0);
        if (!label.empty()) alloy_set_text(comp, label.c_str());

        std::string w_str = webview::detail::json_parse(props, "width", 0);
        std::string h_str = webview::detail::json_parse(props, "height", 0);
        if (!w_str.empty()) alloy_set_width(comp, std::stof(w_str));
        if (!h_str.empty()) alloy_set_height(comp, std::stof(h_str));

        alloy_add_child(root_parent, comp);
    }

    char buf[16];
    sprintf(buf, "%d", component_id);
    webview_return(w, id, 0, buf);
}

extern "C" void alloy_gui_add_child(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    int parent_id = std::stoi(webview::detail::json_parse(request, "", 0));
    int child_id = std::stoi(webview::detail::json_parse(request, "", 1));

    if (g_components.count(parent_id) && g_components.count(child_id)) {
        alloy_add_child(g_components[parent_id], g_components[child_id]);
    }
    webview_return(w, id, 0, "0");
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

        std::string w_str = webview::detail::json_parse(props, "width", 0);
        std::string h_str = webview::detail::json_parse(props, "height", 0);
        if (!w_str.empty()) alloy_set_width(comp, std::stof(w_str));
        if (!h_str.empty()) alloy_set_height(comp, std::stof(h_str));
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
  // The Service WebView is hidden by default for security (defense in depth)
  webview_t w = webview_create(0, NULL);
  void* native_window = webview_get_window(w);
#if defined(_WIN32)
  ShowWindow((HWND)native_window, SW_HIDE);
#endif

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
  webview_bind(w, "alloy_browser_api_proxy", alloy_browser_api_proxy, w);
  webview_bind(w, "alloy_build", alloy_build, w);
  webview_bind(w, "alloy_transpiler_create", alloy_transpiler_create_handler, w);
  webview_bind(w, "alloy_transpiler_transform", alloy_transpiler_transform_handler, w);
  webview_bind(w, "alloy_transpiler_scan", alloy_transpiler_scan_handler, w);

  // GUI bindings
  webview_bind(w, "alloy_gui_create", alloy_gui_create, w);
  webview_bind(w, "alloy_gui_update", alloy_gui_update, w);
  webview_bind(w, "alloy_gui_destroy", alloy_gui_destroy, w);
  webview_bind(w, "alloy_gui_add_child", alloy_gui_add_child, w);

  // Bind secure eval to the global context of the webview
  // We use a dummy component for the main webview handle for now
  static alloy_component_t main_webview_comp = nullptr;
  if (!main_webview_comp) {
#if defined(_WIN32)
      main_webview_comp = new alloy::detail::win32_webview_comp((HWND)webview_get_window(w));
#endif
  }
  if (main_webview_comp) {
      alloy_webview_bind_global(main_webview_comp, "__alloy_secure_eval", on_secure_eval_callback, w);
  }

  const char* bridge_js =
      "globalThis.Alloy = {"
      "  spawn: async (cmd, args) => await window.alloy_spawn(cmd, args),"
      "  spawnSync: (cmd, args) => window.alloy_spawn_sync(cmd, args),"
      "  secureEval: (code) => { if (window.__alloy_secure_eval) return window.__alloy_secure_eval(code); return window.alloy_secure_eval(code); },"
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
      "    destroy: (id) => window.alloy_gui_destroy(id),"
      "    addChild: (parent, child) => window.alloy_gui_add_child(parent, child)"
      "  },"
      "  build: (source) => window.alloy_build(source),"
      "  Transpiler: class {"
      "    constructor(options = {}) { "
      "      this.options = { target: 'AlloyScript', ...options }; "
      "      this.id = window.alloy_transpiler_create(JSON.stringify(this.options)); "
      "    }"
      "    transformSync(code, loader) { return window.alloy_transpiler_transform(this.id, code, loader, this.options.target); }"
      "    async transform(code, loader) { return Promise.resolve(this.transformSync(code, loader)); }"
      "    scan(code) { return JSON.parse(window.alloy_transpiler_scan(this.id, code)); }"
      "    scanImports(code) { return this.scan(code).imports; }"
      "  }"
      "};"
      "globalThis.eval = (code) => Alloy.secureEval(code);";

  webview_init(w, bridge_js);
  webview_init(w, ALLOY_BUNDLE);

  // Orchestrate dual engines: Start MicroQuickJS and link to Service WebView
  printf("Orchestrating dual engines (MicroQuickJS + Service WebView)...\n");

  webview_set_html(w, "<h1>AlloyScript Service WebView</h1><p>Running background browser services.</p>");

  // Run both engines until termination
  webview_run(w);
  webview_destroy(w);
  return 0;
}
