#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_combobox(alloy_component_t parent) {
    auto* combo = new gtk_component(gtk_combo_box_text_new());
    if (parent) static_cast<component_base*>(parent)->add_child(combo);
    return combo;
}

ALLOY_API alloy_component_t alloy_create_slider(alloy_component_t parent) {
    auto* slider = new gtk_component(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0));
    if (parent) static_cast<component_base*>(parent)->add_child(slider);
    return slider;
}

ALLOY_API alloy_component_t alloy_create_progressbar(alloy_component_t parent) {
    auto* pb = new gtk_component(gtk_progress_bar_new());
    if (parent) static_cast<component_base*>(parent)->add_child(pb);
    return pb;
}
#else
ALLOY_API alloy_component_t alloy_create_combobox(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_slider(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_progressbar(alloy_component_t) { return nullptr; }
#endif
}
