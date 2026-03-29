#ifndef ALLOY_VSTACK_HH
#define ALLOY_VSTACK_HH

#include "../component_base.hh"
#include "../backends.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
class win32_vstack : public win32_component {
public:
    win32_vstack(HWND hwnd) : win32_component(hwnd, true) {}
};
#elif defined(ALLOY_PLATFORM_DARWIN)
class cocoa_vstack : public cocoa_component {
public:
    cocoa_vstack(id view) : cocoa_component(view, true) {}
};
#elif defined(ALLOY_PLATFORM_LINUX)
class gtk_vstack : public gtk_component {
public:
    gtk_vstack(GtkWidget* widget) : gtk_component(widget, true) {}
};
#endif

}

#endif
