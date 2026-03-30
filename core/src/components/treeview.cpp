#include "alloy/api.h"
#include "alloy/detail/components/treeview.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_treeview_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, WC_TREEVIEWW, L"", WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_BORDER,
                               0, 0, 200, 150, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_treeview(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_treeview_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_treeview_gtk(alloy_component_t parent) {
    GtkWidget* treeview = gtk_tree_view_new();
    gtk_widget_show(treeview);
    return new gtk_treeview(treeview);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_treeview(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_treeview_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_treeview_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_treeview_gtk(parent);
#else
    return nullptr;
#endif
}
}
