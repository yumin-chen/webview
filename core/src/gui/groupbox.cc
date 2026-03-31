#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_groupbox(alloy_component_t parent, const char* label) {
    auto* f = new gtk_container(gtk_frame_new(label));
    if (parent) static_cast<component_base*>(parent)->add_child(f);
    return f;
}

ALLOY_API alloy_component_t alloy_create_card(alloy_component_t parent) {
    auto* f = new gtk_container(gtk_frame_new(NULL));
    if (parent) static_cast<component_base*>(parent)->add_child(f);
    return f;
}

ALLOY_API alloy_component_t alloy_create_accordion(alloy_component_t parent) {
    auto* e = new gtk_container(gtk_expander_new(""));
    if (parent) static_cast<component_base*>(parent)->add_child(e);
    return e;
}
#else
ALLOY_API alloy_component_t alloy_create_groupbox(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_card(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_accordion(alloy_component_t) { return nullptr; }
#endif
}
