#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

alloy_error_t alloy_create_dialog(alloy_component_t p, alloy_component_t *o) {
#ifdef _WIN32
    *o = (alloy_component_t)CreateWindowExW(WS_EX_DLGMODALFRAME, L"AlloyWindow", L"Dialog", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, (HWND)p, NULL, GetModuleHandle(NULL), NULL);
#elif defined(__linux__)
    *o = (alloy_component_t)gtk_dialog_new();
    gtk_widget_show_all(GTK_WIDGET(*o));
#endif
    return *o ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
