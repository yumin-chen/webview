#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

alloy_error_t alloy_create_label(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(0, L"STATIC", L"Label", WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_label_new("Label");
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(*o));
    gtk_widget_show(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
