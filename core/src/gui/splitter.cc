#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_splitter(alloy_component_t parent, int vertical) {
    auto* p = new gtk_container(gtk_paned_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL));
    if (parent) static_cast<component_base*>(parent)->add_child(p);
    return p;
}
#else
ALLOY_API alloy_component_t alloy_create_splitter(alloy_component_t, int) { return nullptr; }
#endif
}
