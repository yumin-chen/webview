#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include "alloy_gui/detail/backends/gtk_backend.hh"
#endif

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_vstack(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_vstack(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_hstack(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_hstack(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_scrollview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_splitter(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    Component* comp = new Component(paned);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
    Component *parent = (Component*)container;
    Component *comp = (Component*)child;
    if (!parent || !comp) return ALLOY_ERROR_INVALID_ARGUMENT;
#ifdef WEBVIEW_PLATFORM_LINUX
    if (parent->widget && comp->widget && GTK_IS_CONTAINER(parent->widget)) {
        gtk_container_add(GTK_CONTAINER(parent->widget), comp->widget);
        parent->children.push_back(comp);
        return ALLOY_OK;
    }
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_set_flex(alloy_component_t h, float flex) {
    if (h) ((Component*)h)->flex = flex;
    return ALLOY_OK;
}

alloy_error_t alloy_set_padding(alloy_component_t, float, float, float, float) { return ALLOY_OK; }
alloy_error_t alloy_set_margin(alloy_component_t, float, float, float, float) { return ALLOY_OK; }

}
