#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
#endif

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_image(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *img = gtk_image_new();
    Component* comp = new Component(img);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_icon(alloy_component_t parent) {
    return alloy_create_image(parent);
}

alloy_component_t alloy_create_menubar(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *bar = gtk_menu_bar_new();
    Component* comp = new Component(bar);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_menu(alloy_component_t /*parent*/) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return (alloy_component_t)new Component(gtk_menu_new());
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_contextmenu(alloy_component_t /*parent*/) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return (alloy_component_t)new Component(gtk_menu_new());
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_toolbar(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *bar = gtk_toolbar_new();
    Component* comp = new Component(bar);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_statusbar(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *bar = gtk_statusbar_new();
    Component* comp = new Component(bar);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_separator(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    Component* comp = new Component(sep);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_divider(alloy_component_t parent) {
    return alloy_create_separator(parent);
}

alloy_component_t alloy_create_groupbox(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *frame = gtk_frame_new(NULL);
    Component* comp = new Component(frame);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_accordion(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *exp = gtk_expander_new(NULL);
    Component* comp = new Component(exp);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_popover(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *pop = gtk_popover_new(p ? p->widget : NULL);
    return (alloy_component_t)new Component(pop);
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_badge(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *lbl = gtk_label_new(NULL);
    Component* comp = new Component(lbl);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_chip(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *btn = gtk_button_new();
    Component* comp = new Component(btn);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_loading_indicator(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *spin = gtk_spinner_new();
    Component* comp = new Component(spin);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_card(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    Component* comp = new Component(box);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_link(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *btn = gtk_link_button_new("");
    Component* comp = new Component(btn);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_rating(alloy_component_t parent) {
    return alloy_create_card(parent);
}

alloy_component_t alloy_create_richtexteditor(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scroll), view);
    Component* comp = new Component(scroll);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_codeeditor(alloy_component_t parent) {
    return alloy_create_richtexteditor(parent);
}

alloy_component_t alloy_create_tooltip(alloy_component_t /*parent*/) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return (alloy_component_t)new Component(gtk_label_new(NULL));
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) {
    Component *comp = (Component*)handle;
    if (!comp) return ALLOY_ERROR_INVALID_ARGUMENT;
    comp->callbacks[event] = {callback, userdata};
    return ALLOY_OK;
}

}
