#ifndef ALLOY_WIN32_GUI_HH
#define ALLOY_WIN32_GUI_HH

#include "../component_base.hh"
#include <windows.h>
#include <commctrl.h>

namespace alloy::detail {

class win32_component : public component_base {
public:
    win32_component(HWND hwnd, bool is_container = false)
        : component_base(is_container), m_hwnd(hwnd) {}

    virtual ~win32_component() {
        if (m_hwnd) DestroyWindow(m_hwnd);
    }

    alloy_error_t set_text(std::string_view text) override {
        SetWindowTextA(m_hwnd, text.data());
        return ALLOY_OK;
    }

    alloy_error_t get_text(char *buf, size_t len) override {
        GetWindowTextA(m_hwnd, buf, (int)len);
        return ALLOY_OK;
    }

    alloy_error_t set_checked(bool v) override {
        SendMessage(m_hwnd, BM_SETCHECK, v ? BST_CHECKED : BST_UNCHECKED, 0);
        return ALLOY_OK;
    }

    bool get_checked() override {
        return SendMessage(m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    alloy_error_t set_value(double v) override {
        // Implementation for ProgressBar/Slider
        return ALLOY_OK;
    }

    double get_value() override {
        return 0.0;
    }

    alloy_error_t set_enabled(bool v) override {
        EnableWindow(m_hwnd, v);
        return ALLOY_OK;
    }

    bool get_enabled() override {
        return IsWindowEnabled(m_hwnd);
    }

    alloy_error_t set_visible(bool v) override {
        ShowWindow(m_hwnd, v ? SW_SHOW : SW_HIDE);
        return ALLOY_OK;
    }

    bool get_visible() override {
        return IsWindowVisible(m_hwnd);
    }

    alloy_error_t set_style(const alloy_style_t &s) override {
        // Apply styling
        return ALLOY_OK;
    }

    void *native_handle() override { return m_hwnd; }

protected:
    HWND m_hwnd;
};

// Specific component implementations (Window, Button, etc.) would follow
}

#endif
