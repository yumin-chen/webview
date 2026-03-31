#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

namespace alloy {
namespace detail {

void Component::on_signal_changed(alloy_prop_id_t prop, const signal_value& val) {
    if (!widget) return;
    if (prop == ALLOY_PROP_TEXT) {
        if (GTK_IS_WINDOW(widget)) gtk_window_set_title(GTK_WINDOW(widget), val.s.c_str());
        else if (GTK_IS_BUTTON(widget)) gtk_button_set_label(GTK_BUTTON(widget), val.s.c_str());
        else if (GTK_IS_LABEL(widget)) gtk_label_set_text(GTK_LABEL(widget), val.s.c_str());
    }
}

} // namespace detail
} // namespace alloy

extern "C" {

static bool is_gtk_init() {
    static bool initialized = false;
    static bool success = false;
    if (!initialized) {
        success = gtk_init_check(NULL, NULL);
        initialized = true;
    }
    return success;
}

alloy_component_t alloy_create_window(const char *title, int width, int height) {
    if (is_gtk_init()) {
        GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(w), title);
        gtk_window_set_default_size(GTK_WINDOW(w), width, height);
        return (alloy_component_t)new Component(w);
    }
    return (alloy_component_t)new Component(nullptr);
}

alloy_error_t alloy_destroy(alloy_component_t handle) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete (Component*)handle;
    return ALLOY_OK;
}

alloy_error_t alloy_set_text(alloy_component_t handle, const char *text) {
    Component *comp = (Component*)handle;
    if (!comp || !comp->widget) return ALLOY_OK;
    if (GTK_IS_WINDOW(comp->widget)) gtk_window_set_title(GTK_WINDOW(comp->widget), text);
    else if (GTK_IS_BUTTON(comp->widget)) gtk_button_set_label(GTK_BUTTON(comp->widget), text);
    else if (GTK_IS_LABEL(comp->widget)) gtk_label_set_text(GTK_LABEL(comp->widget), text);
    return ALLOY_OK;
}

alloy_error_t alloy_run(alloy_component_t /*window*/) {
    if (is_gtk_init()) gtk_main();
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t /*window*/) {
    if (is_gtk_init()) gtk_main_quit();
    return ALLOY_OK;
}

}
