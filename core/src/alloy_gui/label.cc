#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"
#ifdef WEBVIEW_PLATFORM_LINUX
#include "alloy_gui/detail/backends/gtk_backend.hh"
#elif defined(WEBVIEW_PLATFORM_DARWIN)
#include "alloy_gui/detail/backends/cocoa_backend.hh"
#endif

using namespace alloy::detail;

extern "C" {

alloy_component_t alloy_create_label(alloy_component_t parent) {
    Component* p = (Component*)parent;
#ifdef WEBVIEW_PLATFORM_LINUX
    Component* comp = GTKBackend::create_label(p);
    if (p && p->widget && GTK_IS_CONTAINER(p->widget)) {
        gtk_container_add(GTK_CONTAINER(p->widget), comp->widget);
    }
    return (alloy_component_t)comp;
#elif defined(WEBVIEW_PLATFORM_DARWIN)
    Component* comp = CocoaBackend::create_label(p);
    if (p && p->native_handle) {
        using namespace webview::detail;
        id contentView = p->native_handle;
        if (objc::msg_send<BOOL>(p->native_handle, objc::selector("isKindOfClass:"), objc::get_class("NSWindow"))) {
            contentView = objc::msg_send<id>(p->native_handle, objc::selector("contentView"));
        }
        objc::msg_send<void>(contentView, objc::selector("addSubview:"), comp->native_handle);
    }
    return (alloy_component_t)comp;
#else
    return (alloy_component_t)new Component(nullptr);
#endif
}

}
