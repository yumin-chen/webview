#ifndef ALLOY_DETAIL_COMPONENTS_MISC_HH
#define ALLOY_DETAIL_COMPONENTS_MISC_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {

class gtk_splitter : public component_base {
public:
  explicit gtk_splitter(component_base *parent) : component_base(true) {
    m_widget = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_ref_sink(m_widget);
  }
  ~gtk_splitter() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
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

class gtk_tooltip : public component_base {
public:
  explicit gtk_tooltip(component_base *parent) : component_base(false) {}
  void *native_handle() override { return nullptr; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_OK; }
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
};

class gtk_divider : public component_base {
public:
  explicit gtk_divider(component_base *parent) : component_base(false) {
    m_widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_ref_sink(m_widget);
  }
  ~gtk_divider() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override { return ALLOY_OK; }
  bool get_enabled() override { return true; }
  alloy_error_t set_visible(bool v) override { return ALLOY_OK; }
  bool get_visible() override { return true; }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }
private:
  GtkWidget *m_widget;
};

}
#endif
#endif
