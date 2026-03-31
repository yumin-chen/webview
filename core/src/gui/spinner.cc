#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_spinner(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_spin_button_new_with_range(0, 100, 1));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_loading_spinner(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_spinner_new());
    gtk_spinner_start(GTK_SPINNER(static_cast<GtkWidget*>(s->native_handle())));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}
#else
ALLOY_API alloy_component_t alloy_create_spinner(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_loading_spinner(alloy_component_t) { return nullptr; }
#endif
}
