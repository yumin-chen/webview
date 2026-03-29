#include "alloy/api.h"
#include "alloy/detail/components/textarea.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_textarea_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = p ? (HWND)p->native_handle() : NULL;
    HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL,
                              0, 0, 200, 100, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_textarea(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_textarea_cocoa(alloy_component_t parent) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_textarea_gtk(alloy_component_t parent) {
    GtkWidget* textview = gtk_text_view_new();
    gtk_widget_show(textview);
    return new gtk_textarea(textview);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_textarea(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_textarea_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_textarea_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_textarea_gtk(parent);
#else
    return nullptr;
#endif
}
}
