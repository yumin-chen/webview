#include "alloy/api.h"
#include "alloy/detail/components/icon.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_icon_win(alloy_component_t parent) {
    return new win32_icon(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_icon_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_icon_gtk(alloy_component_t parent) {
    return new gtk_icon(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_icon(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_icon_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_icon_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_icon_gtk(parent);
#else
    return nullptr;
#endif
}
}
