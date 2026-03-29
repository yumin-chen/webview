#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

using namespace alloy::detail;

void signal_base::notify() {
    for (auto& sub : subscribers) {
        static_cast<Component*>(sub.first)->on_signal_changed(sub.second, value);
    }
}

void Component::on_signal_changed(alloy_prop_id_t prop, const signal_value& val) {
    if (prop == ALLOY_PROP_TEXT) {
        if (GTK_IS_WINDOW(widget)) gtk_window_set_title(GTK_WINDOW(widget), val.s.c_str());
        else if (GTK_IS_BUTTON(widget)) gtk_button_set_label(GTK_BUTTON(widget), val.s.c_str());
        else if (GTK_IS_LABEL(widget)) gtk_label_set_text(GTK_LABEL(widget), val.s.c_str());
        else if (GTK_IS_ENTRY(widget)) gtk_entry_set_text(GTK_ENTRY(widget), val.s.c_str());
    } else if (prop == ALLOY_PROP_CHECKED) {
        if (GTK_IS_CHECK_BUTTON(widget)) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), val.b);
        else if (GTK_IS_SWITCH(widget)) gtk_switch_set_active(GTK_SWITCH(widget), val.b);
    } else if (prop == ALLOY_PROP_VALUE) {
        if (GTK_IS_RANGE(widget)) gtk_range_set_value(GTK_RANGE(widget), val.d);
        else if (GTK_IS_PROGRESS_BAR(widget)) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget), val.d);
    }
}

extern "C" {

alloy_signal_t alloy_signal_create_str(const char *initial) {
    signal_base *s = new signal_base();
    s->value.type = signal_type::STR;
    s->value.s = initial ? initial : "";
    return (alloy_signal_t)s;
}

alloy_error_t alloy_signal_set_str(alloy_signal_t s, const char *v) {
    signal_base *sig = (signal_base*)s;
    sig->value.s = v ? v : "";
    sig->notify();
    return ALLOY_OK;
}

alloy_error_t alloy_bind_property(alloy_component_t component, alloy_prop_id_t property, alloy_signal_t signal) {
    Component *comp = (Component*)component;
    signal_base *sig = (signal_base*)signal;
    sig->subscribers.push_back({comp, property});
    comp->on_signal_changed(property, sig->value);
    return ALLOY_OK;
}

alloy_component_t alloy_create_window(const char *title, int width, int height) { return (alloy_component_t)GTKBackend::create_window(title, width, height); }
alloy_component_t alloy_create_button(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_button((Component*)parent); }
alloy_component_t alloy_create_textfield(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_textfield((Component*)parent); }
alloy_component_t alloy_create_textarea(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_textarea((Component*)parent); }
alloy_component_t alloy_create_label(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_label((Component*)parent); }
alloy_component_t alloy_create_checkbox(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_checkbox((Component*)parent); }
alloy_component_t alloy_create_radiobutton(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_radiobutton((Component*)parent); }
alloy_component_t alloy_create_combobox(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_combobox((Component*)parent); }
alloy_component_t alloy_create_slider(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_slider((Component*)parent); }
alloy_component_t alloy_create_spinner(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_spinner((Component*)parent); }
alloy_component_t alloy_create_progressbar(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_progressbar((Component*)parent); }
alloy_component_t alloy_create_tabview(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_tabview((Component*)parent); }
alloy_component_t alloy_create_listview(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_listview((Component*)parent); }
alloy_component_t alloy_create_treeview(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_treeview((Component*)parent); }
alloy_component_t alloy_create_webview(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_webview((Component*)parent); }
alloy_component_t alloy_create_vstack(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_vstack((Component*)parent); }
alloy_component_t alloy_create_hstack(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_hstack((Component*)parent); }
alloy_component_t alloy_create_scrollview(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_scrollview((Component*)parent); }
alloy_component_t alloy_create_switch(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_switch((Component*)parent); }
alloy_component_t alloy_create_separator(alloy_component_t parent) { return (alloy_component_t)GTKBackend::create_separator((Component*)parent); }

// Extra components from Reference Table
alloy_component_t alloy_create_image(alloy_component_t parent) { return new Component(gtk_image_new()); }
alloy_component_t alloy_create_icon(alloy_component_t parent) { return new Component(gtk_image_new()); }
alloy_component_t alloy_create_menubar(alloy_component_t parent) { return new Component(gtk_menu_bar_new()); }
alloy_component_t alloy_create_toolbar(alloy_component_t parent) { return new Component(gtk_toolbar_new()); }
alloy_component_t alloy_create_statusbar(alloy_component_t parent) { return new Component(gtk_status_bar_new()); }
alloy_component_t alloy_create_splitter(alloy_component_t parent) { return new Component(gtk_paned_new(GTK_ORIENTATION_HORIZONTAL)); }
alloy_component_t alloy_create_dialog(const char *title, int width, int height) { return new Component(gtk_dialog_new()); }
alloy_component_t alloy_create_filedialog(alloy_component_t parent) { return new Component(gtk_file_chooser_button_new("Select File", GTK_FILE_CHOOSER_ACTION_OPEN)); }
alloy_component_t alloy_create_colorpicker(alloy_component_t parent) { return new Component(gtk_color_button_new()); }
alloy_component_t alloy_create_datepicker(alloy_component_t parent) { return new Component(gtk_calendar_new()); }
alloy_component_t alloy_create_timepicker(alloy_component_t parent) { return new Component(gtk_spin_button_new_with_range(0, 23, 1)); }
alloy_component_t alloy_create_link(alloy_component_t parent) { return new Component(gtk_link_button_new("")); }
alloy_component_t alloy_create_chip(alloy_component_t parent) { return new Component(gtk_button_new()); }
alloy_component_t alloy_create_accordion(alloy_component_t parent) { return new Component(gtk_expander_new("")); }
alloy_component_t alloy_create_codeeditor(alloy_component_t parent) { return new Component(gtk_text_view_new()); }
alloy_component_t alloy_create_tooltip(alloy_component_t parent) { return new Component(gtk_label_new("")); }
alloy_component_t alloy_create_groupbox(alloy_component_t parent) { return new Component(gtk_frame_new("")); }
alloy_component_t alloy_create_popover(alloy_component_t parent) { return new Component(gtk_popover_new(parent ? ((Component*)parent)->widget : NULL)); }
alloy_component_t alloy_create_badge(alloy_component_t parent) { return new Component(gtk_label_new("")); }
alloy_component_t alloy_create_card(alloy_component_t parent) { return new Component(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)); }
alloy_component_t alloy_create_rating(alloy_component_t parent) { return new Component(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0)); }
alloy_component_t alloy_create_menu(alloy_component_t parent) { return new Component(gtk_menu_item_new()); }
alloy_component_t alloy_create_contextmenu(alloy_component_t parent) { return new Component(gtk_menu_new()); }
alloy_component_t alloy_create_divider(alloy_component_t parent) { return new Component(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)); }
alloy_component_t alloy_create_loading_indicator(alloy_component_t parent) { return new Component(gtk_spinner_new()); }
alloy_component_t alloy_create_richtexteditor(alloy_component_t parent) { return new Component(gtk_text_view_new()); }

