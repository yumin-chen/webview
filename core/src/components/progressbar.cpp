#include "alloy/api.h"
#include "alloy/detail/components/progressbar.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_progressbar_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
                               0, 0, 150, 20, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_progressbar(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_progressbar_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_progressbar_gtk(alloy_component_t parent) {
    GtkWidget* pb = gtk_progress_bar_new();
    gtk_widget_show(pb);
    return new gtk_progressbar(pb);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_progressbar(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_progressbar_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_progressbar_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_progressbar_gtk(parent);
#else
    return nullptr;
#endif
}
}
