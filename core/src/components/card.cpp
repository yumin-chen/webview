#include "alloy/api.h"
#include "alloy/detail/components/card.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_card_win(alloy_component_t parent) {
    return new win32_card(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_card_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_card_gtk(alloy_component_t parent) {
    return new gtk_card(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_card(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_card_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_card_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_card_gtk(parent);
#else
    return nullptr;
#endif
}
}