alloy_error_t alloy_destroy(alloy_component_t handle) {
    if (!handle) return ALLOY_ERROR_INVALID_ARGUMENT;
    delete (Component*)handle;
    return ALLOY_OK;
}

alloy_error_t alloy_set_text(alloy_component_t handle, const char *text) {
    Component *comp = (Component*)handle;
    if (GTK_IS_WINDOW(comp->widget)) gtk_window_set_title(GTK_WINDOW(comp->widget), text);
    else if (GTK_IS_BUTTON(comp->widget)) gtk_button_set_label(GTK_BUTTON(comp->widget), text);
    else if (GTK_IS_LABEL(comp->widget)) gtk_label_set_text(GTK_LABEL(comp->widget), text);
    else if (GTK_IS_ENTRY(comp->widget)) gtk_entry_set_text(GTK_ENTRY(comp->widget), text);
    return ALLOY_OK;
}

alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata) {
    Component *comp = (Component*)handle;
    comp->callbacks[event] = {callback, userdata};
    return ALLOY_OK;
}

alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child) {
    Component *parent = (Component*)container;
    Component *comp = (Component*)child;
    if (GTK_IS_CONTAINER(parent->widget)) {
        gtk_container_add(GTK_CONTAINER(parent->widget), comp->widget);
        parent->children.push_back(comp);
        return ALLOY_OK;
    }
    return ALLOY_ERROR_INVALID_ARGUMENT;
}

alloy_error_t alloy_layout(alloy_component_t window) {
    Component *comp = (Component*)window;
    gtk_widget_show_all(comp->widget);
    return ALLOY_OK;
}

alloy_error_t alloy_run(alloy_component_t window) {
    gtk_main();
    return ALLOY_OK;
}

alloy_error_t alloy_terminate(alloy_component_t window) {
    gtk_main_quit();
    return ALLOY_OK;
}

const char* alloy_error_message(alloy_error_t err) {
    switch(err) {
        case ALLOY_OK: return "Success";
        case ALLOY_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case ALLOY_ERROR_INVALID_STATE: return "Invalid state";
        case ALLOY_ERROR_PLATFORM: return "Platform error";
        case ALLOY_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ALLOY_ERROR_NOT_SUPPORTED: return "Not supported";
        default: return "Unknown error";
    }
}

}
