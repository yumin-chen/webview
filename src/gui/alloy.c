#include "alloy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define ALLOY_PLATFORM_WINDOWS
#include <windows.h>
#include <commctrl.h>
#elif defined(__APPLE__)
#define ALLOY_PLATFORM_DARWIN
#include <objc/objc-runtime.h>
#else
#define ALLOY_PLATFORM_LINUX
#include <gtk/gtk.h>
#endif

// --- Error Messaging ---

const char* alloy_error_message(alloy_error_t error) {
    switch (error) {
        case ALLOY_OK: return "Success";
        case ALLOY_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case ALLOY_ERROR_INVALID_STATE: return "Invalid state";
        case ALLOY_ERROR_PLATFORM: return "Platform error";
        case ALLOY_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ALLOY_ERROR_NOT_SUPPORTED: return "Not supported";
        default: return "Unknown error";
    }
}

// --- Internal Component Structure ---

typedef struct {
    alloy_event_cb_t callbacks[8];
    void *userdata[8];
    void *native_handle;
    char *text;
    int enabled;
    int visible;
} alloy_comp_internal_t;

// --- Window Lifecycle ---

alloy_error_t alloy_create_window(const char *title, int width, int height, alloy_component_t *out_window) {
    alloy_comp_internal_t *win = (alloy_comp_internal_t*)calloc(1, sizeof(alloy_comp_internal_t));

#ifdef ALLOY_PLATFORM_WINDOWS
    win->native_handle = CreateWindowExW(0, L"STATIC", L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
#elif defined(ALLOY_PLATFORM_LINUX)
    if (!gtk_init_check(NULL, NULL)) return ALLOY_ERROR_PLATFORM;
    win->native_handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win->native_handle), title);
    gtk_window_set_default_size(GTK_WINDOW(win->native_handle), width, height);
#endif

    if (!win->native_handle) { free(win); return ALLOY_ERROR_PLATFORM; }
    *out_window = (alloy_component_t)win;
    return ALLOY_OK;
}

alloy_error_t alloy_destroy(alloy_component_t component) {
    if (!component) return ALLOY_ERROR_INVALID_ARGUMENT;
    alloy_comp_internal_t *comp = (alloy_comp_internal_t*)component;
#ifdef ALLOY_PLATFORM_WINDOWS
    DestroyWindow((HWND)comp->native_handle);
#elif defined(ALLOY_PLATFORM_LINUX)
    gtk_widget_destroy(GTK_WIDGET(comp->native_handle));
#endif
    if (comp->text) free(comp->text);
    free(comp);
    return ALLOY_OK;
}

// --- Component Creation ---

alloy_error_t alloy_create_button(alloy_component_t parent, alloy_component_t *out_button) {
    alloy_comp_internal_t *p = (alloy_comp_internal_t*)parent;
    alloy_comp_internal_t *btn = (alloy_comp_internal_t*)calloc(1, sizeof(alloy_comp_internal_t));

#ifdef ALLOY_PLATFORM_WINDOWS
    btn->native_handle = CreateWindowExW(0, L"BUTTON", L"Button", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, (HWND)p->native_handle, NULL, NULL, NULL);
#elif defined(ALLOY_PLATFORM_LINUX)
    btn->native_handle = gtk_button_new_with_label("Button");
    gtk_container_add(GTK_CONTAINER(p->native_handle), GTK_WIDGET(btn->native_handle));
    gtk_widget_show(GTK_WIDGET(btn->native_handle));
#endif

    if (!btn->native_handle) { free(btn); return ALLOY_ERROR_PLATFORM; }
    *out_button = (alloy_component_t)btn;
    return ALLOY_OK;
}

alloy_error_t alloy_create_textfield(alloy_component_t parent, alloy_component_t *out_textfield) {
    alloy_comp_internal_t *p = (alloy_comp_internal_t*)parent;
    alloy_comp_internal_t *tf = (alloy_comp_internal_t*)calloc(1, sizeof(alloy_comp_internal_t));

#ifdef ALLOY_PLATFORM_WINDOWS
    tf->native_handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT, 0, 0, 100, 25, (HWND)p->native_handle, NULL, NULL, NULL);
