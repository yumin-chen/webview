#include "alloy/api.h"
#include "alloy/detail/components/label.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_label_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"STATIC", L"Label", WS_CHILD | WS_VISIBLE | SS_LEFT,
                              0, 0, 100, 20, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_label(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_label_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_label_gtk(alloy_component_t parent) {
    GtkWidget* lbl = gtk_label_new("Label");
    gtk_widget_show(lbl);
    return new gtk_label_comp(lbl);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_label(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_label_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_label_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_label_gtk(parent);
#else
    return nullptr;
#endif
}
}
