#include "alloy/api.h"
#include "alloy/detail/components/combobox.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_combobox_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"COMBOBOX", L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN,
                               0, 0, 150, 30, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_combobox(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_combobox_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_combobox_gtk(alloy_component_t parent) {
    GtkWidget* cb = gtk_combo_box_text_new();
    gtk_widget_show(cb);
    return new gtk_combobox(cb);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_combobox(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_combobox_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_combobox_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_combobox_gtk(parent);
#else
    return nullptr;
#endif
}
}
