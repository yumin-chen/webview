#ifndef ALLOY_BACKENDS_GTK_GUI_HH
#define ALLOY_BACKENDS_GTK_GUI_HH

#include "../component_base.hh"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string>
#include <vector>

namespace alloy::detail {

class gtk_component : public component_base {
public:
    explicit gtk_component(GtkWidget* widget, bool is_container = false)
        : component_base(is_container), m_widget(widget) {
        if (m_widget) g_object_ref_sink(m_widget);
    }

    virtual ~gtk_component() {
        if (m_widget) g_object_unref(m_widget);
    }

    alloy_error_t set_text(std::string_view text) override {
        if (GTK_IS_LABEL(m_widget)) gtk_label_set_text(GTK_LABEL(m_widget), text.data());
        else if (GTK_IS_BUTTON(m_widget)) gtk_button_set_label(GTK_BUTTON(m_widget), text.data());
        else if (GTK_IS_ENTRY(m_widget)) gtk_entry_set_text(GTK_ENTRY(m_widget), text.data());
        else if (GTK_IS_WINDOW(m_widget)) gtk_window_set_title(GTK_WINDOW(m_widget), text.data());
        else if (GTK_IS_TEXT_VIEW(m_widget)) {
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_widget));
            gtk_text_buffer_set_text(buffer, text.data(), -1);
        }
        return ALLOY_OK;
    }

    alloy_error_t get_text(char *buf, size_t len) override {
        const char* text = "";
        std::string s;
        if (GTK_IS_LABEL(m_widget)) text = gtk_label_get_text(GTK_LABEL(m_widget));
        else if (GTK_IS_ENTRY(m_widget)) text = gtk_entry_get_text(GTK_ENTRY(m_widget));
        else if (GTK_IS_TEXT_VIEW(m_widget)) {
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_widget));
            GtkTextIter start, end;
            gtk_text_buffer_get_bounds(buffer, &start, &end);
            char* tmp = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
            s = tmp;
            g_free(tmp);
            text = s.c_str();
        }

        if (strlen(text) >= len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
        strcpy(buf, text);
        return ALLOY_OK;
    }

    alloy_error_t set_checked(bool v) override {
        if (GTK_IS_TOGGLE_BUTTON(m_widget)) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_widget), v);
        else if (GTK_IS_SWITCH(m_widget)) gtk_switch_set_active(GTK_SWITCH(m_widget), v);
        return ALLOY_OK;
    }

    bool get_checked() override {
        if (GTK_IS_TOGGLE_BUTTON(m_widget)) return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_widget));
        else if (GTK_IS_SWITCH(m_widget)) return gtk_switch_get_active(GTK_SWITCH(m_widget));
        return false;
    }

    alloy_error_t set_value(double v) override {
        if (GTK_IS_RANGE(m_widget)) gtk_range_set_value(GTK_RANGE(m_widget), v * 100.0);
        else if (GTK_IS_PROGRESS_BAR(m_widget)) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_widget), v);
        return ALLOY_OK;
    }

    double get_value() override {
        if (GTK_IS_RANGE(m_widget)) return gtk_range_get_value(GTK_RANGE(m_widget)) / 100.0;
        else if (GTK_IS_PROGRESS_BAR(m_widget)) return gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(m_widget));
        return 0.0;
    }

    alloy_error_t set_enabled(bool v) override {
        gtk_widget_set_sensitive(m_widget, v);
        return ALLOY_OK;
    }

    bool get_enabled() override {
        return gtk_widget_get_sensitive(m_widget);
    }

    alloy_error_t set_visible(bool v) override {
        if (v) gtk_widget_show(m_widget);
        else gtk_widget_hide(m_widget);
        return ALLOY_OK;
    }

    bool get_visible() override {
        return gtk_widget_get_visible(m_widget);
    }

    alloy_error_t set_style(const alloy_style_t &s) override {
        if (s.background != 0) {
            GdkRGBA color;
            color.red = ((s.background >> 24) & 0xFF) / 255.0;
            color.green = ((s.background >> 16) & 0xFF) / 255.0;
            color.blue = ((s.background >> 8) & 0xFF) / 255.0;
            color.alpha = (s.background & 0xFF) / 255.0;
            gtk_widget_override_background_color(m_widget, GTK_STATE_FLAG_NORMAL, &color);
        }
        return ALLOY_OK;
    }

    void *native_handle() override { return m_widget; }

    void apply_native_layout(float x, float y, float w, float h) override {
        GtkWidget* parent = gtk_widget_get_parent(m_widget);
        if (parent && GTK_IS_FIXED(parent)) {
            gtk_fixed_move(GTK_FIXED(parent), m_widget, (gint)x, (gint)y);
        }
        gtk_widget_set_size_request(m_widget, (gint)w, (gint)h);
    }

protected:
    GtkWidget* m_widget;
};

class gtk_window : public gtk_component {
public:
    gtk_window(const char* title, int w, int h)
        : gtk_component(gtk_window_new(GTK_WINDOW_TOPLEVEL), true) {
        gtk_window_set_title(GTK_WINDOW(m_widget), title);
        gtk_window_set_default_size(GTK_WINDOW(m_widget), w, h);

        m_fixed = gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(m_widget), m_fixed);
        gtk_widget_show(m_fixed);

        g_signal_connect(m_widget, "destroy", G_CALLBACK(+[](GtkWidget*, gpointer arg) {
            auto* self = static_cast<gtk_window*>(arg);
            self->fire_event(ALLOY_EVENT_CLOSE);
        }), this);
    }

    void add_child(component_base* child) override {
        component_base::add_child(child);
        GtkWidget* child_widget = static_cast<GtkWidget*>(child->native_handle());
        // For Window, fixed container might be replaced by a proper layout root
        gtk_fixed_put(GTK_FIXED(m_fixed), child_widget, 0, 0);
        gtk_widget_show(child_widget);
    }

private:
    GtkWidget* m_fixed;
};

class gtk_container : public gtk_component {
public:
    explicit gtk_container(GtkWidget* widget) : gtk_component(widget, true) {}

    void add_child(component_base* child) override {
        component_base::add_child(child);
        GtkWidget* child_widget = static_cast<GtkWidget*>(child->native_handle());
        if (GTK_IS_CONTAINER(m_widget)) {
            gtk_container_add(GTK_CONTAINER(m_widget), child_widget);
            gtk_widget_show(child_widget);
        }
    }
};

} // namespace alloy::detail

#endif // ALLOY_BACKENDS_GTK_GUI_HH
