#include "alloy/api.h"
#include "alloy/detail/components/checkbox.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_checkbox_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"BUTTON", L"CheckBox", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                               0, 0, 100, 20, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_checkbox(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_checkbox_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_checkbox_gtk(alloy_component_t parent) {
    GtkWidget* cb = gtk_check_button_new_with_label("CheckBox");
    gtk_widget_show(cb);
    return new gtk_checkbox(cb);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_checkbox(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_checkbox_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_checkbox_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_checkbox_gtk(parent);
#else
    return nullptr;
#endif
}
}
