#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif
#include "webview/detail/json.hh"
#include <string>
#include <vector>

using namespace alloy::detail;

extern "C" {

ALLOY_API const char *alloy_error_message(alloy_error_t err) {
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

// Signal system impl
ALLOY_API alloy_signal_t alloy_signal_create_str(const char *initial) {
    return new signal(signal_value(std::string(initial)));
}
ALLOY_API alloy_signal_t alloy_signal_create_double(double initial) {
    return new signal(signal_value(initial));
}
ALLOY_API alloy_signal_t alloy_signal_create_int(int initial) {
    return new signal(signal_value(initial));
}
ALLOY_API alloy_signal_t alloy_signal_create_bool(int initial) {
    return new signal(signal_value(initial != 0));
}

ALLOY_API alloy_error_t alloy_signal_set_str(alloy_signal_t s, const char *v) {
    static_cast<signal*>(s)->set_value(signal_value(std::string(v)));
    return ALLOY_OK;
}
ALLOY_API alloy_error_t alloy_signal_set_double(alloy_signal_t s, double v) {
    static_cast<signal*>(s)->set_value(signal_value(v));
    return ALLOY_OK;
}
ALLOY_API alloy_error_t alloy_signal_set_int(alloy_signal_t s, int v) {
    static_cast<signal*>(s)->set_value(signal_value(v));
    return ALLOY_OK;
}
ALLOY_API alloy_error_t alloy_signal_set_bool(alloy_signal_t s, int v) {
    static_cast<signal*>(s)->set_value(signal_value(v != 0));
    return ALLOY_OK;
}

ALLOY_API alloy_error_t alloy_bind_property(alloy_component_t component, alloy_prop_id_t property, alloy_signal_t signal) {
    static_cast<component_base*>(component)->bind_property(property, static_cast<signal_base*>(signal));
    return ALLOY_OK;
}

// Component creation
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_window(const char *title, int width, int height) {
    return new gtk_window(title, width, height);
}

ALLOY_API alloy_component_t alloy_create_button(alloy_component_t parent, const char* label) {
    auto* btn = new gtk_component(gtk_button_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(btn);
    return btn;
}

ALLOY_API alloy_component_t alloy_create_label(alloy_component_t parent, const char* text) {
    auto* lbl = new gtk_component(gtk_label_new(text));
    if (parent) static_cast<component_base*>(parent)->add_child(lbl);
    return lbl;
}

ALLOY_API alloy_component_t alloy_create_textfield(alloy_component_t parent) {
    auto* entry = new gtk_component(gtk_entry_new());
    if (parent) static_cast<component_base*>(parent)->add_child(entry);
    return entry;
}

ALLOY_API alloy_component_t alloy_create_textarea(alloy_component_t parent) {
    auto* tv = gtk_text_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_checkbox(alloy_component_t parent, const char* label) {
    auto* cb = new gtk_component(gtk_check_button_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(cb);
    return cb;
}

ALLOY_API alloy_component_t alloy_create_radiobutton(alloy_component_t parent, const char* label, const char* name, const char* value) {
    auto* rb = new gtk_component(gtk_radio_button_new_with_label(NULL, label));
    // Implementation for groups (name) and values would go here
    if (parent) static_cast<component_base*>(parent)->add_child(rb);
    return rb;
}

ALLOY_API alloy_component_t alloy_create_combobox(alloy_component_t parent) {
    auto* combo = new gtk_component(gtk_combo_box_text_new());
    if (parent) static_cast<component_base*>(parent)->add_child(combo);
    return combo;
}

ALLOY_API alloy_component_t alloy_create_slider(alloy_component_t parent) {
    auto* slider = new gtk_component(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0));
    if (parent) static_cast<component_base*>(parent)->add_child(slider);
    return slider;
}

ALLOY_API alloy_component_t alloy_create_progressbar(alloy_component_t parent) {
    auto* pb = new gtk_component(gtk_progress_bar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(pb);
    return pb;
}

ALLOY_API alloy_component_t alloy_create_tabview(alloy_component_t parent) {
    auto* nb = new gtk_container(gtk_notebook_new());
    if (parent) static_cast<component_base*>(parent)->add_child(nb);
    return nb;
}

ALLOY_API alloy_component_t alloy_create_listview(alloy_component_t parent) {
    auto* tv = gtk_tree_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_treeview(alloy_component_t parent) {
    auto* tv = gtk_tree_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_webview(alloy_component_t parent) {
    auto* wv = webkit_web_view_new();
    auto* comp = new gtk_component(wv);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_vstack(alloy_component_t parent) {
    auto* box = new gtk_container(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    if (parent) static_cast<component_base*>(parent)->add_child(box);
    return box;
}

ALLOY_API alloy_component_t alloy_create_hstack(alloy_component_t parent) {
    auto* box = new gtk_container(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    if (parent) static_cast<component_base*>(parent)->add_child(box);
    return box;
}

ALLOY_API alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
    auto* sw = new gtk_container(gtk_scrolled_window_new(NULL, NULL));
    if (parent) static_cast<component_base*>(parent)->add_child(sw);
    return sw;
}

ALLOY_API alloy_component_t alloy_create_spinner(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_spin_button_new_with_range(0, 100, 1));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_menubar(alloy_component_t parent) {
    auto* mb = new gtk_container(gtk_menu_bar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(mb);
    return mb;
}

ALLOY_API alloy_component_t alloy_create_menu(alloy_component_t parent, const char* label) {
    auto* m = new gtk_container(gtk_menu_new());
    if (parent) static_cast<component_base*>(parent)->add_child(m);
    return m;
}

ALLOY_API alloy_component_t alloy_create_menuitem(alloy_component_t parent, const char* label) {
    auto* mi = new gtk_component(gtk_menu_item_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(mi);
    return mi;
}

ALLOY_API alloy_component_t alloy_create_toolbar(alloy_component_t parent) {
    auto* tb = new gtk_container(gtk_toolbar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(tb);
    return tb;
}

ALLOY_API alloy_component_t alloy_create_statusbar(alloy_component_t parent) {
    auto* sb = new gtk_component(gtk_statusbar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(sb);
    return sb;
}

ALLOY_API alloy_component_t alloy_create_splitter(alloy_component_t parent, int vertical) {
    auto* p = new gtk_container(gtk_paned_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL));
    if (parent) static_cast<component_base*>(parent)->add_child(p);
    return p;
}

ALLOY_API alloy_component_t alloy_create_dialog(alloy_component_t parent, const char* title) {
    auto* d = new gtk_window(title, 400, 300); // Simple dialog based on window for now
    if (parent) static_cast<component_base*>(parent)->add_child(d);
    return d;
}

ALLOY_API alloy_component_t alloy_create_image(alloy_component_t parent, const char* src) {
    auto* img = new gtk_component(gtk_image_new_from_file(src));
    if (parent) static_cast<component_base*>(parent)->add_child(img);
    return img;
}

ALLOY_API alloy_component_t alloy_create_groupbox(alloy_component_t parent, const char* label) {
    auto* f = new gtk_container(gtk_frame_new(label));
    if (parent) static_cast<component_base*>(parent)->add_child(f);
    return f;
}

ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_switch_new());
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_datepicker(alloy_component_t parent) {
    auto* c = new gtk_component(gtk_calendar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(c);
    return c;
}

ALLOY_API alloy_component_t alloy_create_colorpicker(alloy_component_t parent) {
    auto* cb = new gtk_component(gtk_color_button_new());
    if (parent) static_cast<component_base*>(parent)->add_child(cb);
    return cb;
}

ALLOY_API alloy_component_t alloy_create_link(alloy_component_t parent, const char* text, const char* url) {
    auto* l = new gtk_component(gtk_link_button_new_with_label(url, text));
    if (parent) static_cast<component_base*>(parent)->add_child(l);
    return l;
}

ALLOY_API alloy_component_t alloy_create_timepicker(alloy_component_t parent) {
    auto* b = new gtk_container(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2));
    gtk_container_add(GTK_CONTAINER(static_cast<GtkWidget*>(b->native_handle())), gtk_spin_button_new_with_range(0, 23, 1));
    gtk_container_add(GTK_CONTAINER(static_cast<GtkWidget*>(b->native_handle())), gtk_spin_button_new_with_range(0, 59, 1));
    if (parent) static_cast<component_base*>(parent)->add_child(b);
    return b;
}

ALLOY_API alloy_component_t alloy_create_tooltip(alloy_component_t parent, const char* text) {
    auto* l = new gtk_component(gtk_label_new(text));
    if (parent) static_cast<component_base*>(parent)->add_child(l);
    return l;
}

ALLOY_API alloy_component_t alloy_create_divider(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_icon(alloy_component_t parent, const char* name) {
    auto* i = new gtk_component(gtk_image_new_from_icon_name(name, GTK_ICON_SIZE_BUTTON));
    if (parent) static_cast<component_base*>(parent)->add_child(i);
    return i;
}

ALLOY_API alloy_component_t alloy_create_separator(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_accordion(alloy_component_t parent) {
    auto* e = new gtk_container(gtk_expander_new(""));
    if (parent) static_cast<component_base*>(parent)->add_child(e);
    return e;
}

ALLOY_API alloy_component_t alloy_create_popover(alloy_component_t parent) {
    auto* p = new gtk_container(gtk_popover_new(NULL));
    if (parent) static_cast<component_base*>(parent)->add_child(p);
    return p;
}

ALLOY_API alloy_component_t alloy_create_contextmenu(alloy_component_t parent) {
    auto* m = new gtk_container(gtk_menu_new());
    if (parent) static_cast<component_base*>(parent)->add_child(m);
    return m;
}

ALLOY_API alloy_component_t alloy_create_badge(alloy_component_t parent, const char* text) {
    auto* l = new gtk_component(gtk_label_new(text));
    if (parent) static_cast<component_base*>(parent)->add_child(l);
    return l;
}

ALLOY_API alloy_component_t alloy_create_chip(alloy_component_t parent, const char* label) {
    auto* b = new gtk_component(gtk_button_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(b);
    return b;
}

ALLOY_API alloy_component_t alloy_create_loading_spinner(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_spinner_new());
    gtk_spinner_start(GTK_SPINNER(static_cast<GtkWidget*>(s->native_handle())));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_card(alloy_component_t parent) {
    auto* f = new gtk_container(gtk_frame_new(NULL));
    if (parent) static_cast<component_base*>(parent)->add_child(f);
    return f;
}

ALLOY_API alloy_component_t alloy_create_rating(alloy_component_t parent) {
    auto* b = new gtk_container(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    if (parent) static_cast<component_base*>(parent)->add_child(b);
    return b;
}

ALLOY_API alloy_component_t alloy_create_richtexteditor(alloy_component_t parent) {
    auto* tv = gtk_text_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_codeeditor(alloy_component_t parent) {
    auto* tv = gtk_text_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_file_dialog(alloy_component_t parent, int save) {
    auto* fd = gtk_file_chooser_dialog_new("Select File", NULL,
        save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        save ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
        NULL);
    auto* comp = new gtk_component(fd);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_error_t alloy_destroy(alloy_component_t handle) {
    delete static_cast<component_base*>(handle);
    return ALLOY_OK;
}

ALLOY_API alloy_error_t alloy_set_text(alloy_component_t h, const char *text) {
    return static_cast<component_base*>(h)->set_text(text);
}

ALLOY_API alloy_error_t alloy_layout(alloy_component_t window) {
    auto* win = static_cast<component_base*>(window);
    // For Window, we get its current size
    int w, h;
    GtkWidget* widget = static_cast<GtkWidget*>(win->native_handle());
    gtk_window_get_size(GTK_WINDOW(widget), &w, &h);

    YGNodeCalculateLayout(win->yoga_node(), (float)w, (float)h, YGDirectionLTR);
    win->apply_layout();
    return ALLOY_OK;
}

ALLOY_API alloy_error_t alloy_run(alloy_component_t window) {
    gtk_main();
    return ALLOY_OK;
}
#else
// Non-linux stubs or other platform implementations
ALLOY_API alloy_component_t alloy_create_window(const char *, int, int) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_button(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_label(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_textfield(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_textarea(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_checkbox(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_radiobutton(alloy_component_t, const char*, const char*, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_combobox(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_slider(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_progressbar(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_tabview(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_listview(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_treeview(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_webview(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_vstack(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_hstack(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_scrollview(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_spinner(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_menubar(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_menu(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_menuitem(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_toolbar(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_statusbar(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_splitter(alloy_component_t, int) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_dialog(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_image(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_groupbox(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_datepicker(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_colorpicker(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_link(alloy_component_t, const char*, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_timepicker(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_tooltip(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_divider(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_icon(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_separator(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_accordion(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_popover(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_contextmenu(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_badge(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_chip(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_loading_spinner(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_card(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_rating(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_richtexteditor(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_codeeditor(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_file_dialog(alloy_component_t, int) { return nullptr; }
ALLOY_API alloy_error_t alloy_destroy(alloy_component_t) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_set_text(alloy_component_t, const char *) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_layout(alloy_component_t) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_run(alloy_component_t) { return ALLOY_OK; }
#endif

}
