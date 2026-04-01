#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

alloy_error_t alloy_create_combobox(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN, 0, 0, 100, 150, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}

alloy_error_t alloy_create_slider(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(0, L"msctls_trackbar32", L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 150, 30, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
