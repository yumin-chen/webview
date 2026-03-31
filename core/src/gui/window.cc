#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_window(const char *title, int width, int height) {
    return new gtk_window(title, width, height);
}
#else
ALLOY_API alloy_component_t alloy_create_window(const char *, int, int) { return nullptr; }
#endif
}
