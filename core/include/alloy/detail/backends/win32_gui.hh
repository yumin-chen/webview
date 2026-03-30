#ifndef ALLOY_DETAIL_BACKENDS_WIN32_GUI_HH
#define ALLOY_DETAIL_BACKENDS_WIN32_GUI_HH

#include "../component_base.hh"
#include <windows.h>
#include <commctrl.h>

namespace alloy::detail {

class win32_component : public component_base {
public:
  win32_component(HWND hwnd, bool is_container = false)
      : component_base(is_container), m_hwnd(hwnd) {}
  ~win32_component() { if (m_hwnd) DestroyWindow(m_hwnd); }

  void *native_handle() override { return m_hwnd; }

  alloy_error_t set_text(const std::string &text) override {
    SetWindowTextA(m_hwnd, text.c_str());
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
    if (GetClassNameA(m_hwnd, NULL, 0) == "PROGRESS_CLASS") {
        SendMessage(m_hwnd, PBM_SETPOS, (WPARAM)(v * 100), 0);
    }
    return ALLOY_OK;
  }
  double get_value() override {
    if (GetClassNameA(m_hwnd, NULL, 0) == "PROGRESS_CLASS") {
        return (double)SendMessage(m_hwnd, PBM_GETPOS, 0, 0) / 100.0;
    }
    return 0;
  }

  alloy_error_t set_enabled(bool v) override {
    EnableWindow(m_hwnd, v);
    return ALLOY_OK;
  }
  bool get_enabled() override { return IsWindowEnabled(m_hwnd); }

  alloy_error_t set_visible(bool v) override {
    ShowWindow(m_hwnd, v ? SW_SHOW : SW_HIDE);
    return ALLOY_OK;
  }
  bool get_visible() override { return IsWindowVisible(m_hwnd); }

  alloy_error_t set_style(const alloy_style_t &s) override {
    (void)s;
    return ALLOY_OK;
  }

protected:
  HWND m_hwnd;
};

class win32_window : public win32_component {
public:
  win32_window(const char *title, int w, int h)
      : win32_component(CreateWindowExA(0, "AlloyWindow", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, GetModuleHandle(NULL), NULL), true) {
  }
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_BACKENDS_WIN32_GUI_HH
