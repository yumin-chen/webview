#ifndef ALLOY_DETAIL_COMPONENTS_SPINNER_HH
#define ALLOY_DETAIL_COMPONENTS_SPINNER_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {
class gtk_spinner : public component_base {
public:
  explicit gtk_spinner(component_base *parent) : component_base(false) {
    m_widget = gtk_spin_button_new_with_range(0, 100, 1);
    g_object_ref_sink(m_widget);
  }
  ~gtk_spinner() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_widget), v); return ALLOY_OK; }
  double get_value() override { return gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_widget)); }
  alloy_error_t set_enabled(bool v) override { gtk_widget_set_sensitive(m_widget, v); return ALLOY_OK; }
  bool get_enabled() override { return gtk_widget_get_sensitive(m_widget); }
  alloy_error_t set_visible(bool v) override { if (v) gtk_widget_show(m_widget); else gtk_widget_hide(m_widget); return ALLOY_OK; }
  bool get_visible() override { return gtk_widget_get_visible(m_widget); }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }

private:
  GtkWidget *m_widget;
};

class gtk_loadingspinner : public component_base {
public:
  explicit gtk_loadingspinner(component_base *parent) : component_base(false) {
    m_widget = gtk_spinner_new();
    g_object_ref_sink(m_widget);
    gtk_spinner_start(GTK_SPINNER(m_widget));
  }
  ~gtk_loadingspinner() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override {
    if (v) gtk_spinner_start(GTK_SPINNER(m_widget));
    else gtk_spinner_stop(GTK_SPINNER(m_widget));
    return ALLOY_OK;
  }
  bool get_checked() override { return true; }
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
