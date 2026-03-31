#include "../alloy.h"
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

typedef struct {
    void *native_handle;
} alloy_tf_internal_t;

alloy_error_t alloy_create_textfield(alloy_component_t p, alloy_component_t *o) {
    alloy_tf_internal_t *tf = (alloy_tf_internal_t*)calloc(1, sizeof(alloy_tf_internal_t));
#ifdef _WIN32
    tf->native_handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT, 0, 0, 100, 25, (HWND)p, NULL, NULL, NULL);
#elif defined(__linux__)
    tf->native_handle = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(p), GTK_WIDGET(tf->native_handle));
    gtk_widget_show(GTK_WIDGET(tf->native_handle));
#endif
    *o = (alloy_component_t)tf;
    return tf->native_handle ? ALLOY_OK : ALLOY_ERROR_PLATFORM;
}
