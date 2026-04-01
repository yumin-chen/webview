#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

alloy_error_t alloy_create_menu(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateMenu();
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_menu_new();
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}

alloy_error_t alloy_create_menubar(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateMenu();
    SetMenu((HWND)p, (HMENU)*o);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_menu_bar_new();
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
