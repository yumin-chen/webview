#ifndef ALLOY_DETAIL_BACKENDS_GTK_GUI_HH
#define ALLOY_DETAIL_BACKENDS_GTK_GUI_HH

#include "../component_base.hh"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace alloy::detail {

class gtk_component : public component_base {
public:
  gtk_component(GtkWidget *widget, bool is_container = false)
      : component_base(is_container), m_widget(widget) {
    g_object_ref_sink(m_widget);
  }
  ~gtk_component() { g_object_unref(m_widget); }

  void *native_handle() override { return m_widget; }

  alloy_error_t set_text(const std::string &text) override {
    if (GTK_IS_WINDOW(m_widget)) {
      gtk_window_set_title(GTK_WINDOW(m_widget), text.c_str());
    } else if (GTK_IS_BUTTON(m_widget)) {
      gtk_button_set_label(GTK_BUTTON(m_widget), text.c_str());
    } else if (GTK_IS_LABEL(m_widget)) {
      gtk_label_set_text(GTK_LABEL(m_widget), text.c_str());
    } else if (GTK_IS_ENTRY(m_widget)) {
      gtk_entry_set_text(GTK_ENTRY(m_widget), text.c_str());
    } else if (GTK_IS_LINK_BUTTON(m_widget)) {
      gtk_button_set_label(GTK_BUTTON(m_widget), text.c_str());
    } else if (GTK_IS_TEXT_VIEW(m_widget)) {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_widget));
      gtk_text_buffer_set_text(buffer, text.c_str(), -1);
    }
    return ALLOY_OK;
  }

  alloy_error_t get_text(char *buf, size_t len) override {
    const char *text = "";
    std::string temp;
    if (GTK_IS_WINDOW(m_widget)) text = gtk_window_get_title(GTK_WINDOW(m_widget));
    else if (GTK_IS_BUTTON(m_widget)) text = gtk_button_get_label(GTK_BUTTON(m_widget));
    else if (GTK_IS_LABEL(m_widget)) text = gtk_label_get_text(GTK_LABEL(m_widget));
    else if (GTK_IS_ENTRY(m_widget)) text = gtk_entry_get_text(GTK_ENTRY(m_widget));
    else if (GTK_IS_TEXT_VIEW(m_widget)) {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_widget));
      GtkTextIter start, end;
      gtk_text_buffer_get_bounds(buffer, &start, &end);
      char *raw = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
      temp = raw;
      g_free(raw);
      text = temp.c_str();
    }

    if (!text) text = "";
    size_t n = strlen(text);
    if (n >= len) {
      if (len > 0) { memcpy(buf, text, len - 1); buf[len - 1] = '\0'; }
      return ALLOY_ERROR_BUFFER_TOO_SMALL;
    }
    memcpy(buf, text, n + 1);
    return ALLOY_OK;
  }

  alloy_error_t set_checked(bool v) override {
    if (GTK_IS_TOGGLE_BUTTON(m_widget)) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_widget), v);
    } else if (GTK_IS_SWITCH(m_widget)) {
      gtk_switch_set_active(GTK_SWITCH(m_widget), v);
    }
    return ALLOY_OK;
  }
  bool get_checked() override {
    if (GTK_IS_TOGGLE_BUTTON(m_widget)) return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_widget));
    if (GTK_IS_SWITCH(m_widget)) return gtk_switch_get_active(GTK_SWITCH(m_widget));
    return false;
  }

  alloy_error_t set_value(double v) override {
    if (GTK_IS_PROGRESS_BAR(m_widget)) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_widget), v);
    else if (GTK_IS_RANGE(m_widget)) gtk_range_set_value(GTK_RANGE(m_widget), v);
    return ALLOY_OK;
  }
  double get_value() override {
    if (GTK_IS_PROGRESS_BAR(m_widget)) return gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(m_widget));
    else if (GTK_IS_RANGE(m_widget)) return gtk_range_get_value(GTK_RANGE(m_widget));
    return 0;
  }

  alloy_error_t set_enabled(bool v) override {
    gtk_widget_set_sensitive(m_widget, v);
    return ALLOY_OK;
  }
  bool get_enabled() override { return gtk_widget_get_sensitive(m_widget); }

  alloy_error_t set_visible(bool v) override {
    if (v) gtk_widget_show(m_widget); else gtk_widget_hide(m_widget);
    return ALLOY_OK;
  }
  bool get_visible() override { return gtk_widget_get_visible(m_widget); }

  alloy_error_t set_style(const alloy_style_t &s) override {
    (void)s;
    return ALLOY_OK;
  }

protected:
  GtkWidget *m_widget;
};

class gtk_window : public gtk_component {
public:
  gtk_window(const char *title, int w, int h)
      : gtk_component(gtk_window_new(GTK_WINDOW_TOPLEVEL), true) {
    gtk_window_set_title(GTK_WINDOW(m_widget), title);
    gtk_window_set_default_size(GTK_WINDOW(m_widget), w, h);
    g_signal_connect(m_widget, "destroy", G_CALLBACK(+[](GtkWidget *, gpointer) { gtk_main_quit(); }), NULL);
  }
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_BACKENDS_GTK_GUI_HH
