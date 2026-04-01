#include "alloy/api.h"
#include "alloy/detail/components/menubar.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_menubar_win(alloy_component_t parent) {
        auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | 0,
                               0, 0, 100, 25, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_menubar(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_menubar_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_menubar_gtk(alloy_component_t parent) {
    return new gtk_menubar(nullptr);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_menubar(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_menubar_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_menubar_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_menubar_gtk(parent);
#else
    return nullptr;
#endif
}
}
