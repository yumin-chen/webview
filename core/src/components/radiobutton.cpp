#include "alloy/api.h"
#include "alloy/detail/components/radiobutton.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_radiobutton_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, L"BUTTON", L"RadioButton", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
                               0, 0, 100, 20, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_radiobutton(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_radiobutton_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_radiobutton_gtk(alloy_component_t parent) {
    GtkWidget* rb = gtk_radio_button_new_with_label(NULL, "RadioButton");
    gtk_widget_show(rb);
    return new gtk_radiobutton(rb);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_radiobutton(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_radiobutton_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_radiobutton_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_radiobutton_gtk(parent);
#else
    return nullptr;
#endif
}
}
