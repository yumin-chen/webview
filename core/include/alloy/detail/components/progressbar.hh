#ifndef ALLOY_DETAIL_COMPONENTS_PROGRESSBAR_HH
#define ALLOY_DETAIL_COMPONENTS_PROGRESSBAR_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {
class gtk_progressbar : public component_base {
public:
  explicit gtk_progressbar(component_base *parent) : component_base(false) {
    m_widget = gtk_progress_bar_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_progressbar() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override {
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_widget), text.data());
    return ALLOY_OK;
  }
  alloy_error_t get_text(char *buf, size_t len) override {
    const char *txt = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(m_widget));
    if (txt) strncpy(buf, txt, len);
    return ALLOY_OK;
  }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_widget), v);
    return ALLOY_OK;
  }
  double get_value() override { return gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(m_widget)); }
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
