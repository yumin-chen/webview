#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_checkbox(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_checkbox(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_radiobutton(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_radiobutton(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_combobox(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_combobox(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_slider(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_slider(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_spinner(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_spinner(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_progressbar(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_progressbar(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

alloy_component_t alloy_create_switch(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_switch(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

}
