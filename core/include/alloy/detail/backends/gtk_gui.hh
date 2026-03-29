#ifndef ALLOY_GTK_GUI_HH
#define ALLOY_GTK_GUI_HH

#include "../component_base.hh"
#include <gtk/gtk.h>

namespace alloy::detail {

class gtk_component : public component_base {
public:
    gtk_component(GtkWidget* widget, bool is_container = false)
        : component_base(is_container), m_widget(widget) {}

    virtual ~gtk_component() {
        if (m_widget) gtk_widget_destroy(m_widget);
    }

    alloy_error_t set_text(std::string_view text) override {
        if (GTK_IS_BUTTON(m_widget)) {
            gtk_button_set_label(GTK_BUTTON(m_widget), text.data());
        } else if (GTK_IS_ENTRY(m_widget)) {
            gtk_entry_set_text(GTK_ENTRY(m_widget), text.data());
        } else if (GTK_IS_LABEL(m_widget)) {
            gtk_label_set_text(GTK_LABEL(m_widget), text.data());
        }
        return ALLOY_OK;
    }

    alloy_error_t get_text(char *buf, size_t len) override {
        return ALLOY_ERROR_NOT_SUPPORTED;
    }

    alloy_error_t set_checked(bool v) override {
        if (GTK_IS_TOGGLE_BUTTON(m_widget)) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_widget), v);
        }
        return ALLOY_OK;
    }

    bool get_checked() override {
        return GTK_IS_TOGGLE_BUTTON(m_widget) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_widget));
    }

    alloy_error_t set_value(double v) override {
        if (GTK_IS_RANGE(m_widget)) {
            gtk_range_set_value(GTK_RANGE(m_widget), v);
        }
        return ALLOY_OK;
    }

    double get_value() override {
        return GTK_IS_RANGE(m_widget) ? gtk_range_get_value(GTK_RANGE(m_widget)) : 0.0;
    }

    alloy_error_t set_enabled(bool v) override {
        gtk_widget_set_sensitive(m_widget, v);
        return ALLOY_OK;
    }

    bool get_enabled() override {
        return gtk_widget_is_sensitive(m_widget);
    }

    alloy_error_t set_visible(bool v) override {
        if (v) gtk_widget_show(m_widget); else gtk_widget_hide(m_widget);
        return ALLOY_OK;
    }

    bool get_visible() override {
        return gtk_widget_get_visible(m_widget);
    }

    alloy_error_t set_style(const alloy_style_t &s) override {
        return ALLOY_OK;
    }

    void *native_handle() override { return m_widget; }

protected:
    GtkWidget* m_widget;
};

}

#endif
