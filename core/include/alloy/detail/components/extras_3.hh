#ifndef ALLOY_DETAIL_COMPONENTS_EXTRAS_3_HH
#define ALLOY_DETAIL_COMPONENTS_EXTRAS_3_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {

class gtk_filedialog : public component_base {
public:
  explicit gtk_filedialog(component_base *parent) : component_base(true) {
    m_widget = gtk_file_chooser_dialog_new("Open File", parent ? GTK_WINDOW(parent->native_handle()) : NULL,
                                            GTK_FILE_CHOOSER_ACTION_OPEN,
                                            "_Cancel", GTK_RESPONSE_CANCEL,
                                            "_Open", GTK_RESPONSE_ACCEPT, NULL);
    g_object_ref_sink(m_widget);
  }
  ~gtk_filedialog() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override { gtk_window_set_title(GTK_WINDOW(m_widget), text.data()); return ALLOY_OK; }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override { return ALLOY_OK; }
  bool get_enabled() override { return true; }
  alloy_error_t set_visible(bool v) override { if (v) gtk_widget_show(m_widget); else gtk_widget_hide(m_widget); return ALLOY_OK; }
  bool get_visible() override { return gtk_widget_get_visible(m_widget); }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }
private:
  GtkWidget *m_widget;
};

class gtk_contextmenu : public component_base {
public:
  explicit gtk_contextmenu(component_base *parent) : component_base(true) {
    m_widget = gtk_menu_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_contextmenu() { g_object_unref(m_widget); }
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

class gtk_richtexteditor : public component_base {
public:
  explicit gtk_richtexteditor(component_base *parent) : component_base(false) {
    m_widget = gtk_text_view_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_richtexteditor() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_widget));
    gtk_text_buffer_set_text(buffer, text.data(), -1);
    return ALLOY_OK;
  }
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

class gtk_codeeditor : public component_base {
public:
  explicit gtk_codeeditor(component_base *parent) : component_base(false) {
    m_widget = gtk_text_view_new();
    g_object_ref_sink(m_widget);
  }
  ~gtk_codeeditor() { g_object_unref(m_widget); }
  void *native_handle() override { return m_widget; }
  alloy_error_t set_text(std::string_view text) override {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_widget));
    gtk_text_buffer_set_text(buffer, text.data(), -1);
    return ALLOY_OK;
  }
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
