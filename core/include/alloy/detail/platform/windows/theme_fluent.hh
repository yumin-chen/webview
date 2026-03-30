#ifndef ALLOY_PLATFORM_WINDOWS_THEME_FLUENT_HH
#define ALLOY_PLATFORM_WINDOWS_THEME_FLUENT_HH

#include <windows.h>
#include <dwmapi.h>

namespace alloy::detail {

inline void apply_fluent_theme(HWND window) {
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20
    // DWMWA_MICA_EFFECT = 1029
    // DWMWA_SYSTEMBACKDROP_TYPE = 38
    // DWMSBT_MICA = 2

    BOOL dark_mode = TRUE;
    DwmSetWindowAttribute(window, 20, &dark_mode, sizeof(dark_mode));

    int backdrop_type = 2; // Mica
    DwmSetWindowAttribute(window, 38, &backdrop_type, sizeof(backdrop_type));

    // Rounded corners: DWMWA_WINDOW_CORNER_PREFERENCE = 33
    // DWMWCP_ROUND = 2
    int corner_preference = 2;
    DwmSetWindowAttribute(window, 33, &corner_preference, sizeof(corner_preference));
}

}

#endif
