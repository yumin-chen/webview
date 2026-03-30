#include "alloy/api.h"
#include "alloy/detail/backends/gtk_gui.hh"
#include <string>
#include <vector>

using namespace alloy::detail;

extern "C" {

const char* alloy_error_message(alloy_error_t err) {
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

alloy_component_t alloy_create_window(const char *title, int width, int height) {
    // Stub implementation
    return nullptr;
}

alloy_component_t alloy_create_button(alloy_component_t parent) {
#ifdef WEBVIEW_PLATFORM_LINUX
    return static_cast<alloy_component_t>(new gtk_button(static_cast<component_base*>(parent)));
#else
    return nullptr;
#endif
}

alloy_error_t alloy_set_text(alloy_component_t h, const char *text) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<component_base*>(h)->set_text(text);
}

alloy_error_t alloy_destroy(alloy_component_t handle) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete static_cast<component_base*>(handle);
    return ALLOY_OK;
}

// Stubs for other API functions to allow compilation
alloy_signal_t  alloy_signal_create_str(const char *initial) { return nullptr; }
alloy_error_t   alloy_signal_set_str(alloy_signal_t s, const char *v) { return ALLOY_OK; }
alloy_error_t   alloy_bind_property(alloy_component_t component, alloy_prop_id_t property, alloy_signal_t signal) { return ALLOY_OK; }
alloy_error_t   alloy_run(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t   alloy_terminate(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t   alloy_dispatch(alloy_component_t window, void (*fn)(void *arg), void *arg) { return ALLOY_OK; }
alloy_component_t alloy_create_textfield(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_textarea(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_label(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_checkbox(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_radiobutton(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_combobox(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_slider(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_progressbar(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_tabview(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_listview(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_treeview(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_webview(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_vstack(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_hstack(alloy_component_t parent) { return nullptr; }
alloy_component_t alloy_create_scrollview(alloy_component_t parent) { return nullptr; }
alloy_error_t alloy_get_text(alloy_component_t h, char *buf, size_t buf_len) { return ALLOY_OK; }
alloy_error_t alloy_set_checked(alloy_component_t h, int checked) { return ALLOY_OK; }
int           alloy_get_checked(alloy_component_t h) { return 0; }
alloy_error_t alloy_set_value(alloy_component_t h, double value) { return ALLOY_OK; }
double        alloy_get_value(alloy_component_t h) { return 0; }
alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled) { return ALLOY_OK; }
int           alloy_get_enabled(alloy_component_t h) { return 0; }
alloy_error_t alloy_set_visible(alloy_component_t h, int visible) { return ALLOY_OK; }
int           alloy_get_visible(alloy_component_t h) { return 0; }
alloy_error_t alloy_set_style(alloy_component_t h, const alloy_style_t *style) { return ALLOY_OK; }
alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) { return ALLOY_OK; }
alloy_error_t alloy_set_flex(alloy_component_t h, float flex) { return ALLOY_OK; }
alloy_error_t alloy_set_padding(alloy_component_t h, float top, float right, float bottom, float left) { return ALLOY_OK; }
alloy_error_t alloy_set_margin(alloy_component_t h, float top, float right, float bottom, float left) { return ALLOY_OK; }
alloy_error_t alloy_layout(alloy_component_t window) { return ALLOY_OK; }
alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) { return ALLOY_OK; }
alloy_signal_t  alloy_signal_create_double(double initial) { return nullptr; }
alloy_signal_t  alloy_signal_create_int(int initial) { return nullptr; }
alloy_signal_t  alloy_signal_create_bool(int initial) { return nullptr; }
alloy_error_t   alloy_signal_set_double(alloy_signal_t s, double v) { return ALLOY_OK; }
alloy_error_t   alloy_signal_set_int(alloy_signal_t s, int v) { return ALLOY_OK; }
alloy_error_t   alloy_signal_set_bool(alloy_signal_t s, int v) { return ALLOY_OK; }
const char     *alloy_signal_get_str(alloy_signal_t s) { return nullptr; }
double          alloy_signal_get_double(alloy_signal_t s) { return 0; }
int             alloy_signal_get_int(alloy_signal_t s) { return 0; }
int             alloy_signal_get_bool(alloy_signal_t s) { return 0; }
alloy_computed_t alloy_computed_create(alloy_signal_t *deps, size_t dep_count, void (*compute)(alloy_signal_t *deps, size_t dep_count, void *out, void *userdata), void *userdata) { return nullptr; }
alloy_effect_t  alloy_effect_create(alloy_signal_t *deps, size_t dep_count, void (*run)(void *userdata), void *userdata) { return nullptr; }
alloy_error_t   alloy_signal_destroy(alloy_signal_t s) { return ALLOY_OK; }
alloy_error_t   alloy_computed_destroy(alloy_computed_t c) { return ALLOY_OK; }
alloy_error_t   alloy_effect_destroy(alloy_effect_t e) { return ALLOY_OK; }
alloy_error_t   alloy_unbind_property(alloy_component_t component, alloy_prop_id_t property) { return ALLOY_OK; }

}
