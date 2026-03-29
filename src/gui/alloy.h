/*
 * alloy:gui C API
 */

#ifndef ALLOY_GUI_H
#define ALLOY_GUI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Types ---

typedef void* alloy_component_t;

typedef enum {
    ALLOY_OK = 0,
    ALLOY_ERROR_INVALID_ARGUMENT,
    ALLOY_ERROR_INVALID_STATE,
    ALLOY_ERROR_PLATFORM,
    ALLOY_ERROR_BUFFER_TOO_SMALL,
    ALLOY_ERROR_NOT_SUPPORTED
} alloy_error_t;

typedef enum {
    ALLOY_EVENT_CLICK,
    ALLOY_EVENT_CHANGE,
    ALLOY_EVENT_CLOSE,
    ALLOY_EVENT_RESIZE,
    ALLOY_EVENT_MOVE,
    ALLOY_EVENT_FOCUS,
    ALLOY_EVENT_BLUR
} alloy_event_type_t;

typedef struct {
    float r, g, b, a;
} alloy_color_t;

typedef struct {
    alloy_color_t background_color;
    alloy_color_t foreground_color;
    float font_size;
    const char *font_family;
    float border_radius;
    float opacity;
} alloy_style_t;

typedef void (*alloy_event_cb_t)(alloy_component_t component, void *userdata);

// --- Lifecycle & System ---

alloy_error_t alloy_create_window(const char *title, int width, int height, alloy_component_t *out_window);
alloy_error_t alloy_destroy(alloy_component_t component);
alloy_error_t alloy_run(alloy_component_t window);
alloy_error_t alloy_terminate(alloy_component_t window);
alloy_error_t alloy_dispatch(alloy_component_t window, void (*fn)(void *), void *arg);

// --- Component Creation (Input) ---
alloy_error_t alloy_create_button(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_textfield(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_textarea(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_checkbox(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_radiobutton(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_combobox(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_slider(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_spinner(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_datepicker(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_timepicker(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_colorpicker(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_switch(alloy_component_t parent, alloy_component_t *out);

// --- Component Creation (Display) ---
alloy_error_t alloy_create_label(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_image(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_icon(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_progressbar(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_tooltip(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_badge(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_card(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_divider(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_richtexteditor(alloy_component_t parent, alloy_component_t *out);

// --- Component Creation (Selection) ---
alloy_error_t alloy_create_listview(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_treeview(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_tabview(alloy_component_t parent, alloy_component_t *out);

// --- Component Creation (Layout) ---
alloy_error_t alloy_create_vstack(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_hstack(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_scrollview(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_groupbox(alloy_component_t parent, alloy_component_t *out);

// --- Component Creation (Navigation) ---
alloy_error_t alloy_create_menu(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_menubar(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_toolbar(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_contextmenu(alloy_component_t parent, alloy_component_t *out);

// --- Component Creation (Dialog) ---
alloy_error_t alloy_create_dialog(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_filedialog(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_popover(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_statusbar(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_splitter(alloy_component_t parent, alloy_component_t *out);

// --- Component Creation (Additional) ---
alloy_error_t alloy_create_webview(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_link(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_chip(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_rating(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_accordion(alloy_component_t parent, alloy_component_t *out);
alloy_error_t alloy_create_codeeditor(alloy_component_t parent, alloy_component_t *out);

// --- Properties ---
alloy_error_t alloy_set_text(alloy_component_t component, const char *text);
int alloy_get_text(alloy_component_t component, char *buf, size_t buf_len);
alloy_error_t alloy_set_checked(alloy_component_t component, int checked);
int alloy_get_checked(alloy_component_t component);
alloy_error_t alloy_set_value(alloy_component_t component, double value);
double alloy_get_value(alloy_component_t component);
alloy_error_t alloy_set_enabled(alloy_component_t component, int enabled);
int alloy_get_enabled(alloy_component_t component);
alloy_error_t alloy_set_visible(alloy_component_t component, int visible);
int alloy_get_visible(alloy_component_t component);

// --- Events ---
alloy_error_t alloy_set_event_callback(alloy_component_t component, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata);

// --- Layout (Yoga) ---
alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child);
alloy_error_t alloy_layout(alloy_component_t window);
alloy_error_t alloy_set_flex(alloy_component_t component, float flex);
alloy_error_t alloy_set_padding(alloy_component_t component, float top, float right, float bottom, float left);
alloy_error_t alloy_set_margin(alloy_component_t component, float top, float right, float bottom, float left);

// --- Styling ---
alloy_error_t alloy_set_style(alloy_component_t component, const alloy_style_t *style);

// --- Utilities ---
const char* alloy_error_message(alloy_error_t error);

#ifdef __cplusplus
}
#endif

#endif // ALLOY_GUI_H
