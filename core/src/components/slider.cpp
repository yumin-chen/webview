#include "alloy/api.h"
#include "alloy/detail/components/slider.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_slider_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_HORZ,
                               0, 0, 150, 30, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_slider(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_slider_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_slider_gtk(alloy_component_t parent) {
    GtkWidget* slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_show(slider);
    return new gtk_slider(slider);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_slider(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_slider_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_slider_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_slider_gtk(parent);
#else
    return nullptr;
#endif
}
}
