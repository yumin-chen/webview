#ifndef ALLOY_GUI_COMPONENT_HH
#define ALLOY_GUI_COMPONENT_HH

#include "alloy_gui/api.h"
#include <string>
#include <vector>
#include <map>

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
#elif defined(WEBVIEW_PLATFORM_DARWIN)
#include "webview/detail/platform/darwin/objc/objc.hh"
#endif

namespace alloy {
namespace detail {

enum class signal_type { STR, DOUBLE, INT, BOOL };
struct signal_value {
    signal_type type;
    std::string s;
    double d;
    int i;
    bool b;
};

struct Component;
struct signal_base {
    signal_value value;
    std::vector<std::pair<Component*, alloy_prop_id_t>> subscribers;
    void notify();
};

struct Component {
#ifdef WEBVIEW_PLATFORM_LINUX
    GtkWidget *widget;
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    id native_handle;
#else
    void* widget;
#endif
    std::map<alloy_event_type_t, std::pair<alloy_event_cb_t, void*>> callbacks;
    std::vector<Component*> children;
    float flex = 0;
    bool is_container = false;

    // Runtime association for event bridging
    std::string runtime_id;
    void* webview_ptr = nullptr;

#ifdef WEBVIEW_PLATFORM_LINUX
    Component(GtkWidget *w) : widget(w) {}
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    Component(id h) : native_handle(h) {}
#else
    Component(void* w) : widget(w) {}
#endif

    virtual ~Component() {
#ifdef WEBVIEW_PLATFORM_LINUX
        if (widget) {
            gtk_widget_destroy(widget);
        }
#endif
    }

    void on_signal_changed(alloy_prop_id_t prop, const signal_value& val);
    void fire_event(alloy_event_type_t event);
};

} // namespace detail
} // namespace alloy

#endif
