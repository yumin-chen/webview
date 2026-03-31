#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_toolbar(alloy_component_t parent) {
    auto* tb = new gtk_container(gtk_toolbar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(tb);
    return tb;
}
#else
ALLOY_API alloy_component_t alloy_create_toolbar(alloy_component_t) { return nullptr; }
#endif
}
