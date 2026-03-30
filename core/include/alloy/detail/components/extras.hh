#ifndef ALLOY_DETAIL_COMPONENTS_EXTRAS_HH
#define ALLOY_DETAIL_COMPONENTS_EXTRAS_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {

class gtk_badge : public component_base {
public:
  explicit gtk_badge(component_base *parent) : component_base(false) {
    m_widget = gtk_label_new("");
    g_object_ref_sink(m_widget);
  }
  ~gtk_badge() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { gtk_label_set_text(GTK_LABEL(m_widget), text.data()); return ALLOY_OK; }
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

class gtk_chip : public component_base {
public:
  explicit gtk_chip(component_base *parent) : component_base(false) {
    m_widget = gtk_button_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_chip() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { gtk_button_set_label(GTK_BUTTON(m_widget), text.data()); return ALLOY_OK; }
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

class gtk_card : public component_base {
public:
  explicit gtk_card(component_base *parent) : component_base(true) {
    m_widget = gtk_frame_new(NULL);
    g_object_ref_sink(m_widget);
  }
  ~gtk_card() { g_object_unref(m_widget); }
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
