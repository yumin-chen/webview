#include "alloy/api.h"
#include "alloy/detail/components/filedialog.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_filedialog_win(alloy_component_t parent) {
    return new win32_filedialog(NULL);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_filedialog_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_filedialog_gtk(alloy_component_t parent) {
    return new gtk_filedialog(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_filedialog(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_filedialog_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_filedialog_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_filedialog_gtk(parent);
#else
    return nullptr;
#endif
}
}
