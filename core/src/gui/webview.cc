#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_webview(alloy_component_t parent) {
    auto* wv = webkit_web_view_new();
    auto* comp = new gtk_component(wv);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}
#else
ALLOY_API alloy_component_t alloy_create_webview(alloy_component_t) { return nullptr; }
#endif
}
