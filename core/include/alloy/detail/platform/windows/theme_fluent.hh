#ifndef ALLOY_THEME_FLUENT_HH
#define ALLOY_THEME_FLUENT_HH

#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif

namespace alloy::detail {

inline void apply_fluent_theme(HWND hwnd) {
    // Enable dark mode
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

    // Rounded corners
    DWORD corner_preference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_preference, sizeof(corner_preference));

    // Mica effect (Windows 11)
    DWORD mica_value = 1;
    DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &mica_value, sizeof(mica_value));
}

}

#endif
