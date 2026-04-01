#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#include "alloy/detail/backends.hh"
#include <string.h>
#include <vector>
#include <algorithm>

namespace alloy::detail {
struct property_binding {
    alloy_component_t component;
    alloy_prop_id_t property;
    alloy_signal_t signal;
    alloy_effect_t effect;
};
static std::vector<property_binding> g_bindings;

void sync_property(void* userdata) {
    auto* b = static_cast<property_binding*>(userdata);
    auto* comp = static_cast<alloy::detail::component_base*>(b->component);
    switch (b->property) {
        case ALLOY_PROP_TEXT:
        case ALLOY_PROP_LABEL:
            comp->set_text(alloy_signal_get_str(b->signal));
            break;
        case ALLOY_PROP_CHECKED:
            comp->set_checked(alloy_signal_get_bool(b->signal) != 0);
            break;
        case ALLOY_PROP_VALUE:
            comp->set_value(alloy_signal_get_double(b->signal));
            break;
        case ALLOY_PROP_ENABLED:
            comp->set_enabled(alloy_signal_get_bool(b->signal) != 0);
            break;
        case ALLOY_PROP_VISIBLE:
            comp->set_visible(alloy_signal_get_bool(b->signal) != 0);
            break;
    }
}
}

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

alloy_error_t alloy_bind_property(alloy_component_t component,
                                            alloy_prop_id_t property,
                                            alloy_signal_t signal) {
    if (!component || !signal) return ALLOY_ERROR_INVALID_ARGUMENT;
    alloy::detail::property_binding b;
    b.component = component;
    b.property = property;
    b.signal = signal;
    alloy::detail::g_bindings.push_back(b);
    auto& ref = alloy::detail::g_bindings.back();
    ref.effect = alloy_effect_create(&ref.signal, 1, alloy::detail::sync_property, &ref);
    return ALLOY_OK;
}

alloy_error_t alloy_unbind_property(alloy_component_t component,
                                              alloy_prop_id_t property) {
    auto it = std::find_if(alloy::detail::g_bindings.begin(), alloy::detail::g_bindings.end(), [&](const alloy::detail::property_binding& b) {
        return b.component == component && b.property == property;
    });
    if (it != alloy::detail::g_bindings.end()) {
        alloy_effect_destroy(it->effect);
        alloy::detail::g_bindings.erase(it);
        return ALLOY_OK;
    }
    return ALLOY_ERROR_INVALID_ARGUMENT;
}

alloy_error_t alloy_webview_bind_global(alloy_component_t webview,
                                                   const char *name,
                                                   void (*callback)(const char *json_args, void *userdata),
                                                   void *userdata) {
    if (!webview || !name || !callback) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(webview)->bind_global(name, callback, userdata);
}

alloy_error_t alloy_webview_secure_post(alloy_component_t webview,
                                                   const char *encrypted_msg) {
    if (!webview || !encrypted_msg) return ALLOY_ERROR_INVALID_ARGUMENT;
    return static_cast<alloy::detail::component_base*>(webview)->secure_post(encrypted_msg);
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

void perform_recursive_layout(alloy::detail::component_base* comp, float x, float y, float w, float h) {
    if (!comp) return;

    // Set native frame (platform specific)
#if defined(_WIN32)
    MoveWindow((HWND)comp->native_handle(), (int)x, (int)y, (int)w, (int)h, TRUE);
#endif

    if (comp->is_container()) {
        // Implementation of basic layout for VStack/HStack
        // For now, simple stacking logic
        float child_y = 0;
        // This would iterate children and apply layout
    }
}

alloy_error_t alloy_layout(alloy_component_t window) {
    if (!window) return ALLOY_ERROR_INVALID_ARGUMENT;
    auto w = static_cast<alloy::detail::component_base*>(window);
    perform_recursive_layout(w, 0, 0, 800, 600); // Root layout
    return ALLOY_OK;
}

alloy_error_t alloy_run(alloy_component_t window) {
    if (!window) return ALLOY_ERROR_INVALID_ARGUMENT;
#if defined(_WIN32)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#elif defined(__APPLE__)
    // Cocoa run loop handled by NSApplication
#else
    gtk_main();
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t window) {
    if (!window) return ALLOY_ERROR_INVALID_ARGUMENT;
#if defined(_WIN32)
    PostQuitMessage(0);
#elif defined(__APPLE__)
    // Terminate Cocoa app
#else
    gtk_main_quit();
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_dispatch(alloy_component_t window, void (*fn)(void *arg), void *arg) {
    if (!fn) return ALLOY_ERROR_INVALID_ARGUMENT;
#if defined(_WIN32)
    // Post custom message to window thread
    SendMessage((HWND)static_cast<alloy::detail::component_base*>(window)->native_handle(), WM_APP + 1, (WPARAM)fn, (LPARAM)arg);
#else
    fn(arg);
#endif
    return ALLOY_OK;
}

}
