#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_menubar(alloy_component_t parent) {
    auto* mb = new gtk_container(gtk_menu_bar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(mb);
    return mb;
}

ALLOY_API alloy_component_t alloy_create_menu(alloy_component_t parent, const char* label) {
    auto* m = new gtk_container(gtk_menu_new());
    if (parent) static_cast<component_base*>(parent)->add_child(m);
    return m;
}

ALLOY_API alloy_component_t alloy_create_menuitem(alloy_component_t parent, const char* label) {
    auto* mi = new gtk_component(gtk_menu_item_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(mi);
    return mi;
}

ALLOY_API alloy_component_t alloy_create_contextmenu(alloy_component_t parent) {
    auto* m = new gtk_container(gtk_menu_new());
    if (parent) static_cast<component_base*>(parent)->add_child(m);
    return m;
}
#else
ALLOY_API alloy_component_t alloy_create_menubar(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_menu(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_menuitem(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_contextmenu(alloy_component_t) { return nullptr; }
#endif
}
