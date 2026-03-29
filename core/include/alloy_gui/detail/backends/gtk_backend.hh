#ifndef ALLOY_GUI_GTK_BACKEND_HH
#define ALLOY_GUI_GTK_BACKEND_HH

#include <gtk/gtk.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../alloy_gui/api.h"

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

    static Component* create_vstack(Component *parent) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        Component *comp = new Component(box);
        comp->is_container = true;
        return comp;
    }

    static void on_button_clicked(GtkButton *button, gpointer data) {
        Component *comp = static_cast<Component*>(data);
        auto it = comp->callbacks.find(ALLOY_EVENT_CLICK);
        if (it != comp->callbacks.end()) {
            it->second.first(comp, it->second.second);
        }
    }
};

} // namespace detail
} // namespace alloy

#endif // ALLOY_GUI_GTK_BACKEND_HH
