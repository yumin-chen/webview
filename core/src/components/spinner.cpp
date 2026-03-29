#include "alloy/api.h"
#include "alloy/detail/components/spinner.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_spinner_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(0, UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS,
                               0, 0, 50, 30, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_spinner(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_spinner_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_spinner_gtk(alloy_component_t parent) {
    GtkWidget* spinner = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_widget_show(spinner);
    return new gtk_spinner_comp(spinner);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_spinner(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_spinner_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_spinner_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_spinner_gtk(parent);
#else
    return nullptr;
#endif
}
}
