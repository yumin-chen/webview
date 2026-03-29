#include "alloy/api.h"
#include "alloy/detail/components/vstack.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_vstack_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                               0, 0, 100, 100, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_vstack(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_vstack_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_vstack_gtk(alloy_component_t parent) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(box);
    return new gtk_vstack(box);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_vstack(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_vstack_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_vstack_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_vstack_gtk(parent);
#else
    return nullptr;
#endif
}
}
