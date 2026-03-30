#ifndef ALLOY_HSTACK_HH
#define ALLOY_HSTACK_HH

#include "../component_base.hh"
#include "../backends.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
class win32_hstack : public win32_component {
public:
    win32_hstack(HWND hwnd) : win32_component(hwnd, true) {}
};
#elif defined(ALLOY_PLATFORM_DARWIN)
class cocoa_hstack : public cocoa_component {
public:
    cocoa_hstack(id view) : cocoa_component(view, true) {}
};
#elif defined(ALLOY_PLATFORM_LINUX)
class gtk_hstack : public gtk_component {
public:
    gtk_hstack(GtkWidget* widget) : gtk_component(widget, true) {}
};
#endif

}

#endif
