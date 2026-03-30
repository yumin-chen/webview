#ifndef ALLOY_COMPONENTS_WINDOW_HH
#define ALLOY_COMPONENTS_WINDOW_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>

namespace alloy::detail {

class gtk_window : public component_base {
public:
  gtk_window(const char* title, int width, int height) : component_base(true) {
    m_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_widget), title);
    gtk_window_set_default_size(GTK_WINDOW(m_widget), width, height);
    gtk_widget_show(m_widget);
  }

  alloy_error_t set_text(std::string_view text) override {
    gtk_window_set_title(GTK_WINDOW(m_widget), text.data());
    return ALLOY_OK;
  }

  alloy_error_t get_text(char *buf, size_t len) override {
    const char* text = gtk_window_get_title(GTK_WINDOW(m_widget));
    if (strlen(text) >= len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
    strcpy(buf, text);
    return ALLOY_OK;
  }

  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
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
#endif

#endif // ALLOY_COMPONENTS_WINDOW_HH
