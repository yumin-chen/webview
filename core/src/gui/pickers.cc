#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_datepicker(alloy_component_t parent) {
    auto* c = new gtk_component(gtk_calendar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(c);
    return c;
}

ALLOY_API alloy_component_t alloy_create_colorpicker(alloy_component_t parent) {
    auto* cb = new gtk_component(gtk_color_button_new());
    if (parent) static_cast<component_base*>(parent)->add_child(cb);
    return cb;
}

ALLOY_API alloy_component_t alloy_create_timepicker(alloy_component_t parent) {
    auto* b = new gtk_container(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2));
    gtk_container_add(GTK_CONTAINER(static_cast<GtkWidget*>(b->native_handle())), gtk_spin_button_new_with_range(0, 23, 1));
    gtk_container_add(GTK_CONTAINER(static_cast<GtkWidget*>(b->native_handle())), gtk_spin_button_new_with_range(0, 59, 1));
    if (parent) static_cast<component_base*>(parent)->add_child(b);
    return b;
}
#else
ALLOY_API alloy_component_t alloy_create_datepicker(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_colorpicker(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_timepicker(alloy_component_t) { return nullptr; }
#endif
}
