#include "alloy/api.h"
#include "alloy/detail/components/datepicker.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_datepicker_win(alloy_component_t parent) {
    return new win32_datepicker(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_datepicker_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_datepicker_gtk(alloy_component_t parent) {
    return new gtk_datepicker(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_datepicker(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_datepicker_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_datepicker_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_datepicker_gtk(parent);
#else
    return nullptr;
#endif
}
}
