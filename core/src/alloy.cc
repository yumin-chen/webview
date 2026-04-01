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
#elif defined(ALLOY_PLATFORM_DARWIN)
#include "detail/backends/cocoa_gui.hh"
using component_impl = alloy::detail::cocoa_component;
using window_impl = alloy::detail::cocoa_window;
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
#if defined(ALLOY_PLATFORM_LINUX) || defined(ALLOY_PLATFORM_WINDOWS) || defined(ALLOY_PLATFORM_DARWIN)
    return new window_impl(title, width, height);
#else
    (void)title; (void)width; (void)height;
    return nullptr;
#endif
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

alloy_component_t alloy_create_spinner(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto spin = new gtk_component(gtk_spinner_new());
    gtk_spinner_start(GTK_SPINNER(spin->native_handle()));
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(spin->native_handle()));
    return spin;
#endif
    return nullptr;
}

alloy_component_t alloy_create_menubar(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto mb = new gtk_component(gtk_menu_bar_new());
    if (parent && GTK_IS_BOX(cast(parent)->native_handle())) {
        gtk_box_pack_start(GTK_BOX(cast(parent)->native_handle()), GTK_WIDGET(mb->native_handle()), FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(cast(parent)->native_handle()), GTK_WIDGET(mb->native_handle()), 0);
    }
    return mb;
#endif
    return nullptr;
}

alloy_component_t alloy_create_link(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto link = new gtk_component(gtk_link_button_new(""));
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(link->native_handle()));
    return link;
#endif
    return nullptr;
}

alloy_component_t alloy_create_separator(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto sep = new gtk_component(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(sep->native_handle()));
    return sep;
#endif
    return nullptr;
}

alloy_component_t alloy_create_menu(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto m = new gtk_component(gtk_menu_new());
    if (parent && GTK_IS_MENU_ITEM(cast(parent)->native_handle())) {
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(cast(parent)->native_handle()), GTK_WIDGET(m->native_handle()));
    }
    return m;
#endif
    return nullptr;
}

alloy_component_t alloy_create_menuitem(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto mi = new gtk_component(gtk_menu_item_new_with_label(""));
    if (parent) {
        if (GTK_IS_MENU_SHELL(cast(parent)->native_handle())) {
            gtk_menu_shell_append(GTK_MENU_SHELL(cast(parent)->native_handle()), GTK_WIDGET(mi->native_handle()));
        }
    }
    return mi;
#endif
    return nullptr;
}

alloy_component_t alloy_create_label(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto lbl = new gtk_component(gtk_label_new(""));
    gtk_label_set_xalign(GTK_LABEL(lbl->native_handle()), 0.0);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(lbl->native_handle()));
    return lbl;
#elif defined(ALLOY_PLATFORM_WINDOWS)
    auto lbl = new win32_component(CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, (HWND)cast(parent)->native_handle(), NULL, GetModuleHandle(NULL), NULL));
    return lbl;
#endif
    return nullptr;
}

alloy_component_t alloy_create_switch(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto sw = new gtk_component(gtk_switch_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(sw->native_handle()));
    return sw;
#endif
    return nullptr;
}

alloy_component_t alloy_create_image(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto img = new gtk_component(gtk_image_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(img->native_handle()));
    return img;
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

alloy_error_t alloy_image_load_file(alloy_component_t h, const char *path) {
#ifdef ALLOY_PLATFORM_LINUX
    gtk_image_set_from_file(GTK_IMAGE(cast(h)->native_handle()), path);
    return ALLOY_OK;
#endif
    return ALLOY_ERROR_NOT_SUPPORTED;
}

alloy_error_t alloy_listview_append(alloy_component_t h, const char *text) {
#ifdef ALLOY_PLATFORM_LINUX
    GtkTreeView *tv = GTK_TREE_VIEW(cast(h)->native_handle());
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, text, -1);
    return ALLOY_OK;
#endif
    return ALLOY_ERROR_NOT_SUPPORTED;
}

alloy_error_t alloy_tabview_add_page(alloy_component_t h, alloy_component_t child, const char *label) {
#ifdef ALLOY_PLATFORM_LINUX
    GtkNotebook *nb = GTK_NOTEBOOK(cast(h)->native_handle());
    gtk_notebook_append_page(nb, GTK_WIDGET(cast(child)->native_handle()), gtk_label_new(label));
    return ALLOY_OK;
#endif
    return ALLOY_ERROR_NOT_SUPPORTED;
}

alloy_error_t alloy_combobox_append(alloy_component_t h, const char *text) {
#ifdef ALLOY_PLATFORM_LINUX
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cast(h)->native_handle()), text);
    return ALLOY_OK;
