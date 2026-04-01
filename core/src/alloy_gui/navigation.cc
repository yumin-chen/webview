#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include "alloy_gui/detail/backends/gtk_backend.hh"
#endif

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_tabview(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_tabview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_listview(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_listview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_treeview(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_treeview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_webview(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_webview(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

}
