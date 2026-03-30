#include "alloy/api.h"
#include "alloy/detail/components/switch.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_switch_win(alloy_component_t parent) {
    // Windows doesn't have a native switch, use a checkbox styled as a toggle or a custom control.
    // For now, use a checkbox.
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"BUTTON", L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE,
                               0, 0, 50, 25, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_switch(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_switch_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_switch_gtk(alloy_component_t parent) {
    GtkWidget* sw = gtk_switch_new();
    gtk_widget_show(sw);
    return new gtk_switch_comp(sw);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_switch(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_switch_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_switch_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_switch_gtk(parent);
#else
    return nullptr;
#endif
}
}
