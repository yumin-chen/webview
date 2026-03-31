#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_error_t alloy_destroy(alloy_component_t handle) {
    delete static_cast<component_base*>(handle);
    return ALLOY_OK;
}

ALLOY_API alloy_error_t alloy_set_text(alloy_component_t h, const char *text) {
    return static_cast<component_base*>(h)->set_text(text);
}

ALLOY_API alloy_error_t alloy_layout(alloy_component_t window) {
    auto* win = static_cast<component_base*>(window);
    int w, h;
    GtkWidget* widget = static_cast<GtkWidget*>(win->native_handle());
    gtk_window_get_size(GTK_WINDOW(widget), &w, &h);

    YGNodeCalculateLayout(win->yoga_node(), (float)w, (float)h, YGDirectionLTR);
    win->apply_layout();
    return ALLOY_OK;
}

ALLOY_API alloy_error_t alloy_run(alloy_component_t window) {
    gtk_main();
    return ALLOY_OK;
}

ALLOY_API const char *alloy_error_message(alloy_error_t err) {
    switch (err) {
        case ALLOY_OK: return "OK";
        case ALLOY_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case ALLOY_ERROR_INVALID_STATE: return "Invalid state";
        case ALLOY_ERROR_PLATFORM: return "Platform error";
        case ALLOY_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ALLOY_ERROR_NOT_SUPPORTED: return "Not supported";
        default: return "Unknown error";
    }
}
#else
ALLOY_API alloy_error_t alloy_destroy(alloy_component_t) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_set_text(alloy_component_t, const char *) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_layout(alloy_component_t) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_run(alloy_component_t) { return ALLOY_OK; }
ALLOY_API const char *alloy_error_message(alloy_error_t) { return "Unknown error"; }
#endif
}
