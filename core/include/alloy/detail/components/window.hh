#ifndef ALLOY_WINDOW_HH
#define ALLOY_WINDOW_HH

#include "../component_base.hh"
#include "../backends.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
#include "../platform/windows/theme_fluent.hh"
class win32_window : public win32_component {
public:
    win32_window(HWND hwnd) : win32_component(hwnd, true) {
        apply_fluent_theme(hwnd);
    }
};
#elif defined(ALLOY_PLATFORM_DARWIN)
class cocoa_window : public cocoa_component {
public:
    using cocoa_component::cocoa_component;
};
#elif defined(ALLOY_PLATFORM_LINUX)
class gtk_window_comp : public gtk_component {
public:
    using gtk_component::gtk_component;
};
#endif

}

#endif
