#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

typedef struct {
    void *native_handle;
} alloy_btn_internal_t;

alloy_error_t alloy_create_button(alloy_component_t p, alloy_component_t *o) {
    alloy_btn_internal_t *btn = (alloy_btn_internal_t*)calloc(1, sizeof(alloy_btn_internal_t));
#ifdef _WIN32
    btn->native_handle = CreateWindowExW(0, L"BUTTON", L"Button", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    btn->native_handle = gtk_button_new_with_label("Button");
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(btn->native_handle));
    gtk_widget_show(GTK_WIDGET(btn->native_handle));
#endif
    *o = (alloy_component_t)btn;
    return btn->native_handle ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
