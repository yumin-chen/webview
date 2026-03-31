#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_vstack(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_vstack(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_hstack(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_hstack(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_scrollview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_splitter(alloy_component_t parent) {
    Component* p = (Component*)parent;
    GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    Component* comp = new Component(paned);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
    Component *parent = (Component*)container;
    Component *comp = (Component*)child;
    if (parent && parent->widget && comp && comp->widget && GTK_IS_CONTAINER(parent->widget)) {
        gtk_container_add(GTK_CONTAINER(parent->widget), comp->widget);
        parent->children.push_back(comp);
        return ALLOY_OK;
    }
    return ALLOY_ERROR_INVALID_ARGUMENT;
}

alloy_error_t alloy_layout(alloy_component_t window) {
    Component *comp = (Component*)window;
    if (comp && comp->widget) {
        gtk_widget_show_all(comp->widget);
    }
    return ALLOY_OK;
}

}
