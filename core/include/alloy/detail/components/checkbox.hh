#ifndef ALLOY_DETAIL_COMPONENTS_CHECKBOX_HH
#define ALLOY_DETAIL_COMPONENTS_CHECKBOX_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {
class gtk_checkbox : public component_base {
public:
  explicit gtk_checkbox(component_base *parent) : component_base(false) {
    m_widget = gtk_check_button_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_checkbox() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override {
    gtk_button_set_label(GTK_BUTTON(m_widget), text.data());
    return ALLOY_OK;
  }
  alloy_error_t get_text(char *buf, size_t len) override {
    const char *txt = gtk_button_get_label(GTK_BUTTON(m_widget));
    if (!txt) return ALLOY_ERROR_PLATFORM;
    strncpy(buf, txt, len);
    return ALLOY_OK;
  }
  alloy_error_t set_checked(bool v) override {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_widget), v);
    return ALLOY_OK;
  }
  bool get_checked() override { return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_widget)); }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override { gtk_widget_set_sensitive(m_widget, v); return ALLOY_OK; }
  bool get_enabled() override { return gtk_widget_get_sensitive(m_widget); }
  alloy_error_t set_visible(bool v) override { if (v) gtk_widget_show(m_widget); else gtk_widget_hide(m_widget); return ALLOY_OK; }
  bool get_visible() override { return gtk_widget_get_visible(m_widget); }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }

private:
  GtkWidget *m_widget;
};
}
#endif
#endif
