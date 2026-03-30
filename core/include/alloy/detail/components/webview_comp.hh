#ifndef ALLOY_DETAIL_COMPONENTS_WEBVIEW_COMP_HH
#define ALLOY_DETAIL_COMPONENTS_WEBVIEW_COMP_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <webkit2/webkit2.h>
namespace alloy::detail {
class gtk_webview_comp : public component_base {
public:
  explicit gtk_webview_comp(component_base *parent) : component_base(false) {
    m_widget = webkit_web_view_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_webview_comp() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(m_widget), text.data());
    return ALLOY_OK;
  }
  alloy_error_t get_text(char *buf, size_t len) override {
    const char *uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(m_widget));
    if (uri) strncpy(buf, uri, len);
    return ALLOY_OK;
  }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
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
