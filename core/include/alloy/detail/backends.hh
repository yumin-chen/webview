#ifndef ALLOY_BACKENDS_HH
#define ALLOY_BACKENDS_HH

#if defined(ALLOY_PLATFORM_WINDOWS)
#include "backends/win32_gui.hh"
#elif defined(ALLOY_PLATFORM_DARWIN)
#include "backends/cocoa_gui.hh"
#elif defined(ALLOY_PLATFORM_LINUX)
#include "backends/gtk_gui.hh"
#endif

#endif
