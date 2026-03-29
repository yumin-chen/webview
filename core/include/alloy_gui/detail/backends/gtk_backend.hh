#ifndef ALLOY_GUI_GTK_BACKEND_HH
#define ALLOY_GUI_GTK_BACKEND_HH

#include <gtk/gtk.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../../alloy_gui/api.h"

#include <webkit2/webkit2.h>

namespace alloy {
namespace detail {

enum class signal_type { STR, DOUBLE, INT, BOOL };
struct signal_value {
    signal_type type;
    std::string s;
    double d;
    int i;
    bool b;
};

struct signal_base {
    signal_value value;
    std::vector<std::pair<void*, alloy_prop_id_t>> subscribers;
    void notify();
};

struct Component {
    GtkWidget *widget;
    std::map<alloy_event_type_t, std::pair<alloy_event_cb_t, void*>> callbacks;
    std::vector<Component*> children;
    float flex = 0;
    bool is_container = false;

    Component(GtkWidget *w) : widget(w) {}
    virtual ~Component() {
        if (widget) {
            gtk_widget_destroy(widget);
        }
    }

    void on_signal_changed(alloy_prop_id_t prop, const signal_value& val);
};

class GTKBackend {
public:
    static Component* create_window(const char *title, int width, int height) {
        GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), title);
        gtk_window_set_default_size(GTK_WINDOW(window), width, height);
        return new Component(window);
    }

    static Component* create_button(Component *parent) {
        GtkWidget *button = gtk_button_new();
        Component *comp = new Component(button);
        g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), comp);
        return comp;
    }

    static Component* create_textfield(Component *parent) {
        GtkWidget *entry = gtk_entry_new();
        Component *comp = new Component(entry);
        g_signal_connect(entry, "changed", G_CALLBACK(on_changed), comp);
        return comp;
    }

    static Component* create_textarea(Component *parent) {
        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        GtkWidget *text = gtk_text_view_new();
        gtk_container_add(GTK_CONTAINER(scroll), text);
        Component *comp = new Component(scroll);
        return comp;
    }

    static Component* create_label(Component *parent) {
        return new Component(gtk_label_new(""));
    }

    static Component* create_checkbox(Component *parent) {
        GtkWidget *btn = gtk_check_button_new();
        Component *comp = new Component(btn);
        g_signal_connect(btn, "toggled", G_CALLBACK(on_changed), comp);
        return comp;
    }

    static Component* create_radiobutton(Component *parent) {
        return new Component(gtk_radio_button_new(NULL));
    }

    static Component* create_combobox(Component *parent) {
        return new Component(gtk_combo_box_text_new());
    }

    static Component* create_slider(Component *parent) {
        return new Component(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01));
    }

    static Component* create_spinner(Component *parent) {
        return new Component(gtk_spin_button_new_with_range(0, 100, 1));
    }

    static Component* create_progressbar(Component *parent) {
        return new Component(gtk_progress_bar_new());
    }

    static Component* create_tabview(Component *parent) {
        Component *comp = new Component(gtk_notebook_new());
        comp->is_container = true;
        return comp;
    }

    static Component* create_listview(Component *parent) {
        GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
        GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        return new Component(view);
    }

    static Component* create_treeview(Component *parent) {
        GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);
        GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        return new Component(view);
    }

    static Component* create_webview(Component *parent) {
        return new Component(webkit_web_view_new());
    }

    static Component* create_vstack(Component *parent) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        Component *comp = new Component(box);
        comp->is_container = true;
        return comp;
    }

    static Component* create_hstack(Component *parent) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        Component *comp = new Component(box);
        comp->is_container = true;
        return comp;
    }

    static Component* create_scrollview(Component *parent) {
        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        Component *comp = new Component(scroll);
        comp->is_container = true;
        return comp;
    }

    static Component* create_switch(Component *parent) {
        return new Component(gtk_switch_new());
    }

    static Component* create_separator(Component *parent) {
        return new Component(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    }

    static void on_button_clicked(GtkButton *button, gpointer data) {
        Component *comp = static_cast<Component*>(data);
        auto it = comp->callbacks.find(ALLOY_EVENT_CLICK);
        if (it != comp->callbacks.end()) {
            it->second.first(comp, it->second.second);
        }
    }

    static void on_changed(GtkWidget *widget, gpointer data) {
        Component *comp = static_cast<Component*>(data);
        auto it = comp->callbacks.find(ALLOY_EVENT_CHANGE);
        if (it != comp->callbacks.end()) {
            it->second.first(comp, it->second.second);
        }
    }
};

} // namespace detail
} // namespace alloy

#endif // ALLOY_GUI_GTK_BACKEND_HH
