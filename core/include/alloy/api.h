#ifndef ALLOY_API_H
#define ALLOY_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define ALLOY_API __declspec(dllexport)
#else
#define ALLOY_API __attribute__((visibility("default")))
#endif

typedef void *alloy_component_t;
typedef void *alloy_signal_t;
typedef void *alloy_computed_t;
typedef void *alloy_effect_t;

typedef enum {
  ALLOY_OK = 0,
  ALLOY_ERROR_INVALID_ARGUMENT,
  ALLOY_ERROR_INVALID_STATE,
  ALLOY_ERROR_PLATFORM,
  ALLOY_ERROR_BUFFER_TOO_SMALL,
  ALLOY_ERROR_NOT_SUPPORTED,
} alloy_error_t;

typedef enum {
  ALLOY_EVENT_CLICK = 0,
  ALLOY_EVENT_CHANGE,
  ALLOY_EVENT_CLOSE,
  ALLOY_EVENT_FOCUS,
  ALLOY_EVENT_BLUR,
} alloy_event_type_t;

typedef enum {
  ALLOY_PROP_TEXT = 0,
  ALLOY_PROP_CHECKED,
  ALLOY_PROP_VALUE,
  ALLOY_PROP_ENABLED,
  ALLOY_PROP_VISIBLE,
  ALLOY_PROP_LABEL,
} alloy_prop_id_t;

typedef void (*alloy_event_cb_t)(alloy_component_t handle,
                                 alloy_event_type_t event,
                                 void *userdata);

typedef struct {
  unsigned int background;
  unsigned int foreground;
  float font_size;
  const char *font_family;
  float border_radius;
  float opacity;
} alloy_style_t;

ALLOY_API const char *alloy_error_message(alloy_error_t err);

ALLOY_API alloy_signal_t alloy_signal_create_str(const char *initial);
ALLOY_API alloy_signal_t alloy_signal_create_double(double initial);
ALLOY_API alloy_signal_t alloy_signal_create_int(int initial);
ALLOY_API alloy_signal_t alloy_signal_create_bool(int initial);

ALLOY_API alloy_error_t alloy_signal_set_str(alloy_signal_t s, const char *v);
ALLOY_API alloy_error_t alloy_signal_set_double(alloy_signal_t s, double v);
ALLOY_API alloy_error_t alloy_signal_set_int(alloy_signal_t s, int v);
ALLOY_API alloy_error_t alloy_signal_set_bool(alloy_signal_t s, int v);

ALLOY_API const char *alloy_signal_get_str(alloy_signal_t s);
ALLOY_API double alloy_signal_get_double(alloy_signal_t s);
ALLOY_API int alloy_signal_get_int(alloy_signal_t s);
ALLOY_API int alloy_signal_get_bool(alloy_signal_t s);

ALLOY_API alloy_computed_t alloy_computed_create(
    alloy_signal_t *deps, size_t dep_count,
    void (*compute)(alloy_signal_t *deps, size_t dep_count, void *out, void *userdata),
    void *userdata);

ALLOY_API alloy_effect_t alloy_effect_create(
    alloy_signal_t *deps, size_t dep_count,
    void (*run)(void *userdata), void *userdata);

ALLOY_API alloy_error_t alloy_signal_destroy(alloy_signal_t s);
ALLOY_API alloy_error_t alloy_computed_destroy(alloy_computed_t c);
ALLOY_API alloy_error_t alloy_effect_destroy(alloy_effect_t e);

ALLOY_API alloy_error_t alloy_bind_property(alloy_component_t component,
                                            alloy_prop_id_t property,
                                            alloy_signal_t signal);
ALLOY_API alloy_error_t alloy_unbind_property(alloy_component_t component,
                                              alloy_prop_id_t property);

ALLOY_API alloy_component_t alloy_create_window(const char *title,
                                                 int width, int height);
ALLOY_API alloy_component_t alloy_create_button(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_textfield(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_textarea(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_label(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_checkbox(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_radiobutton(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_combobox(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_slider(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_spinner(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_progressbar(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_tabview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_listview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_treeview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_webview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_vstack(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_hstack(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_scrollview(alloy_component_t parent);

ALLOY_API alloy_error_t alloy_destroy(alloy_component_t handle);

ALLOY_API alloy_error_t alloy_set_text(alloy_component_t h, const char *text);
ALLOY_API alloy_error_t alloy_get_text(alloy_component_t h,
                                        char *buf, size_t buf_len);
ALLOY_API alloy_error_t alloy_set_checked(alloy_component_t h, int checked);
ALLOY_API int alloy_get_checked(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_value(alloy_component_t h, double value);
ALLOY_API double alloy_get_value(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled);
ALLOY_API int alloy_get_enabled(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_visible(alloy_component_t h, int visible);
ALLOY_API int alloy_get_visible(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_style(alloy_component_t h,
                                         const alloy_style_t *style);

ALLOY_API alloy_error_t alloy_add_child(alloy_component_t container,
                                         alloy_component_t child);
ALLOY_API alloy_error_t alloy_set_flex(alloy_component_t h, float flex);
ALLOY_API alloy_error_t alloy_set_padding(alloy_component_t h,
                                           float top, float right,
                                           float bottom, float left);
ALLOY_API alloy_error_t alloy_set_margin(alloy_component_t h,
                                          float top, float right,
                                          float bottom, float left);
ALLOY_API alloy_error_t alloy_set_width(alloy_component_t h, float width);
ALLOY_API alloy_error_t alloy_set_height(alloy_component_t h, float height);
ALLOY_API alloy_error_t alloy_layout(alloy_component_t window);

ALLOY_API alloy_error_t alloy_set_event_callback(alloy_component_t handle,
                                                 alloy_event_type_t event,
                                                 alloy_event_cb_t callback,
                                                 void *userdata);

ALLOY_API alloy_error_t alloy_run(alloy_component_t window);
ALLOY_API alloy_error_t alloy_terminate(alloy_component_t window);
ALLOY_API alloy_error_t alloy_dispatch(alloy_component_t window,
                                        void (*fn)(void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif
