#ifndef ALLOY_DETAIL_COMPONENTS_EXTRAS_2_HH
#define ALLOY_DETAIL_COMPONENTS_EXTRAS_2_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {

class gtk_rating : public component_base {
public:
  explicit gtk_rating(component_base *parent) : component_base(false) {
    m_widget = gtk_level_bar_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_rating() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { gtk_level_bar_set_value(GTK_LEVEL_BAR(m_widget), v); return ALLOY_OK; }
  double get_value() override { return gtk_level_bar_get_value(GTK_LEVEL_BAR(m_widget)); }
  alloy_error_t set_enabled(bool v) override { return ALLOY_OK; }
  bool get_enabled() override { return true; }
  alloy_error_t set_visible(bool v) override { return ALLOY_OK; }
  bool get_visible() override { return true; }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }
private:
  GtkWidget *m_widget;
};

class gtk_separator : public component_base {
public:
  explicit gtk_separator(component_base *parent) : component_base(false) {
    m_widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_ref_sink(m_widget);
  }
  ~gtk_separator() { g_object_unref(m_widget); }
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
