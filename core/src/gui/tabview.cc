#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_tabview(alloy_component_t parent) {
    auto* nb = new gtk_container(gtk_notebook_new());
    if (parent) static_cast<component_base*>(parent)->add_child(nb);
    return nb;
}
#else
ALLOY_API alloy_component_t alloy_create_tabview(alloy_component_t) { return nullptr; }
#endif
}
