#include "cron_manager.hpp"

namespace alloy {

#ifdef _WIN32
// Included via cron_manager_windows.cpp in actual build
#elif defined(__APPLE__)
// Included via cron_manager_macos.mm in actual build
#else
// Included via cron_manager_linux.cpp in actual build
#endif

} // namespace alloy
