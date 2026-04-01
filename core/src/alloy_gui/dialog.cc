#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
#endif

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_dialog(const char *title, int width, int height) {
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, NULL, GTK_DIALOG_MODAL, "OK", GTK_RESPONSE_OK, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), width, height);
    return (alloy_component_t)new Component(dialog);
#else
    (void)title; (void)width; (void)height;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_filedialog(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *dialog = gtk_file_chooser_button_new("Select File", GTK_FILE_CHOOSER_ACTION_OPEN);
    Component* comp = new Component(dialog);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_colorpicker(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *picker = gtk_color_button_new();
    Component* comp = new Component(picker);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_datepicker(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *calendar = gtk_calendar_new();
    Component* comp = new Component(calendar);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    (void)p;
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_timepicker(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *hour = gtk_spin_button_new_with_range(0, 23, 1);
    GtkWidget *min = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_container_add(GTK_CONTAINER(box), hour);
    gtk_container_add(GTK_CONTAINER(box), min);
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

}
