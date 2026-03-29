#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_window(const char *title, int width, int height) {
    return (alloy_component_t)GTKBackend::create_window(title, width, height);
}

alloy_component_t alloy_create_button(alloy_component_t parent) {
    return (alloy_component_t)GTKBackend::create_button((Component*)parent);
}

alloy_component_t alloy_create_vstack(alloy_component_t parent) {
    return (alloy_component_t)GTKBackend::create_vstack((Component*)parent);
}

alloy_error_t alloy_destroy(alloy_component_t handle) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete (Component*)handle;
    return ALLOY_OK;
}

alloy_error_t alloy_set_text(alloy_component_t handle, const char *text) {
    Component *comp = (Component*)handle;
    if (GTK_IS_WINDOW(comp->widget)) {
        gtk_window_set_title(GTK_WINDOW(comp->widget), text);
    } else if (GTK_IS_BUTTON(comp->widget)) {
        gtk_button_set_label(GTK_BUTTON(comp->widget), text);
    }
    return ALLOY_OK;
}

alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) {
    Component *comp = (Component*)handle;
    comp->callbacks[event] = {callback, userdata};
    return ALLOY_OK;
}

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
    Component *parent = (Component*)container;
    Component *comp = (Component*)child;
    if (!parent->is_container) return ALLOY_ERROR_INVALID_ARGUMENT;
    gtk_container_add(GTK_CONTAINER(parent->widget), comp->widget);
    parent->children.push_back(comp);
    return ALLOY_OK;
}

alloy_error_t alloy_layout(alloy_component_t window) {
    Component *comp = (Component*)window;
    gtk_widget_show_all(comp->widget);
    return ALLOY_OK;
}

alloy_error_t alloy_run(alloy_component_t window) {
    gtk_main();
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t window) {
    gtk_main_quit();
    return ALLOY_OK;
}

const char* alloy_error_message(alloy_error_t err) {
    switch(err) {
        case ALLOY_OK: return "Success";
        case ALLOY_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case ALLOY_ERROR_INVALID_STATE: return "Invalid state";
        case ALLOY_ERROR_PLATFORM: return "Platform error";
        case ALLOY_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ALLOY_ERROR_NOT_SUPPORTED: return "Not supported";
        default: return "Unknown error";
    }
}

}
