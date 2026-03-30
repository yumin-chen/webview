#include "alloy/api.h"
#include "alloy/detail/components/spinner_loading.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_spinner_loading_win(alloy_component_t parent) {
    return new win32_spinner_loading(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_spinner_loading_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_spinner_loading_gtk(alloy_component_t parent) {
    return new gtk_spinner_loading(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_spinner_loading(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_spinner_loading_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_spinner_loading_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_spinner_loading_gtk(parent);
#else
    return nullptr;
#endif
}
}
