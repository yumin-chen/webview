#ifndef ALLOY_SCROLLVIEW_HH
#define ALLOY_SCROLLVIEW_HH

#include "../component_base.hh"
#include "../backends.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
class win32_scrollview : public win32_component {
public:
    win32_scrollview(HWND hwnd) : win32_component(hwnd, true) {}
};
#elif defined(ALLOY_PLATFORM_DARWIN)
class cocoa_scrollview : public cocoa_component {
public:
    cocoa_scrollview(id view) : cocoa_component(view, true) {}
};
#elif defined(ALLOY_PLATFORM_LINUX)
class gtk_scrollview : public gtk_component {
public:
    gtk_scrollview(GtkWidget* widget) : gtk_component(widget, true) {}
};
#endif

}

#endif
