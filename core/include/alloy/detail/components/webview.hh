#ifndef ALLOY_WEBVIEW_HH
#define ALLOY_WEBVIEW_HH

#include "../component_base.hh"
#include "../backends.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
class win32_webview_comp : public win32_component {
public:
    using win32_component::win32_component;

    alloy_error_t bind_global(const char *name,
                               void (*cb)(const char *, void *),
                               void *ud) override {
        // Implementation for Windows WebView2 would go here.
        // For now, this serves as the binding point.
        return ALLOY_OK;
    }
};
#elif defined(ALLOY_PLATFORM_DARWIN)
class cocoa_webview_comp : public cocoa_component {
public:
    using cocoa_component::cocoa_component;
};
#elif defined(ALLOY_PLATFORM_LINUX)
class gtk_webview_comp : public gtk_component {
public:
    using gtk_component::gtk_component;
};
#endif

}

#endif
