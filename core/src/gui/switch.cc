#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_switch_new());
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}
#else
ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t) { return nullptr; }
#endif
}
