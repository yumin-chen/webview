#include "alloy/api.h"
#include "alloy/detail/components/textfield.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
alloy_component_t create_textfield_win(alloy_component_t parent) {
    auto p = static_cast<win32_component*>(parent);
    HWND parent_hwnd = (HWND)p->native_handle();
    HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
                              0, 0, 200, 25, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);
    return new win32_textfield(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_textfield_cocoa(alloy_component_t parent) {
    auto p = static_cast<cocoa_component*>(parent);
    id parent_view = (id)p->native_handle();
    id field = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSTextField"), sel_registerName("alloc"));
    ((id (*)(id, SEL, NSRect))objc_msgSend)(field, sel_registerName("initWithFrame:"), (NSRect){{0, 0}, {200, 25}});
    ((void (*)(id, SEL, id))objc_msgSend)(parent_view, sel_registerName("addSubview:"), field);
    return new cocoa_textfield(field);
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_textfield_gtk(alloy_component_t parent) {
    auto p = static_cast<gtk_component*>(parent);
    GtkWidget* entry = gtk_entry_new();
    gtk_widget_show(entry);
    return new gtk_textfield(entry);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_textfield(alloy_component_t parent) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_textfield_win(parent);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_textfield_cocoa(parent);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_textfield_gtk(parent);
#else
    return nullptr;
#endif
}
}
