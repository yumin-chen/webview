#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

alloy_error_t alloy_create_checkbox(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(0, L"BUTTON", L"Check", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 0, 0, 100, 20, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_check_button_new_with_label("Check");
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
