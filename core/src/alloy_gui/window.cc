#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include "alloy_gui/detail/backends/gtk_backend.hh"
#elif defined(WEBVIEW_PLATFORM_DARWIN)
#include "alloy_gui/detail/backends/cocoa_backend.hh"
#include "webview/detail/platform/darwin/cocoa/NSApplication.hh"
#endif

#include "webview.h"
#include <cstring>

using namespace alloy::detail;

namespace alloy {
namespace detail {

void Component::on_signal_changed(alloy_prop_id_t prop, const signal_value& val) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (!widget) return;
    GtkWidget* w = (GtkWidget*)widget;
    if (prop == ALLOY_PROP_TEXT) {
        if (GTK_IS_WINDOW(w)) gtk_window_set_title(GTK_WINDOW(w), val.s.c_str());
        else if (GTK_IS_BUTTON(w)) gtk_button_set_label(GTK_BUTTON(w), val.s.c_str());
        else if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), val.s.c_str());
        else if (GTK_IS_ENTRY(w)) gtk_entry_set_text(GTK_ENTRY(w), val.s.c_str());
    } else if (prop == ALLOY_PROP_ENABLED) {
        gtk_widget_set_sensitive(w, val.b);
    } else if (prop == ALLOY_PROP_VISIBLE) {
        if (val.b) gtk_widget_show(w); else gtk_widget_hide(w);
    } else if (prop == ALLOY_PROP_CHECKED) {
        if (GTK_IS_TOGGLE_BUTTON(w)) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), val.b);
    } else if (prop == ALLOY_PROP_VALUE) {
        if (GTK_IS_RANGE(w)) gtk_range_set_value(GTK_RANGE(w), val.d);
        else if (GTK_IS_PROGRESS_BAR(w)) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w), val.d);
    }
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    if (!native_handle) return;
    if (prop == ALLOY_PROP_TEXT) {
        using namespace webview::detail::cocoa;
        if (objc::msg_send<BOOL>(native_handle, objc::selector("isKindOfClass:"), objc::get_class("NSWindow"))) {
            NSWindow_set_title(native_handle, val.s);
        }
    }
#endif
}

void Component::fire_event(alloy_event_type_t event) {
    auto it = callbacks.find(event);
    if (it != callbacks.end()) {
        it->second.first((alloy_component_t)this, event, it->second.second);
    }

    // Bridge to JavaScript if runtime is available
    if (this->runtime_id != "" && this->webview_ptr) {
        auto* wv = static_cast<webview::webview*>(this->webview_ptr);
        std::string js = "window.__alloy_on_gui_event(\"" + this->runtime_id + "\", " + std::to_string((int)event) + ")";
        wv->dispatch([wv, js]() {
            wv->eval(js);
        });
    }
}

} // namespace detail
} // namespace alloy

extern "C" {

#ifdef WEBVIEW_PLATFORM_LINUX
static bool is_gtk_init() {
    static bool initialized = false;
    static bool success = false;
    if (!initialized) {
        success = gtk_init_check(NULL, NULL);
        initialized = true;
    }
    return success;
}
#endif

alloy_component_t alloy_create_window(const char *title, int width, int height) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (is_gtk_init()) {
        GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(w), title);
        gtk_window_set_default_size(GTK_WINDOW(w), width, height);
        return (alloy_component_t)new Component(w);
    }
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    return (alloy_component_t)CocoaBackend::create_window(title, width, height);
#endif
    return (alloy_component_t)new Component(nullptr);
}

alloy_error_t alloy_destroy(alloy_component_t handle) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete (Component*)handle;
    return ALLOY_OK;
}

alloy_error_t alloy_set_text(alloy_component_t handle, const char *text) {
    Component *comp = (Component*)handle;
    if (!comp) return ALLOY_ERROR_INVALID_ARGUMENT;
#ifdef WEBVIEW_PLATFORM_LINUX
    if (!comp->widget) return ALLOY_OK;
    GtkWidget* w = (GtkWidget*)comp->widget;
    if (GTK_IS_WINDOW(w)) gtk_window_set_title(GTK_WINDOW(w), text);
    else if (GTK_IS_BUTTON(w)) gtk_button_set_label(GTK_BUTTON(w), text);
    else if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), text);
    else if (GTK_IS_ENTRY(w)) gtk_entry_set_text(GTK_ENTRY(w), text);
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    if (!comp->native_handle) return ALLOY_OK;
    using namespace webview::detail::cocoa;
    if (objc::msg_send<BOOL>(comp->native_handle, objc::selector("isKindOfClass:"), objc::get_class("NSWindow"))) {
        NSWindow_set_title(comp->native_handle, text);
    }
