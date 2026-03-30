#include "alloy/api.h"
#include "alloy/detail/components/window.hh"

namespace alloy::detail {

#if defined(ALLOY_PLATFORM_WINDOWS)
#include "alloy/detail/platform/windows/theme_fluent.hh"

LRESULT CALLBACK AlloyWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* comp = (alloy::detail::win32_component*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_COMMAND: {
            HWND child_hwnd = (HWND)lp;
            if (child_hwnd) {
                auto* child_comp = (alloy::detail::win32_component*)GetWindowLongPtr(child_hwnd, GWLP_USERDATA);
                if (child_comp) {
                    if (HIWORD(wp) == BN_CLICKED) child_comp->fire_event(ALLOY_EVENT_CLICK);
                    else if (HIWORD(wp) == EN_CHANGE) child_comp->fire_event(ALLOY_EVENT_CHANGE);
                }
            }
            break;
        }
        case WM_NOTIFY: {
            NMHDR* nmhdr = (NMHDR*)lp;
            if (nmhdr->hwndFrom) {
                auto* child_comp = (alloy::detail::win32_component*)GetWindowLongPtr(nmhdr->hwndFrom, GWLP_USERDATA);
                if (child_comp) {
                    if (nmhdr->code == NM_CLICK) child_comp->fire_event(ALLOY_EVENT_CLICK);
                }
            }
            break;
        }
        case WM_APP + 1: {
            void (*fn)(void *arg) = (void (*)(void *))wp;
            fn((void*)lp);
            break;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

alloy_component_t create_window_win(const char *title, int width, int height) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = AlloyWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"AlloyWindow";
    RegisterClassExW(&wc);

    std::wstring wtitle = L"Alloy";
    if (title) {
        int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
        if (len > 0) {
            std::vector<wchar_t> buf(len);
            MultiByteToWideChar(CP_UTF8, 0, title, -1, buf.data(), len);
            wtitle = buf.data();
        }
    }

    HWND hwnd = CreateWindowExW(0, L"AlloyWindow", wtitle.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);

    alloy::detail::apply_fluent_theme(hwnd);

    return new win32_window(hwnd);
}
#elif defined(ALLOY_PLATFORM_DARWIN)
alloy_component_t create_window_cocoa(const char *title, int width, int height) {
    return nullptr;
}
#elif defined(ALLOY_PLATFORM_LINUX)
alloy_component_t create_window_gtk(const char *title, int width, int height) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    gtk_widget_show(window);
    return new gtk_window_comp(window);
}
#endif

}

extern "C" {
alloy_component_t alloy_create_window(const char *title, int width, int height) {
#if defined(ALLOY_PLATFORM_WINDOWS)
    return alloy::detail::create_window_win(title, width, height);
#elif defined(ALLOY_PLATFORM_DARWIN)
    return alloy::detail::create_window_cocoa(title, width, height);
#elif defined(ALLOY_PLATFORM_LINUX)
    return alloy::detail::create_window_gtk(title, width, height);
#else
    return nullptr;
#endif
}
}
