#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_image(alloy_component_t parent, const char* src) {
    auto* img = new gtk_component(gtk_image_new_from_file(src));
    if (parent) static_cast<component_base*>(parent)->add_child(img);
    return img;
}

ALLOY_API alloy_component_t alloy_create_icon(alloy_component_t parent, const char* name) {
    auto* i = new gtk_component(gtk_image_new_from_icon_name(name, GTK_ICON_SIZE_BUTTON));
    if (parent) static_cast<component_base*>(parent)->add_child(i);
    return i;
}
#else
ALLOY_API alloy_component_t alloy_create_image(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_icon(alloy_component_t, const char*) { return nullptr; }
#endif
}
