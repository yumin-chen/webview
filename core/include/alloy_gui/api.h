#ifndef ALLOY_GUI_API_H
#define ALLOY_GUI_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ALLOY_OK = 0,
    ALLOY_ERROR_INVALID_ARGUMENT = 1,
    ALLOY_ERROR_INVALID_STATE = 2,
    ALLOY_ERROR_PLATFORM = 3,
    ALLOY_ERROR_BUFFER_TOO_SMALL = 4,
    ALLOY_ERROR_NOT_SUPPORTED = 5
} alloy_error_t;

typedef enum {
    ALLOY_EVENT_CLICK,
    ALLOY_EVENT_CHANGE,
    ALLOY_EVENT_CLOSE
} alloy_event_type_t;

typedef void* alloy_component_t;
typedef void (*alloy_event_cb_t)(alloy_component_t handle, void *userdata);

typedef struct {
    float bg_r, bg_g, bg_b, bg_a;
    float fg_r, fg_g, fg_b, fg_a;
    float font_size;
    const char *font_family;
    float border_radius;
    float opacity;
} alloy_style_t;

// Lifecycle
alloy_component_t alloy_create_window(const char *title, int width, int height);
alloy_component_t alloy_create_button(alloy_component_t parent);
alloy_component_t alloy_create_textfield(alloy_component_t parent);
alloy_component_t alloy_create_vstack(alloy_component_t parent);
alloy_component_t alloy_create_hstack(alloy_component_t parent);
alloy_error_t alloy_destroy(alloy_component_t handle);

// Properties
alloy_error_t alloy_set_text(alloy_component_t handle, const char *text);
int alloy_get_text(alloy_component_t handle, char *buf, size_t buf_len);
alloy_error_t alloy_set_enabled(alloy_component_t handle, int enabled);
alloy_error_t alloy_set_visible(alloy_component_t handle, int visible);

// Layout
alloy_error_t alloy_add_child(alloy_component_t container, alloy_component_t child);
alloy_error_t alloy_layout(alloy_component_t window);
alloy_error_t alloy_set_flex(alloy_component_t handle, float flex);

// Events
alloy_error_t alloy_set_event_callback(alloy_component_t handle, alloy_event_type_t event, alloy_event_cb_t callback, void *userdata);

// Execution
alloy_error_t alloy_run(alloy_component_t window);
alloy_error_t alloy_terminate(alloy_component_t window);
alloy_error_t alloy_dispatch(alloy_component_t window, void (*fn)(void *), void *arg);

const char* alloy_error_message(alloy_error_t err);

#ifdef __cplusplus
}
#endif

#endif // ALLOY_GUI_API_H