#elif defined(ALLOY_PLATFORM_LINUX)
    tf->native_handle = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(p->native_handle), GTK_WIDGET(tf->native_handle));
    gtk_widget_show(GTK_WIDGET(tf->native_handle));
#endif

    if (!tf->native_handle) { free(tf); return ALLOY_ERROR_PLATFORM; }
    *out_textfield = (alloy_component_t)tf;
    return ALLOY_OK;
}

// --- Property Implementations (Simplified) ---

alloy_error_t alloy_set_text(alloy_component_t component, const char *text) {
    alloy_comp_internal_t *comp = (alloy_comp_internal_t*)component;
#ifdef ALLOY_PLATFORM_LINUX
    if (GTK_IS_LABEL(comp->native_handle)) gtk_label_set_text(GTK_LABEL(comp->native_handle), text);
    else if (GTK_IS_ENTRY(comp->native_handle)) gtk_entry_set_text(GTK_ENTRY(comp->native_handle), text);
    else if (GTK_IS_WINDOW(comp->native_handle)) gtk_window_set_title(GTK_WINDOW(comp->native_handle), text);
#endif
    return ALLOY_OK;
}

// --- Event Loop ---

alloy_error_t alloy_run(alloy_component_t window) {
#ifdef ALLOY_PLATFORM_WINDOWS
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#elif defined(ALLOY_PLATFORM_LINUX)
    gtk_main();
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t window) {
#ifdef ALLOY_PLATFORM_LINUX
    gtk_main_quit();
#elif defined(ALLOY_PLATFORM_WINDOWS)
    PostQuitMessage(0);
#endif
    return ALLOY_OK;
}

// --- Stubs for the rest of Requirement components/props ---
alloy_error_t alloy_create_textarea(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_label(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_checkbox(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_radiobutton(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_combobox(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_slider(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_progressbar(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_tabview(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_listview(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_treeview(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_webview(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_vstack(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_hstack(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_create_scrollview(alloy_component_t parent, alloy_component_t *out) { return ALLOY_ERROR_NOT_SUPPORTED; }
alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) { return ALLOY_OK; }
alloy_error_t alloy_layout(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t alloy_set_event_callback(alloy_component_t c, alloy_event_type_t e, alloy_event_cb_t cb, void *u) { return ALLOY_OK; }
alloy_error_t alloy_set_style(alloy_component_t c, const alloy_style_t *s) { return ALLOY_OK; }
alloy_error_t alloy_dispatch(alloy_component_t w, void (*f)(void *), void *a) { return ALLOY_OK; }
int alloy_get_text(alloy_component_t c, char *b, size_t l) { return 0; }
alloy_error_t alloy_set_checked(alloy_component_t c, int ck) { return ALLOY_OK; }
int alloy_get_checked(alloy_component_t c) { return 0; }
alloy_error_t alloy_set_value(alloy_component_t c, double v) { return ALLOY_OK; }
double alloy_get_value(alloy_component_t c) { return 0.0; }
alloy_error_t alloy_set_enabled(alloy_component_t c, int e) { return ALLOY_OK; }
int alloy_get_enabled(alloy_component_t c) { return 1; }
alloy_error_t alloy_set_visible(alloy_component_t c, int v) { return ALLOY_OK; }
int alloy_get_visible(alloy_component_t c) { return 1; }
alloy_error_t alloy_set_flex(alloy_component_t c, float f) { return ALLOY_OK; }
alloy_error_t alloy_set_padding(alloy_component_t c, float t, float r, float b, float l) { return ALLOY_OK; }
alloy_error_t alloy_set_margin(alloy_component_t c, float t, float r, float b, float l) { return ALLOY_OK; }
