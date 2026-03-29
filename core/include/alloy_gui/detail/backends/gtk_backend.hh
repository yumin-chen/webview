#ifndef ALLOY_GUI_GTK_BACKEND_HH
#define ALLOY_GUI_GTK_BACKEND_HH

#include <gtk/gtk.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "../../alloy_gui/api.h"

namespace alloy {
namespace detail {

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
};

class GTKBackend {
public:
    static Component* create_window(const char *title, int width, int height) {
        GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), title);
        gtk_window_set_default_size(GTK_WINDOW(window), width, height);
        g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), nullptr);
        return new Component(window);
    }

    static Component* create_button(Component *parent) {
        GtkWidget *button = gtk_button_new();
        if (parent && parent->widget) {
            gtk_container_add(GTK_CONTAINER(parent->widget), button);
        }
        Component *comp = new Component(button);
        g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), comp);
        return comp;
    }

    static Component* create_vstack(Component *parent) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        if (parent && parent->widget) {
            gtk_container_add(GTK_CONTAINER(parent->widget), box);
        }
        Component *comp = new Component(box);
        comp->is_container = true;
        return comp;
    }

    static void on_window_destroy(GtkWidget *widget, gpointer data) {
        // Handle window close event if registered
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
