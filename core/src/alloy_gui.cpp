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

}
