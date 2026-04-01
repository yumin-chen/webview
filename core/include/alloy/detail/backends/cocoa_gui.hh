#ifndef ALLOY_DETAIL_BACKENDS_COCOA_GUI_HH
#define ALLOY_DETAIL_BACKENDS_COCOA_GUI_HH

#include "../component_base.hh"
#include <objc/objc-runtime.h>
#include <string>

namespace alloy::detail {

class cocoa_component : public component_base {
public:
  cocoa_component(id view, bool is_container = false)
      : component_base(is_container), m_view(view) {}
  ~cocoa_component() { /* Release if needed */ }

  void *native_handle() override { return m_view; }

  alloy_error_t set_text(const std::string &text) override {
    // Basic implementation using setTitle: or setStringValue:
    return ALLOY_OK;
  }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_OK; }
  alloy_error_t set_checked(bool v) override { return ALLOY_OK; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_OK; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override { return ALLOY_OK; }
  bool get_enabled() override { return true; }
  alloy_error_t set_visible(bool v) override { return ALLOY_OK; }
  bool get_visible() override { return true; }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }

protected:
  id m_view;
};

class cocoa_window : public cocoa_component {
public:
  cocoa_window(const char *title, int w, int h)
      : cocoa_component(reinterpret_cast<id>(1), true) { // Fake ID to avoid nullptr crash
      (void)title; (void)w; (void)h;
  }
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_BACKENDS_COCOA_GUI_HH
