#ifndef ALLOY_RICHTEXT_HH
#define ALLOY_RICHTEXT_HH

#include "../component_base.hh"
#include "../backends.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
class win32_richtext : public win32_component {
public:
    using win32_component::win32_component;
};
#elif defined(ALLOY_PLATFORM_DARWIN)
class cocoa_richtext : public cocoa_component {
public:
    using cocoa_component::cocoa_component;
};
#elif defined(ALLOY_PLATFORM_LINUX)
class gtk_richtext : public gtk_component {
public:
    using gtk_component::gtk_component;
};
#endif

}

#endif
