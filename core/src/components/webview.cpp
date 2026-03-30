#include "alloy/api.h"
#include "alloy/detail/components/webview.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_webview_win(alloy_component_t parent) {
    // This would involve initializing WebView2
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_webview_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_webview_gtk(alloy_component_t parent) {
    return nullptr;
}
#endif

}

extern "C" {
alloy_component_t alloy_create_webview(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_webview_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_webview_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_webview_gtk(parent);
#else
    return nullptr;
#endif
}
}