#endif
    return ALLOY_OK;
}

int alloy_get_text(alloy_component_t handle, char *buf, size_t buf_len) {
    Component *comp = (Component*)handle;
    if (!comp || !buf || buf_len == 0) return ALLOY_ERROR_INVALID_ARGUMENT;
#ifdef WEBVIEW_PLATFORM_LINUX
    if (!comp->widget) return 0;
    GtkWidget* w = (GtkWidget*)comp->widget;
    const char* text = "";
    if (GTK_IS_WINDOW(w)) text = gtk_window_get_title(GTK_WINDOW(w));
    else if (GTK_IS_BUTTON(w)) text = gtk_button_get_label(GTK_BUTTON(w));
    else if (GTK_IS_LABEL(w)) text = gtk_label_get_text(GTK_LABEL(w));
    else if (GTK_IS_ENTRY(w)) text = gtk_entry_get_text(GTK_ENTRY(w));
    if (text) {
        size_t len = strlen(text);
        if (len >= buf_len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
        strncpy(buf, text, buf_len);
        buf[buf_len - 1] = '\0';
        return (int)len;
    }
#endif
    return 0;
}

alloy_error_t alloy_set_checked(alloy_component_t h, int checked) {
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = (Component*)h;
    if (comp->widget && GTK_IS_TOGGLE_BUTTON((GtkWidget*)comp->widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON((GtkWidget*)comp->widget), checked);
#endif
    return ALLOY_OK;
}

int alloy_get_checked(alloy_component_t h) {
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = (Component*)h;
    if (comp->widget && GTK_IS_TOGGLE_BUTTON((GtkWidget*)comp->widget))
        return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON((GtkWidget*)comp->widget));
#endif
    return 0;
}

alloy_error_t alloy_set_value(alloy_component_t h, double value) {
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = (Component*)h;
    if (comp->widget) {
        GtkWidget* w = (GtkWidget*)comp->widget;
        if (GTK_IS_RANGE(w)) gtk_range_set_value(GTK_RANGE(w), value);
        else if (GTK_IS_PROGRESS_BAR(w)) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w), value);
    }
#endif
    return ALLOY_OK;
}

double alloy_get_value(alloy_component_t h) {
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = (Component*)h;
    if (comp->widget && GTK_IS_RANGE((GtkWidget*)comp->widget))
        return gtk_range_get_value(GTK_RANGE((GtkWidget*)comp->widget));
#endif
    return 0;
}

alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (((Component*)h)->widget) gtk_widget_set_sensitive(GTK_WIDGET(((Component*)h)->widget), enabled);
#endif
    return ALLOY_OK;
}

int alloy_get_enabled(alloy_component_t h) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (((Component*)h)->widget) return gtk_widget_get_sensitive(GTK_WIDGET(((Component*)h)->widget));
#endif
    return 1;
}

alloy_error_t alloy_set_visible(alloy_component_t h, int visible) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (((Component*)h)->widget) {
        if (visible) gtk_widget_show(GTK_WIDGET(((Component*)h)->widget));
        else gtk_widget_hide(GTK_WIDGET(((Component*)h)->widget));
    }
#endif
    return ALLOY_OK;
}

int alloy_get_visible(alloy_component_t h) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (((Component*)h)->widget) return gtk_widget_get_visible(GTK_WIDGET(((Component*)h)->widget));
#endif
    return 1;
}

alloy_error_t alloy_set_style(alloy_component_t, const alloy_style_t*) { return ALLOY_OK; }

alloy_error_t alloy_run(alloy_component_t /*window*/) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (is_gtk_init()) gtk_main();
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    using namespace webview::detail::cocoa;
    id app = NSApplication_get_sharedApplication();
    NSApplication_setActivationPolicy(app, NSApplicationActivationPolicyRegular);
    NSApplication_run(app);
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t /*window*/) {
#ifdef WEBVIEW_PLATFORM_LINUX
    if (is_gtk_init()) gtk_main_quit();
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    using namespace webview::detail::cocoa;
    NSApplication_stop(NSApplication_get_sharedApplication());
#endif
    return ALLOY_OK;
}

alloy_error_t alloy_dispatch(alloy_component_t /*window*/, void (*fn)(void*), void* arg) {
#ifdef WEBVIEW_PLATFORM_LINUX
    g_idle_add((GSourceFunc)fn, arg);
#endif
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
