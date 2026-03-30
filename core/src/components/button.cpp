#include "alloy/api.h"
#include "alloy/detail/components/button.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_button_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = (HWND)p->native_handle();
    HWND hwnd = CreateWindowExW(0, L"BUTTON", L"Button", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                               0, 0, 100, 30, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_button(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_button_cocoa(alloy_component_t parent) {
    auto p = static_cast<cocoa_component*>(parent);
    id parent_view = (id)p->native_handle();
    id btn = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSButton"), sel_registerName("alloc"));
    ((id (*)(id, SEL, NSRect))objc_msgSend)(btn, sel_registerName("initWithFrame:"), (NSRect){{0, 0}, {100, 30}});
    ((void (*)(id, SEL, id))objc_msgSend)(parent_view, sel_registerName("addSubview:"), btn);
    return new cocoa_button(btn);
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_button_gtk(alloy_component_t parent) {
    auto p = static_cast<gtk_component*>(parent);
    GtkWidget* parent_widget = (GtkWidget*)p->native_handle();
    GtkWidget* btn = gtk_button_new_with_label("Button");
    gtk_widget_show(btn);
    return new gtk_button_comp(btn);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_button(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_button_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_button_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_button_gtk(parent);
#else
    return nullptr;
#endif
}
}
