#ifndef ALLOY_BACKENDS_GTK_GUI_HH
#define ALLOY_BACKENDS_GTK_GUI_HH

#include "../component_base.hh"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace alloy::detail {

class gtk_button : public component_base {
public:
  gtk_button(component_base* parent) : component_base(false) {
    m_widget = gtk_button_new();
    if (parent) {
      // Logic to add to parent container
    }
  }

  alloy_error_t set_text(std::string_view text) override {
    gtk_button_set_label(GTK_BUTTON(m_widget), text.data());
    return ALLOY_OK;
  }

  // ... other overrides ...
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_OK; }
  alloy_error_t set_checked(bool v) override { return ALLOY_OK; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_OK; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override {
    gtk_widget_set_sensitive(m_widget, v);
    return ALLOY_OK;
  }
  bool get_enabled() override { return gtk_widget_get_sensitive(m_widget); }
  alloy_error_t set_visible(bool v) override {
    gtk_widget_set_visible(m_widget, v);
    return ALLOY_OK;
  }
  bool get_visible() override { return gtk_widget_get_visible(m_widget); }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }

  void* native_handle() override { return m_widget; }

private:
  GtkWidget* m_widget;
};

} // namespace alloy::detail

#endif // ALLOY_BACKENDS_GTK_GUI_HH
