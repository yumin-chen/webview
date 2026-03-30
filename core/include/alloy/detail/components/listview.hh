#ifndef ALLOY_DETAIL_COMPONENTS_LISTVIEW_HH
#define ALLOY_DETAIL_COMPONENTS_LISTVIEW_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>
namespace alloy::detail {
class gtk_listview : public component_base {
public:
  explicit gtk_listview(component_base *parent) : component_base(false) {
    m_store = gtk_list_store_new(1, G_TYPE_STRING);
    m_widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_store));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Item", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_widget), column);
    m_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(m_scrolled_window), m_widget);
    g_object_ref_sink(m_scrolled_window);
  }
  ~gtk_listview() { g_object_unref(m_scrolled_window); }
  void *native_handle() override { return m_scrolled_window; }
  alloy_error_t set_text(std::string_view text) override {
    GtkTreeIter iter;
    gtk_list_store_append(m_store, &iter);
    gtk_list_store_set(m_store, &iter, 0, text.data(), -1);
    return ALLOY_OK;
  }
  alloy_error_t get_text(char *buf, size_t len) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override { gtk_widget_set_sensitive(m_widget, v); return ALLOY_OK; }
  bool get_enabled() override { return gtk_widget_get_sensitive(m_widget); }
  alloy_error_t set_visible(bool v) override { if (v) gtk_widget_show(m_scrolled_window); else gtk_widget_hide(m_scrolled_window); return ALLOY_OK; }
  bool get_visible() override { return gtk_widget_get_visible(m_scrolled_window); }
  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }

private:
  GtkWidget *m_widget;
  GtkWidget *m_scrolled_window;
  GtkListStore *m_store;
};
}
#endif
#endif
