#include "alloy/api.h"
#include "alloy/detail/components/divider.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_divider_win(alloy_component_t parent) {
    return new win32_divider(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_divider_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_divider_gtk(alloy_component_t parent) {
    return new gtk_divider(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_divider(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_divider_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_divider_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_divider_gtk(parent);
#else
    return nullptr;
#endif
}
}
