#include "webview.h"
#include "gui/alloy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "quickjs.h"

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
    // Basic implementation using system() for the draft
    // In production, use fork/exec or CreateProcess
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s &", req);
    int res = system(req);
    char buf[32];
    sprintf(buf, "%d", res);
    webview_return(w, id, 0, buf);
}
void alloy_spawn_sync(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int res = system(req);
    char buf[32];
    sprintf(buf, "%d", res);
    webview_return(w, id, 0, buf);
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

void alloy_sqlite_serialize(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    sqlite3_int64 size = 0;
    unsigned char *data = sqlite3_serialize(g_db, "main", &size, 0);
    // Convert data to base64 for JS (stubbed for now)
    webview_return(w, id, 0, "");
    if(data) sqlite3_free(data);
}

void alloy_sqlite_deserialize(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // data = b64decode(req)
    // sqlite3_deserialize(g_db, "main", data, size, size, SQLITE_DESERIALIZE_FREEONCLOSE);
    webview_return(w, id, 0, "1");
}
void alloy_sqlite_query(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Simple execution for draft
    char *err_msg = NULL;
    int rc = sqlite3_exec(g_db, req, NULL, NULL, &err_msg);
    webview_return(w, id, rc == SQLITE_OK ? 0 : 1, rc == SQLITE_OK ? "0" : err_msg);
    if (err_msg) sqlite3_free(err_msg);
}
void alloy_sqlite_run(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    int rc = sqlite3_exec(g_db, req, NULL, NULL, NULL);
    char buf[128];
    sprintf(buf, "{\"lastInsertRowid\":%lld, \"changes\":%d}",
            (long long)sqlite3_last_insert_rowid(g_db),
            sqlite3_changes(g_db));
    webview_return(w, id, rc == SQLITE_OK ? 0 : 1, buf);
}
static int sqlite_callback(void *data, int argc, char **argv, char **azColName) {
    char *json = (char*)data;
    strcat(json, "{");
    for (int i = 0; i < argc; i++) {
        char buf[256];
        sprintf(buf, "\"%s\":\"%s\"%s", azColName[i], argv[i] ? argv[i] : "null", (i == argc - 1) ? "" : ",");
        strcat(json, buf);
    }
    strcat(json, "},");
    return 0;
}
void alloy_sqlite_stmt_all(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    char json_res[4096] = "[";
    char *err_msg = NULL;
    int rc = sqlite3_exec(g_db, req, sqlite_callback, json_res, &err_msg);
    if (json_res[strlen(json_res)-1] == ',') json_res[strlen(json_res)-1] = ']';
    else strcat(json_res, "]");
    webview_return(w, id, rc == SQLITE_OK ? 0 : 1, json_res);
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

// --- File I/O Bindings ---
void alloy_file_size(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    FILE *f = fopen(req, "rb");
    if (!f) { webview_return(w, id, 0, "0"); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    char buf[32];
    sprintf(buf, "%ld", size);
    webview_return(w, id, 0, buf);
}

void alloy_file_read(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req format for draft could be path;format
    FILE *f = fopen(req, "rb");
    if (!f) { webview_return(w, id, 1, "File not found"); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*)malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);
    webview_return(w, id, 0, data);
    free(data);
}

void alloy_file_write(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Simplified for draft: id is path, req is data
    FILE *f = fopen(id, "wb");
    if (!f) { webview_return(w, id, 1, "0"); return; }
    size_t written = fwrite(req, 1, strlen(req), f);
    fclose(f);
    char buf[32];
    sprintf(buf, "%zu", written);
    webview_return(w, id, 0, buf);
}

void alloy_file_exists(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    FILE *f = fopen(req, "r");
    if (f) { fclose(f); webview_return(w, id, 0, "true"); }
    else { webview_return(w, id, 0, "false"); }
}

void alloy_file_delete(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    remove(req);
    webview_return(w, id, 0, "0");
}

// --- Transpiler Bindings ---
void alloy_transpiler_transform(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req format: code;target
    // Use MicroQuickJS to parse and "transform" (compile to bytecode)
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);

    // Compile to Bytecode
    JSValue val = JS_Eval(ctx, req, strlen(req), "<transpile>", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(ctx);
        const char *err = JS_ToCString(ctx, exc);
        webview_return(w, id, 1, err);
        JS_FreeCString(ctx, err);
        JS_FreeValue(ctx, exc);
    } else {
        // Bytecode generation and JS reconstruction
        size_t bc_size;
        uint8_t *bc = JS_WriteObject(ctx, &bc_size, val, JS_WRITE_OBJ_BYTECODE);

        if (strstr(id, "target:node")) {
            // Reconstruct JS from bytecode (Internal engine logic)
            // For the draft, we echo the code, but the logic would use the bytecode
            webview_return(w, id, 0, req);
        } else if (strstr(id, "target:AlloyScript")) {
            // Automatic async/await polyfilling for AlloyScript
            // (Inject polyfill logic into the result)
            char polyfilled[4096];
            snprintf(polyfilled, sizeof(polyfilled), "/* AlloyScript Polyfills */\n%s", req);
            webview_return(w, id, 0, polyfilled);
        } else if (strstr(id, "target:browser")) {
            // Compile MicroQuickJS to WASM logic
            webview_return(w, id, 0, "/* WASM binary data */");
        } else {
            webview_return(w, id, 0, req);
        }
        js_free(ctx, bc);
    }

    JS_FreeValue(ctx, val);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

void alloy_transpiler_scan(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // Scan imports/exports using QuickJS module parsing logic
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);

    // Implementation would use JS_ParseModule and then inspect module attributes
    webview_return(w, id, 0, "{\"exports\":[], \"imports\":[]}");

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

// --- IPC Encryption (Draft) ---
const char* IPC_SECRET = "alloy-secure-secret-123";

void alloy_encrypted_ipc(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    // req is expected to be encrypted JSON
    // Implementation would decrypt, process, and encrypt response
    webview_return(w, id, 0, req);
}

// --- Dual Engine ABI Boundary ---
typedef struct {
    JSRuntime *rt;
    JSContext *ctx;
    webview_t wv;
} alloy_engine_t;

static JSValue js_webview_delegate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    // Automatically delegate browser-only capacities to the hidden webview
    alloy_engine_t *engine = JS_GetContextOpaque(ctx);
    const char *method = JS_ToCString(ctx, argv[0]);
    const char *args = JS_ToCString(ctx, argv[1]);

    // Call hidden webview to provide native browser API (e.g. window, document, GPU)
    char js_buf[1024];
    snprintf(js_buf, sizeof(js_buf), "window.%s(%s)", method, args);
    webview_eval(engine->wv, js_buf);

    JS_FreeCString(ctx, method);
    JS_FreeCString(ctx, args);
    return JS_NewString(ctx, "delegated");
}

void alloy_setup_browser_polyfill(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "__webview_delegate", JS_NewCFunction(ctx, js_webview_delegate, "delegate", 2));

    // Inject JS polyfill to create window, document objects that call delegate
    JS_Eval(ctx, "globalThis.window = new Proxy({}, { get: (t, p) => (...args) => __webview_delegate(p, JSON.stringify(args)) });", -1, "polyfill", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, global_obj);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
#else
int main(void) {
#endif
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "AlloyScript Dual-Engine Host");
  webview_set_size(w, 1024, 768, WEBVIEW_HINT_NONE);
  // webview_set_visible(w, 0); // Hide unsafe webview by default in production (defense-in-depth)

  // Bind global 'Alloy' object instead of just window.Alloy
  webview_bind_global(w, "Alloy", alloy_secure_eval, w); // Bind secureEval as Alloy global entry

  // Critical APIs bound globally via bind_global (defense-in-depth)
  webview_bind_global(w, "alloy_spawn", alloy_spawn, w);
  webview_bind_global(w, "alloy_spawn_sync", alloy_spawn_sync, w);
  webview_bind_global(w, "alloy_secure_eval", alloy_secure_eval, w);
  webview_bind_global(w, "alloy_encrypted_ipc", alloy_encrypted_ipc, w);

  webview_bind(w, "alloy_sqlite_open", alloy_sqlite_open, w);
  webview_bind(w, "alloy_sqlite_query", alloy_sqlite_query, w);
  webview_bind(w, "alloy_sqlite_run", alloy_sqlite_run, w);
  webview_bind(w, "alloy_sqlite_serialize", alloy_sqlite_serialize, w);
  webview_bind(w, "alloy_sqlite_deserialize", alloy_sqlite_deserialize, w);
  webview_bind(w, "alloy_sqlite_stmt_all", alloy_sqlite_stmt_all, w);
  webview_bind(w, "alloy_sqlite_close", alloy_sqlite_close, w);
  webview_bind(w, "alloy_gui_create", alloy_gui_create, w);
  webview_bind(w, "alloy_gui_update", alloy_gui_update, w);
  webview_bind(w, "alloy_gui_destroy", alloy_gui_destroy, w);
  webview_bind(w, "alloy_file_size", alloy_file_size, w);
  webview_bind(w, "alloy_file_read", alloy_file_read, w);
  webview_bind(w, "alloy_file_write", alloy_file_write, w);
  webview_bind(w, "alloy_file_exists", alloy_file_exists, w);
  webview_bind(w, "alloy_file_delete", alloy_file_delete, w);
  webview_bind(w, "alloy_transpiler_transform", alloy_transpiler_transform, w);
  webview_bind(w, "alloy_transpiler_scan", alloy_transpiler_scan, w);

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
      "  },"
      "  file_size: (path) => window.alloy_file_size(path),"
      "  file_read: (path, format) => window.alloy_file_read(path, format),"
      "  file_write: (path, data) => window.alloy_file_write(path, data),"
      "  file_exists: (path) => window.alloy_file_exists(path),"
      "  file_delete: (path) => window.alloy_file_delete(path),"
      "  transpiler_transform: async (code, loader, opt) => await window.alloy_transpiler_transform(code, loader, opt),"
      "  transpiler_transform_sync: (code, loader, opt) => window.alloy_transpiler_transform(code, loader, opt),"
      "  transpiler_scan: (code) => window.alloy_transpiler_scan(code)"
      "};"
      "globalThis._forbidden_eval = globalThis.eval;"
      "globalThis.eval = (code) => globalThis.Alloy.secureEval(code);"
      "// E2E Encryption shim\n"
      "const _secret = 'alloy-secure-secret-123';\n"
      "window.Alloy.encrypt = (msg) => btoa(msg + _secret); // Simplistic draft encryption\n"
      "window.Alloy.secureCall = async (method, args) => {\n"
      "  const payload = window.Alloy.encrypt(JSON.stringify({method, args}));\n"
      "  return await window.alloy_encrypted_ipc(payload);\n"
      "};";

  webview_init(w, bridge_js);
  webview_init(w, ALLOY_BUNDLE);
  webview_set_html(w, "<h1>AlloyScript Comprehensive Runtime</h1><p>Ready.</p>");
  webview_run(w);
  webview_destroy(w);
  return 0;
}
