#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_checkbox(alloy_component_t parent, const char* label) {
    auto* cb = new gtk_component(gtk_check_button_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(cb);
    return cb;
}

ALLOY_API alloy_component_t alloy_create_radiobutton(alloy_component_t parent, const char* label, const char* name, const char* value) {
    auto* rb = new gtk_component(gtk_radio_button_new_with_label(NULL, label));
    if (parent) static_cast<component_base*>(parent)->add_child(rb);
    return rb;
}

ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_switch_new());
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}
#else
ALLOY_API alloy_component_t alloy_create_checkbox(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_radiobutton(alloy_component_t, const char*, const char*, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t) { return nullptr; }
#endif
}
