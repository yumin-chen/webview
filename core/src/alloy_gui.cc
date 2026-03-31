#include "alloy/api.h"
#include "alloy/detail/backends/gtk_gui.hh"
#include "alloy/detail/backends/win32_gui.hh"
#include "alloy/detail/backends/cocoa_gui.hh"
#include <string>
#include <vector>

using namespace alloy::detail;

extern "C" {

const char* alloy_error_message(alloy_error_t err) {
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
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_window(title, width, height));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_textfield(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_textfield(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_button(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_button(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_label(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_label(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_checkbox(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_checkbox(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_textarea(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_textarea(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_slider(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_slider(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_progressbar(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_progressbar(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_switch(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_switch(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_radiobutton(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_radiobutton(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_combobox(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_combobox(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_spinner(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_spinner(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_loadingspinner(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_loadingspinner(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_statusbar(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_statusbar(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_listview(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_listview(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_tabview(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_tabview(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_webview(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_webview_comp(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_vstack(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_vstack(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_hstack(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_hstack(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_treeview(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_treeview(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_scrollview(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_image(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_image(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_groupbox(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_groupbox(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_link(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_link(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_icon(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_icon(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_accordion(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_accordion(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_popover(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_popover(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_badge(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_badge(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_chip(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_chip(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_card(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_card(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_rating(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_rating(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_separator(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_separator(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_filedialog(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_filedialog(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_contextmenu(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_contextmenu(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_richtexteditor(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_richtexteditor(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_codeeditor(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_codeeditor(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_menu(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_menu(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_menubar(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_menubar(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_toolbar(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_toolbar(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_dialog(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_dialog(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_colorpicker(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_colorpicker(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_datepicker(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_datepicker(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_timepicker(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_timepicker(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_splitter(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_splitter(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_tooltip(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_tooltip(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_component_t alloy_create_divider(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_divider(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}


alloy_error_t alloy_destroy(alloy_component_t handle) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete static_cast<component_base*>(handle);
    return ALLOY_OK;
}

alloy_error_t alloy_set_text(alloy_component_t h, const char *text) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_text(text);
}

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
    if (!container || !child) return ALLOY_ERROR_INVALID_ARGUMENT;
    auto c = static_cast<component_base*>(container);
    auto ch = static_cast<component_base*>(child);
    c->add_child(ch);
#ifdef WEBVIEW_PLATFORM_LINUX
    gtk_container_add(GTK_CONTAINER(c->native_handle()), GTK_WIDGET(ch->native_handle()));
    gtk_widget_show(GTK_WIDGET(ch->native_handle()));
#endif
    return ALLOY_OK;
}

// Stubs for other API functions to allow compilation
alloy_signal_t  alloy_signal_create_str(const char *initial) { return nullptr; }
alloy_error_t   alloy_signal_set_str(alloy_signal_t s, const char *v) { return ALLOY_OK; }
alloy_error_t   alloy_bind_property(alloy_component_t component, alloy_prop_id_t property, alloy_signal_t signal) { return ALLOY_OK; }
alloy_error_t   alloy_run(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t   alloy_terminate(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t   alloy_dispatch(alloy_component_t window, void (*fn)(void *arg), void *arg) { return ALLOY_OK; }
alloy_error_t alloy_get_text(alloy_component_t h, char *buf, size_t buf_len) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->get_text(buf, buf_len);
}
alloy_error_t alloy_set_checked(alloy_component_t h, int checked) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_checked(checked != 0);
}
int           alloy_get_checked(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<component_base*>(h)->get_checked() ? 1 : 0;
}
alloy_error_t alloy_set_value(alloy_component_t h, double value) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_value(value);
}
double        alloy_get_value(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<component_base*>(h)->get_value();
}
alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_enabled(enabled != 0);
}
int           alloy_get_enabled(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<component_base*>(h)->get_enabled() ? 1 : 0;
}
alloy_error_t alloy_set_visible(alloy_component_t h, int visible) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_visible(visible != 0);
}
int           alloy_get_visible(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<component_base*>(h)->get_visible() ? 1 : 0;
}
alloy_error_t alloy_set_style(alloy_component_t h, const alloy_style_t *style) {
    if (!h || !style) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_style(*style);
}
alloy_error_t alloy_set_flex(alloy_component_t h, float flex) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    YGNodeStyleSetFlex(static_cast<component_base*>(h)->yoga_node(), flex);
    return ALLOY_OK;
}
alloy_error_t alloy_set_padding(alloy_component_t h, float top, float right, float bottom, float left) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    YGNodeRef node = static_cast<component_base*>(h)->yoga_node();
    YGNodeStyleSetPadding(node, YGEdgeTop, top);
    YGNodeStyleSetPadding(node, YGEdgeRight, right);
    YGNodeStyleSetPadding(node, YGEdgeBottom, bottom);
    YGNodeStyleSetPadding(node, YGEdgeLeft, left);
    return ALLOY_OK;
}
alloy_error_t alloy_set_margin(alloy_component_t h, float top, float right, float bottom, float left) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    YGNodeRef node = static_cast<component_base*>(h)->yoga_node();
    YGNodeStyleSetMargin(node, YGEdgeTop, top);
    YGNodeStyleSetMargin(node, YGEdgeRight, right);
    YGNodeStyleSetMargin(node, YGEdgeBottom, bottom);
    YGNodeStyleSetMargin(node, YGEdgeLeft, left);
    return ALLOY_OK;
}
static void apply_layout(component_base *comp, component_base *parent = nullptr) {
    YGNodeRef node = comp->yoga_node();
    float x = YGNodeLayoutGetLeft(node);
    float y = YGNodeLayoutGetTop(node);
    float width = YGNodeLayoutGetWidth(node);
    float height = YGNodeLayoutGetHeight(node);

#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *widget = static_cast<GtkWidget *>(comp->native_handle());
    if (GTK_IS_WIDGET(widget)) {
        gtk_widget_set_size_request(widget, (int)width, (int)height);
        if (parent) {
            GtkWidget *parent_widget = static_cast<GtkWidget *>(parent->native_handle());
            if (GTK_IS_FIXED(parent_widget)) {
                gtk_fixed_move(GTK_FIXED(parent_widget), widget, (int)x, (int)y);
            }
        }
    }
#endif

    for (auto child : comp->children()) {
        apply_layout(child, comp);
    }
}

alloy_error_t alloy_layout(alloy_component_t window) {
    if (!window) return ALLOY_ERROR_INVALID_ARGUMENT;
    auto w = static_cast<component_base*>(window);
    YGNodeCalculateLayout(w->yoga_node(), YGUndefined, YGUndefined, YGDirectionLTR);
    apply_layout(w);
    return ALLOY_OK;
}
alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    static_cast<component_base*>(handle)->set_event_callback(event, callback, userdata);
    return ALLOY_OK;
}
alloy_signal_t  alloy_signal_create_double(double initial) { return nullptr; }
alloy_signal_t  alloy_signal_create_int(int initial) { return nullptr; }
alloy_signal_t  alloy_signal_create_bool(int initial) { return nullptr; }
alloy_error_t   alloy_signal_set_double(alloy_signal_t s, double v) { return ALLOY_OK; }
alloy_error_t   alloy_signal_set_int(alloy_signal_t s, int v) { return ALLOY_OK; }
alloy_error_t   alloy_signal_set_bool(alloy_signal_t s, int v) { return ALLOY_OK; }
const char     *alloy_signal_get_str(alloy_signal_t s) { return nullptr; }
double          alloy_signal_get_double(alloy_signal_t s) { return 0; }
int             alloy_signal_get_int(alloy_signal_t s) { return 0; }
int             alloy_signal_get_bool(alloy_signal_t s) { return 0; }
alloy_computed_t alloy_computed_create(alloy_signal_t *deps, size_t dep_count, void (*compute)(alloy_signal_t *deps, size_t dep_count, void *out, void *userdata), void *userdata) { return nullptr; }
alloy_effect_t  alloy_effect_create(alloy_signal_t *deps, size_t dep_count, void (*run)(void *userdata), void *userdata) { return nullptr; }
alloy_error_t   alloy_signal_destroy(alloy_signal_t s) { return ALLOY_OK; }
alloy_error_t   alloy_computed_destroy(alloy_computed_t c) { return ALLOY_OK; }
alloy_error_t   alloy_effect_destroy(alloy_effect_t e) { return ALLOY_OK; }
alloy_error_t   alloy_unbind_property(alloy_component_t component, alloy_prop_id_t property) { return ALLOY_OK; }

}
