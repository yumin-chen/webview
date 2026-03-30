#include "alloy/api.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstring>

#if defined(__APPLE__)
#define ALLOY_PLATFORM_DARWIN
#elif defined(_WIN32)
#define ALLOY_PLATFORM_WINDOWS
#else
#define ALLOY_PLATFORM_LINUX
#endif

#ifdef ALLOY_PLATFORM_LINUX
#include "detail/backends/gtk_gui.hh"
using component_impl = alloy::detail::gtk_component;
using window_impl = alloy::detail::gtk_window;
#elif defined(ALLOY_PLATFORM_WINDOWS)
#include "detail/backends/win32_gui.hh"
using component_impl = alloy::detail::win32_component;
using window_impl = alloy::detail::win32_window;
#endif

using namespace alloy::detail;

static component_base* cast(alloy_component_t h) { return static_cast<component_base*>(h); }

const char *alloy_error_message(alloy_error_t err) {
    switch (err) {
        case ALLOY_OK: return "OK";
        case ALLOY_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case ALLOY_ERROR_INVALID_STATE: return "Invalid state";
        case ALLOY_ERROR_PLATFORM: return "Platform error";
        case ALLOY_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ALLOY_ERROR_NOT_SUPPORTED: return "Not supported";
        default: return "Unknown error";
    }
}

alloy_component_t alloy_create_window(const char *title, int width, int height) {
    return new window_impl(title, width, height);
}

alloy_component_t alloy_create_button(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto btn = new gtk_component(gtk_button_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(btn->native_handle()));
    return btn;
#elif defined(ALLOY_PLATFORM_WINDOWS)
    auto btn = new win32_component(CreateWindowExA(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, (HWND)cast(parent)->native_handle(), NULL, GetModuleHandle(NULL), NULL));
    return btn;
#endif
    return nullptr;
}

alloy_component_t alloy_create_label(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto lbl = new gtk_component(gtk_label_new(""));
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(lbl->native_handle()));
    return lbl;
#elif defined(ALLOY_PLATFORM_WINDOWS)
    auto lbl = new win32_component(CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, (HWND)cast(parent)->native_handle(), NULL, GetModuleHandle(NULL), NULL));
    return lbl;
#endif
    return nullptr;
}

alloy_component_t alloy_create_progressbar(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto pb = new gtk_component(gtk_progress_bar_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(pb->native_handle()));
    return pb;
#elif defined(ALLOY_PLATFORM_WINDOWS)
    auto pb = new win32_component(CreateWindowExA(0, PROGRESS_CLASS, "", WS_CHILD | WS_VISIBLE, 0, 0, 200, 20, (HWND)cast(parent)->native_handle(), NULL, GetModuleHandle(NULL), NULL));
    return pb;
#endif
    return nullptr;
}

alloy_component_t alloy_create_vstack(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto vs = new gtk_component(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10), true);
    gtk_container_set_border_width(GTK_CONTAINER(vs->native_handle()), 10);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(vs->native_handle()));
    return vs;
#endif
    return nullptr;
}

alloy_component_t alloy_create_hstack(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto hs = new gtk_component(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10), true);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(hs->native_handle()));
    return hs;
#endif
    return nullptr;
}

alloy_error_t alloy_destroy(alloy_component_t handle) {
    delete cast(handle);
    return ALLOY_OK;
}

alloy_error_t alloy_set_text(alloy_component_t h, const char *text) { return cast(h)->set_text(text); }
alloy_error_t alloy_get_text(alloy_component_t h, char *buf, size_t buf_len) { return cast(h)->get_text(buf, buf_len); }
alloy_error_t alloy_set_checked(alloy_component_t h, int checked) { return cast(h)->set_checked(checked); }
int alloy_get_checked(alloy_component_t h) { return cast(h)->get_checked(); }
alloy_error_t alloy_set_value(alloy_component_t h, double value) { return cast(h)->set_value(value); }
double alloy_get_value(alloy_component_t h) { return cast(h)->get_value(); }
alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled) { return cast(h)->set_enabled(enabled); }
int alloy_get_enabled(alloy_component_t h) { return cast(h)->get_enabled(); }
alloy_error_t alloy_set_visible(alloy_component_t h, int visible) { return cast(h)->set_visible(visible); }
int alloy_get_visible(alloy_component_t h) { return cast(h)->get_visible(); }
alloy_error_t alloy_set_style(alloy_component_t h, const alloy_style_t *style) { return cast(h)->set_style(*style); }

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
#ifdef ALLOY_PLATFORM_LINUX
    if (GTK_IS_CONTAINER(cast(container)->native_handle())) {
        gtk_container_add(GTK_CONTAINER(cast(container)->native_handle()), GTK_WIDGET(cast(child)->native_handle()));
        return ALLOY_OK;
    }
#elif defined(ALLOY_PLATFORM_WINDOWS)
    SetParent((HWND)cast(child)->native_handle(), (HWND)cast(container)->native_handle());
    return ALLOY_OK;
#endif
    return ALLOY_ERROR_INVALID_ARGUMENT;
}

alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) {
    cast(handle)->set_event_callback(event, callback, userdata);
    return ALLOY_OK;
}

alloy_error_t alloy_run(alloy_component_t window) {
#ifdef ALLOY_PLATFORM_LINUX
    auto widget = GTK_WIDGET(cast(window)->native_handle());
    gtk_widget_show_all(widget);

    // Add some default styling for "professional" look
    const char *css =
        "window { background-color: #f0f0f0; }"
        "button { padding: 8px 16px; border-radius: 4px; background: #0078d4; color: white; border: none; }"
        "button:hover { background: #005a9e; }"
        "label { font-size: 14px; color: #333; }";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    gtk_main();
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t window) {
#ifdef ALLOY_PLATFORM_LINUX
    gtk_main_quit();
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_dispatch(alloy_component_t window, void (*fn)(void *arg), void *arg) {
#ifdef ALLOY_PLATFORM_LINUX
    g_idle_add(+[](gpointer data) -> gboolean {
        auto pair = static_cast<std::pair<void(*)(void*), void*>*>(data);
        pair->first(pair->second);
        delete pair;
        return FALSE;
    }, new std::pair<void(*)(void*), void*>(fn, arg));
#endif
    return ALLOY_OK;
}

// Dummy stubs for unimplemented create functions to make it link
alloy_component_t alloy_create_textfield(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_textarea(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_checkbox(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_radiobutton(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_combobox(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_slider(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_tabview(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_listview(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_treeview(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_webview(alloy_component_t p) { return nullptr; }
alloy_component_t alloy_create_scrollview(alloy_component_t p) { return nullptr; }
alloy_error_t alloy_set_flex(alloy_component_t h, float flex) { return ALLOY_OK; }
alloy_error_t alloy_set_padding(alloy_component_t h, float t, float r, float b, float l) { return ALLOY_OK; }
alloy_error_t alloy_set_margin(alloy_component_t h, float t, float r, float b, float l) { return ALLOY_OK; }
alloy_error_t alloy_layout(alloy_component_t window) { return ALLOY_OK; }
