#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_dialog(alloy_component_t parent, const char* title) {
    auto* d = new gtk_window(title, 400, 300);
    if (parent) static_cast<component_base*>(parent)->add_child(d);
    return d;
}

ALLOY_API alloy_component_t alloy_create_popover(alloy_component_t parent) {
    auto* p = new gtk_container(gtk_popover_new(NULL));
    if (parent) static_cast<component_base*>(parent)->add_child(p);
    return p;
}

ALLOY_API alloy_component_t alloy_create_file_dialog(alloy_component_t parent, int save) {
    auto* fd = gtk_file_chooser_dialog_new("Select File", NULL,
        save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        save ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
        NULL);
    auto* comp = new gtk_component(fd);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}
#else
ALLOY_API alloy_component_t alloy_create_dialog(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_popover(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_file_dialog(alloy_component_t, int) { return nullptr; }
#endif
}
