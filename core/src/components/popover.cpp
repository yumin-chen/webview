#include "alloy/api.h"
#include "alloy/detail/components/popover.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_popover_win(alloy_component_t parent) {
    return new win32_popover(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_popover_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_popover_gtk(alloy_component_t parent) {
    return new gtk_popover(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_popover(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_popover_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_popover_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_popover_gtk(parent);
#else
    return nullptr;
#endif
}
}
