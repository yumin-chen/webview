#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_label(alloy_component_t parent, const char* text) {
    auto* lbl = new gtk_component(gtk_label_new(text));
    if (parent) static_cast<component_base*>(parent)->add_child(lbl);
    return lbl;
}
#else
ALLOY_API alloy_component_t alloy_create_label(alloy_component_t, const char*) { return nullptr; }
#endif
}
