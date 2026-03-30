#include "alloy/api.h"
#include "alloy/detail/components/scrollview.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_scrollview_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
                               0, 0, 200, 200, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_scrollview(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_scrollview_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_scrollview_gtk(alloy_component_t parent) {
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolled);
    return new gtk_scrollview(scrolled);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_scrollview(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_scrollview_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_scrollview_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_scrollview_gtk(parent);
#else
    return nullptr;
#endif
}
}
