#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_button(alloy_component_t parent) {
    Component* p = (Component*)parent;
    Component* comp = GTKBackend::create_button(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
}

}
