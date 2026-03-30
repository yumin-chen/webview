#include "alloy/api.h"
#include "alloy/detail/components/tabview.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_tabview_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                               0, 0, 300, 200, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_tabview(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_tabview_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_tabview_gtk(alloy_component_t parent) {
    GtkWidget* notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    return new gtk_tabview(notebook);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_tabview(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_tabview_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_tabview_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_tabview_gtk(parent);
#else
    return nullptr;
#endif
}
}
