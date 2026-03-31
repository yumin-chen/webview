#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_textfield(alloy_component_t parent) {
    auto* entry = new gtk_component(gtk_entry_new());
    if (parent) static_cast<component_base*>(parent)->add_child(entry);
    return entry;
}

ALLOY_API alloy_component_t alloy_create_textarea(alloy_component_t parent) {
    auto* tv = gtk_text_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}
#else
ALLOY_API alloy_component_t alloy_create_textfield(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_textarea(alloy_component_t) { return nullptr; }
#endif
}
