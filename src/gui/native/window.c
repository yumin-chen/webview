#include "../alloy.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#endif

typedef struct {
    void *native_handle;
    char *title;
} alloy_win_internal_t;

#ifdef _WIN32
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CLOSE) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
#endif

alloy_error_t alloy_create_window(const char *title, int width, int height, alloy_component_t *out_window) {
    alloy_win_internal_t *win = (alloy_win_internal_t*)calloc(1, sizeof(alloy_win_internal_t));
    if (!win) return ALLOY_ERROR_PLATFORM;
    win->title = strdup(title);

#ifdef _WIN32
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"AlloyWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    win->native_handle = CreateWindowExW(0, L"AlloyWindow", L"Alloy", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, wc.hInstance, NULL);
    if (win->native_handle) SetWindowTextA((HWND)win->native_handle, title);
#elif defined(__linux__)
    if (!gtk_init_check(NULL, NULL)) return ALLOY_ERROR_PLATFORM;
    win->native_handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win->native_handle), title);
    gtk_window_set_default_size(GTK_WINDOW(win->native_handle), width, height);
    g_signal_connect(win->native_handle, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(win->native_handle);
#endif

    if (!win->native_handle) { free(win->title); free(win); return ALLOY_ERROR_PLATFORM; }
    *out_window = (alloy_component_t)win;
    return ALLOY_OK;
}

alloy_error_t alloy_run(alloy_component_t window) {
#ifdef _WIN32
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#elif defined(__linux__)
    gtk_main();
#endif
    return ALLOY_OK;
}
