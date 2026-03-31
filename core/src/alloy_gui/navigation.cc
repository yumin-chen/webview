#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_tabview(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_tabview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_listview(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_listview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_treeview(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_treeview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_webview(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_webview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

}
