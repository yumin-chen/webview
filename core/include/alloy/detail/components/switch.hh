#ifndef ALLOY_DETAIL_COMPONENTS_SWITCH_HH
#define ALLOY_DETAIL_COMPONENTS_SWITCH_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {
class gtk_switch : public component_base {
public:
  explicit gtk_switch(component_base *parent) : component_base(false) {
    m_widget = gtk_switch_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_switch() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override {
    gtk_switch_set_active(GTK_SWITCH(m_widget), v);
    return ALLOY_OK;
  }
  bool get_checked() override { return gtk_switch_get_active(GTK_SWITCH(m_widget)); }
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
