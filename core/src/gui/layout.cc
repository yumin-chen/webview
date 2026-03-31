#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_vstack(alloy_component_t parent) {
    auto* box = new gtk_container(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    if (parent) static_cast<component_base*>(parent)->add_child(box);
    return box;
}

ALLOY_API alloy_component_t alloy_create_hstack(alloy_component_t parent) {
    auto* box = new gtk_container(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    if (parent) static_cast<component_base*>(parent)->add_child(box);
    return box;
}

ALLOY_API alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
    auto* sw = new gtk_container(gtk_scrolled_window_new(NULL, NULL));
    if (parent) static_cast<component_base*>(parent)->add_child(sw);
    return sw;
}
#else
ALLOY_API alloy_component_t alloy_create_vstack(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_hstack(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_scrollview(alloy_component_t) { return nullptr; }
#endif
}
