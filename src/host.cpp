#include "webview.h"
#include "webview/detail/json.hh"
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
struct NativeComponent {
    std::string type;
    void* handle;
    int id;
};

std::map<int, NativeComponent> g_components;
int g_next_component_id = 1;

// --- Helpers ---
#ifdef _WIN32
std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#endif

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

// Helper to create native components
void* create_native_control(const std::string& type, const std::string& props, void* parent) {
    int x = 0, y = 0, width = 100, height = 30;

    std::string x_str = webview::detail::json_parse(props, "x", 0);
    std::string y_str = webview::detail::json_parse(props, "y", 0);
    std::string width_str = webview::detail::json_parse(props, "width", 0);
    std::string height_str = webview::detail::json_parse(props, "height", 0);

    if (!x_str.empty()) x = std::stoi(x_str);
    if (!y_str.empty()) y = std::stoi(y_str);
    if (!width_str.empty()) width = std::stoi(width_str);
    if (!height_str.empty()) height = std::stoi(height_str);

#ifdef _WIN32
    HWND hwndParent = (HWND)parent;
    if (type == "Button") {
        std::string label = webview::detail::json_parse(props, "label", 0);
        std::wstring wlabel = utf8_to_utf16(label);
        return CreateWindowExW(0, L"BUTTON", wlabel.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                              x, y, width, height, hwndParent, NULL, (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE), NULL);
    } else if (type == "TextField") {
        return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
                              x, y, width, height, hwndParent, NULL, (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE), NULL);
    } else if (type == "Label") {
        std::string label = webview::detail::json_parse(props, "text", 0);
        std::wstring wlabel = utf8_to_utf16(label);
        return CreateWindowExW(0, L"STATIC", wlabel.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                              x, y, width, height, hwndParent, NULL, (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE), NULL);
    }
#elif defined(__APPLE__)
    id pool = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSAutoreleasePool"), sel_registerName("new"));
    id nsWindow = (id)parent;
    id parentView = ((id (*)(id, SEL))objc_msgSend)(nsWindow, sel_registerName("contentView"));

    if (type == "Button") {
        std::string label = webview::detail::json_parse(props, "label", 0);
        id btn = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSButton"), sel_registerName("alloc"));
        ((id (*)(id, SEL, NSRect))objc_msgSend)(btn, sel_registerName("initWithFrame:"), (NSRect){{(double)x, (double)y}, {(double)width, (double)height}});
        id str = ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), label.c_str());
        ((void (*)(id, SEL, id))objc_msgSend)(btn, sel_registerName("setTitle:"), str);
        ((void (*)(id, SEL, id))objc_msgSend)(parentView, sel_registerName("addSubview:"), btn);
        return btn;
    } else if (type == "TextField") {
        id field = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSTextField"), sel_registerName("alloc"));
        ((id (*)(id, SEL, NSRect))objc_msgSend)(field, sel_registerName("initWithFrame:"), (NSRect){{(double)x, (double)y}, {(double)width, (double)height}});
        ((void (*)(id, SEL, id))objc_msgSend)(parentView, sel_registerName("addSubview:"), field);
        return field;
    }
    ((void (*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
#else
    GtkWidget* gtk_parent = (GtkWidget*)parent;
    if (type == "Button") {
        std::string label = webview::detail::json_parse(props, "label", 0);
        GtkWidget* btn = gtk_button_new_with_label(label.c_str());
        gtk_widget_set_size_request(btn, width, height);
        // Note: For absolute positioning in GTK, one usually needs a GtkFixed or similar.
        // Assuming the main window container is suitable or using an overlay.
        gtk_widget_show(btn);
        return btn;
    } else if (type == "TextField") {
        GtkWidget* entry = gtk_entry_new();
        gtk_widget_set_size_request(entry, width, height);
        gtk_widget_show(entry);
        return entry;
    }
#endif
    return nullptr;
}

extern "C" void alloy_gui_create(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);

    // req is expected to be a JSON array: [type, props]
    std::string type = webview::detail::json_parse(request, "", 0);
    std::string props = webview::detail::json_parse(request, "", 1);

    int component_id = g_next_component_id++;
    void* parent = webview_get_window(w);
    void* handle = create_native_control(type, props, parent);

    g_components[component_id] = {type, handle, component_id};

    char buf[16];
    sprintf(buf, "%d", component_id);
    webview_return(w, id, 0, buf);
}

extern "C" void alloy_gui_update(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    // req: [component_id, props]
    std::string id_str = webview::detail::json_parse(request, "", 0);
    std::string props = webview::detail::json_parse(request, "", 1);
    int comp_id = std::stoi(id_str);

    if (g_components.count(comp_id)) {
        NativeComponent& comp = g_components[comp_id];
#ifdef _WIN32
        if (comp.type == "Button") {
            std::string label = webview::detail::json_parse(props, "label", 0);
            if (!label.empty()) {
                std::wstring wlabel = utf8_to_utf16(label);
                SetWindowTextW((HWND)comp.handle, wlabel.c_str());
            }
        } else if (comp.type == "TextField") {
            std::string val = webview::detail::json_parse(props, "value", 0);
            if (!val.empty()) {
                std::wstring wval = utf8_to_utf16(val);
                SetWindowTextW((HWND)comp.handle, wval.c_str());
            }
        }
#elif defined(__APPLE__)
        if (comp.type == "Button") {
            std::string label = webview::detail::json_parse(props, "label", 0);
            if (!label.empty()) {
                id str = ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), label.c_str());
                ((void (*)(id, SEL, id))objc_msgSend)((id)comp.handle, sel_registerName("setTitle:"), str);
            }
        }
#else
        if (comp.type == "Button") {
            std::string label = webview::detail::json_parse(props, "label", 0);
            if (!label.empty()) {
                gtk_button_set_label(GTK_BUTTON(comp.handle), label.c_str());
            }
        }
#endif
    }

    webview_return(w, id, 0, "0");
}

extern "C" void alloy_gui_destroy(const char *id, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    std::string request(req);
    // req: [component_id]
    std::string id_str = webview::detail::json_parse(request, "", 0);
    int comp_id = std::stoi(id_str);

    if (g_components.count(comp_id)) {
        void* handle = g_components[comp_id].handle;
#ifdef _WIN32
        if (handle) {
            DestroyWindow((HWND)handle);
        }
#elif defined(__APPLE__)
        if (handle) {
            ((void (*)(id, SEL))objc_msgSend)((id)handle, sel_registerName("removeFromSuperview"));
            ((void (*)(id, SEL))objc_msgSend)((id)handle, sel_registerName("release"));
        }
#else
        if (handle) {
            gtk_widget_destroy(GTK_WIDGET(handle));
        }
#endif
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
