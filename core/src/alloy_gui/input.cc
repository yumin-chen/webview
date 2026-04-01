#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include "alloy_gui/detail/backends/gtk_backend.hh"
#endif

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_textfield(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_textfield(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

alloy_component_t alloy_create_textarea(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_textarea(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

}
