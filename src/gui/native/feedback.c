#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#include <commctrl.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

alloy_error_t alloy_create_progressbar(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 150, 20, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_progress_bar_new();
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}

alloy_error_t alloy_create_spinner(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(0, UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 50, 30, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_spin_button_new_with_range(0, 100, 1);
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