#endif
    return ALLOY_ERROR_NOT_SUPPORTED;
}

alloy_error_t alloy_webview_load_url(alloy_component_t h, const char *url) {
#ifdef ALLOY_PLATFORM_LINUX
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(cast(h)->native_handle()), url);
    return ALLOY_OK;
#endif
    return ALLOY_ERROR_NOT_SUPPORTED;
}

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

const char* alloy_dialog_file_open(alloy_component_t parent, const char* title) {
#ifdef ALLOY_PLATFORM_LINUX
    GtkWidget *dialog = gtk_file_chooser_dialog_new(title,
        GTK_WINDOW(cast(parent)->native_handle()),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);
    static std::string result;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        result = filename;
        g_free(filename);
    } else {
        result = "";
    }
    gtk_widget_destroy(dialog);
    return result.empty() ? NULL : result.c_str();
#endif
    return NULL;
}

const char* alloy_dialog_color_picker(alloy_component_t parent, const char* title) {
#ifdef ALLOY_PLATFORM_LINUX
    GtkWidget *dialog = gtk_color_chooser_dialog_new(title, GTK_WINDOW(cast(parent)->native_handle()));
    static std::string result;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        char buf[16];
        snprintf(buf, sizeof(buf), "#%02x%02x%02x",
            (int)(color.red * 255), (int)(color.green * 255), (int)(color.blue * 255));
        result = buf;
    } else {
        result = "";
    }
    gtk_widget_destroy(dialog);
    return result.empty() ? NULL : result.c_str();
#endif
    return NULL;
}

alloy_error_t alloy_run(alloy_component_t window) {
#if defined(ALLOY_PLATFORM_LINUX)
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
#elif defined(ALLOY_PLATFORM_DARWIN)
    (void)window;
    // In a real implementation, we would use [NSApp run]
#elif defined(ALLOY_PLATFORM_WINDOWS)
    (void)window;
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
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

// Implementations for core UI functions
alloy_component_t alloy_create_textfield(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto entry = new gtk_component(gtk_entry_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(entry->native_handle()));
    return entry;
#endif
    return nullptr;
}

alloy_component_t alloy_create_textarea(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto tv = new gtk_component(gtk_text_view_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(tv->native_handle()));
    return tv;
#endif
    return nullptr;
}

alloy_component_t alloy_create_checkbox(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto cb = new gtk_component(gtk_check_button_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(cb->native_handle()));
    return cb;
#endif
    return nullptr;
}

alloy_component_t alloy_create_radiobutton(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto rb = new gtk_component(gtk_radio_button_new(NULL));
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(rb->native_handle()));
    return rb;
#endif
    return nullptr;
}

alloy_component_t alloy_create_combobox(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto cb = new gtk_component(gtk_combo_box_text_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(cb->native_handle()));
    return cb;
#endif
    return nullptr;
}

alloy_component_t alloy_create_slider(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto scale = new gtk_component(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1));
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(scale->native_handle()));
    return scale;
#endif
    return nullptr;
}
alloy_component_t alloy_create_tabview(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto nb = new gtk_component(gtk_notebook_new(), true);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(nb->native_handle()));
    return nb;
#endif
    return nullptr;
}

alloy_component_t alloy_create_listview(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    auto tv = new gtk_component(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store)));
    g_object_unref(store);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tv->native_handle()), -1, "Items", renderer, "text", 0, NULL);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(tv->native_handle()));
    return tv;
#endif
    return nullptr;
}

alloy_component_t alloy_create_treeview(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);
    auto tv = new gtk_component(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store)));
    g_object_unref(store);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tv->native_handle()), -1, "Nodes", renderer, "text", 0, NULL);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(tv->native_handle()));
    return tv;
#endif
    return nullptr;
}

alloy_component_t alloy_create_webview(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto wv = new gtk_component(webkit_web_view_new());
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(wv->native_handle()));
    return wv;
#endif
    return nullptr;
}

alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
#ifdef ALLOY_PLATFORM_LINUX
    auto sv = new gtk_component(gtk_scrolled_window_new(NULL, NULL), true);
    if (parent) gtk_container_add(GTK_CONTAINER(cast(parent)->native_handle()), GTK_WIDGET(sv->native_handle()));
    return sv;
#endif
    return nullptr;
}
alloy_error_t alloy_set_flex(alloy_component_t h, float flex) { return ALLOY_OK; }
alloy_error_t alloy_set_padding(alloy_component_t h, float t, float r, float b, float l) { return ALLOY_OK; }
alloy_error_t alloy_set_margin(alloy_component_t h, float t, float r, float b, float l) { return ALLOY_OK; }
alloy_error_t alloy_layout(alloy_component_t window) { return ALLOY_OK; }
