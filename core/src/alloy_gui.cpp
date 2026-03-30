#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#include "alloy/detail/backends.hh"
#include <string.h>

extern "C" {

const char *alloy_error_message(alloy_error_t err) {
    switch (err) {
        case ALLOY_OK: return "Success";
        case ALLOY_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case ALLOY_ERROR_INVALID_STATE: return "Invalid state";
        case ALLOY_ERROR_PLATFORM: return "Platform error";
        case ALLOY_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ALLOY_ERROR_NOT_SUPPORTED: return "Not supported";
        default: return "Unknown error";
    }
}

alloy_error_t alloy_set_text(alloy_component_t h, const char *text) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(h)->set_text(text);
}

alloy_error_t alloy_get_text(alloy_component_t h, char *buf, size_t buf_len) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(h)->get_text(buf, buf_len);
}

alloy_error_t alloy_set_checked(alloy_component_t h, int checked) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(h)->set_checked(checked != 0);
}

int alloy_get_checked(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<alloy::detail::component_base*>(h)->get_checked() ? 1 : 0;
}

alloy_error_t alloy_set_value(alloy_component_t h, double value) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    if (value < 0.0 || value > 1.0) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(h)->set_value(value);
}

double alloy_get_value(alloy_component_t h) {
    if (!h) return 0.0;
    return static_cast<alloy::detail::component_base*>(h)->get_value();
}

alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(h)->set_enabled(enabled != 0);
}

int alloy_get_enabled(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<alloy::detail::component_base*>(h)->get_enabled() ? 1 : 0;
}

alloy_error_t alloy_set_visible(alloy_component_t h, int visible) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(h)->set_visible(visible != 0);
}

int alloy_get_visible(alloy_component_t h) {
    if (!h) return 0;
    return static_cast<alloy::detail::component_base*>(h)->get_visible() ? 1 : 0;
}

alloy_error_t alloy_destroy(alloy_component_t h) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete static_cast<alloy::detail::component_base*>(h);
    return ALLOY_OK;
}

alloy_error_t alloy_set_event_callback(alloy_component_t handle,
                                       alloy_event_type_t event,
                                       alloy_event_cb_t callback,
                                       void *userdata) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    static_cast<alloy::detail::component_base*>(handle)->set_event_callback(event, callback, userdata);
    return ALLOY_OK;
}

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
    if (!container || !child) return ALLOY_ERROR_INVALID_ARGUMENT;
    auto c = static_cast<alloy::detail::component_base*>(container);
    if (!c->is_container()) return ALLOY_ERROR_INVALID_ARGUMENT;

    // Platform-specific child addition
#if defined(_WIN32)
    HWND child_hwnd = (HWND)static_cast<alloy::detail::component_base*>(child)->native_handle();
    SetParent(child_hwnd, (HWND)c->native_handle());
#elif defined(__APPLE__)
    id child_view = (id)static_cast<alloy::detail::component_base*>(child)->native_handle();
    ((void (*)(id, SEL, id))objc_msgSend)((id)c->native_handle(), sel_registerName("addSubview:"), child_view);
#else
    // GTK addition logic
    // gtk_container_add(GTK_CONTAINER(c->native_handle()), (GtkWidget*)static_cast<alloy::detail::component_base*>(child)->native_handle());
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_set_flex(alloy_component_t h, float flex) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    static_cast<alloy::detail::component_base*>(h)->layout().flex = flex;
    return ALLOY_OK;
}

alloy_error_t alloy_set_padding(alloy_component_t h, float t, float r, float b, float l) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    auto& lp = static_cast<alloy::detail::component_base*>(h)->layout();
    lp.padding[0] = t; lp.padding[1] = r; lp.padding[2] = b; lp.padding[3] = l;
    return ALLOY_OK;
}

alloy_error_t alloy_set_margin(alloy_component_t h, float t, float r, float b, float l) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    auto& lp = static_cast<alloy::detail::component_base*>(h)->layout();
    lp.margin[0] = t; lp.margin[1] = r; lp.margin[2] = b; lp.margin[3] = l;
    return ALLOY_OK;
}

alloy_error_t alloy_set_width(alloy_component_t h, float width) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    static_cast<alloy::detail::component_base*>(h)->layout().width = width;
    return ALLOY_OK;
}

alloy_error_t alloy_set_height(alloy_component_t h, float height) {
    if (!h) return ALLOY_ERROR_INVALID_ARGUMENT;
    static_cast<alloy::detail::component_base*>(h)->layout().height = height;
    return ALLOY_OK;
}

alloy_error_t alloy_layout(alloy_component_t window) {
    if (!window) return ALLOY_ERROR_INVALID_ARGUMENT;
    // Yoga layout calculation entry point
    return ALLOY_OK;
}

}
