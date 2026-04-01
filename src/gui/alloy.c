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

alloy_error_t alloy_destroy(alloy_component_t component) {
    if (!component) return ALLOY_ERROR_INVALID_ARGUMENT;
#ifdef ALLOY_PLATFORM_WINDOWS
    DestroyWindow((HWND)component);
#elif defined(ALLOY_PLATFORM_LINUX)
    gtk_widget_destroy(GTK_WIDGET(component));
#endif
    return ALLOY_OK;
}

// --- Global API Stubs ---
alloy_error_t alloy_terminate(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t alloy_dispatch(alloy_component_t window, void (*fn)(void *), void *arg) { return ALLOY_OK; }
alloy_error_t alloy_set_text(alloy_component_t component, const char *text) { return ALLOY_OK; }
int alloy_get_text(alloy_component_t component, char *buf, size_t buf_len) { return 0; }
alloy_error_t alloy_set_checked(alloy_component_t component, int checked) { return ALLOY_OK; }
int alloy_get_checked(alloy_component_t component) { return 0; }
alloy_error_t alloy_set_value(alloy_component_t component, double value) { return ALLOY_OK; }
double alloy_get_value(alloy_component_t component) { return 0.0; }
alloy_error_t alloy_set_enabled(alloy_component_t component, int enabled) { return ALLOY_OK; }
int alloy_get_enabled(alloy_component_t component) { return 1; }
alloy_error_t alloy_set_visible(alloy_component_t component, int visible) { return ALLOY_OK; }
int alloy_get_visible(alloy_component_t component) { return 1; }
alloy_error_t alloy_set_event_callback(alloy_component_t component, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) { return ALLOY_OK; }
alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) { return ALLOY_OK; }
alloy_error_t alloy_layout(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t alloy_set_flex(alloy_component_t component, float flex) { return ALLOY_OK; }
alloy_error_t alloy_set_padding(alloy_component_t component, float top, float right, float bottom, float left) { return ALLOY_OK; }
alloy_error_t alloy_set_margin(alloy_component_t component, float top, float right, float bottom, float left) { return ALLOY_OK; }
alloy_error_t alloy_set_style(alloy_component_t component, const alloy_style_t *style) { return ALLOY_OK; }

// --- More Component Stubs (Placeholder for full extraction) ---
alloy_error_t alloy_create_datepicker(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_timepicker(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_colorpicker(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_switch(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_image(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_icon(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_tooltip(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_badge(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_card(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_divider(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_richtexteditor(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_treeview(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_scrollview(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_groupbox(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_toolbar(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_contextmenu(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_filedialog(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_popover(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_statusbar(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_splitter(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_webview(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_link(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_chip(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_rating(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_accordion(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_codeeditor(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_separator(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
alloy_error_t alloy_create_loadingspinner(alloy_component_t p, alloy_component_t *o) { return ALLOY_OK; }
