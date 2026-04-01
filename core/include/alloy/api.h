/*
 * AlloyScript Runtime - CC0 Unlicense Public Domain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ALLOY_API_H
#define ALLOY_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ALLOY_API_SHARED)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(ALLOY_API_BUILD)
#define ALLOY_API __declspec(dllexport)
#else
#define ALLOY_API __declspec(dllimport)
#endif
#else
#define ALLOY_API __attribute__((visibility("default")))
#endif
#else
#define ALLOY_API
#endif

// ── Types ──────────────────────────────────────────────────────────────────

typedef void *alloy_component_t;   // opaque component handle
typedef void *alloy_signal_t;      // opaque signal handle
typedef void *alloy_computed_t;    // opaque computed signal handle
typedef void *alloy_effect_t;      // opaque effect handle

typedef enum {
  ALLOY_OK                    = 0,
  ALLOY_ERROR_INVALID_ARGUMENT,
  ALLOY_ERROR_INVALID_STATE,
  ALLOY_ERROR_PLATFORM,
  ALLOY_ERROR_BUFFER_TOO_SMALL,
  ALLOY_ERROR_NOT_SUPPORTED,
} alloy_error_t;

typedef enum {
  ALLOY_EVENT_CLICK  = 0,
  ALLOY_EVENT_CHANGE,
  ALLOY_EVENT_CLOSE,
  ALLOY_EVENT_FOCUS,
  ALLOY_EVENT_BLUR,
} alloy_event_type_t;

typedef enum {
  ALLOY_PROP_TEXT       = 0,
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
  unsigned int background;   // RGBA packed
  unsigned int foreground;   // RGBA packed
  float        font_size;    // points; 0 = inherit
  const char  *font_family;  // NULL = inherit
  float        border_radius;// points
  float        opacity;      // 0.0–1.0
} alloy_style_t;

// ── Error ──────────────────────────────────────────────────────────────────

ALLOY_API const char *alloy_error_message(alloy_error_t err);

// ── Signal system ──────────────────────────────────────────────────────────

ALLOY_API alloy_signal_t  alloy_signal_create_str(const char *initial);
ALLOY_API alloy_signal_t  alloy_signal_create_double(double initial);
ALLOY_API alloy_signal_t  alloy_signal_create_int(int initial);
ALLOY_API alloy_signal_t  alloy_signal_create_bool(int initial);

ALLOY_API alloy_error_t   alloy_signal_set_str(alloy_signal_t s, const char *v);
ALLOY_API alloy_error_t   alloy_signal_set_double(alloy_signal_t s, double v);
ALLOY_API alloy_error_t   alloy_signal_set_int(alloy_signal_t s, int v);
ALLOY_API alloy_error_t   alloy_signal_set_bool(alloy_signal_t s, int v);

ALLOY_API const char     *alloy_signal_get_str(alloy_signal_t s);
ALLOY_API double          alloy_signal_get_double(alloy_signal_t s);
ALLOY_API int             alloy_signal_get_int(alloy_signal_t s);
ALLOY_API int             alloy_signal_get_bool(alloy_signal_t s);

ALLOY_API alloy_computed_t alloy_computed_create(
    alloy_signal_t *deps, size_t dep_count,
    void (*compute)(alloy_signal_t *deps, size_t dep_count, void *out, void *userdata),
    void *userdata);

ALLOY_API alloy_effect_t  alloy_effect_create(
    alloy_signal_t *deps, size_t dep_count,
    void (*run)(void *userdata), void *userdata);

ALLOY_API alloy_error_t   alloy_signal_destroy(alloy_signal_t s);
ALLOY_API alloy_error_t   alloy_computed_destroy(alloy_computed_t c);
ALLOY_API alloy_error_t   alloy_effect_destroy(alloy_effect_t e);

// ── Property binding ───────────────────────────────────────────────────────

ALLOY_API alloy_error_t   alloy_bind_property(alloy_component_t component,
                                               alloy_prop_id_t   property,
                                               alloy_signal_t    signal);
ALLOY_API alloy_error_t   alloy_unbind_property(alloy_component_t component,
                                                 alloy_prop_id_t   property);

// ── Component lifecycle ────────────────────────────────────────────────────

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
ALLOY_API alloy_component_t alloy_create_loadingspinner(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_progressbar(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_listview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_treeview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_tabview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_webview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_vstack(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_hstack(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_scrollview(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_menu(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_menubar(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_toolbar(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_statusbar(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_splitter(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_dialog(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_filedialog(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_colorpicker(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_datepicker(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_timepicker(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_tooltip(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_divider(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_image(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_icon(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_separator(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_groupbox(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_accordion(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_popover(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_contextmenu(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_switch(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_badge(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_chip(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_card(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_link(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_rating(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_richtexteditor(alloy_component_t parent);
ALLOY_API alloy_component_t alloy_create_codeeditor(alloy_component_t parent);

ALLOY_API alloy_error_t     alloy_destroy(alloy_component_t handle);

// ── Property getters/setters ───────────────────────────────────────────────

ALLOY_API alloy_error_t alloy_set_text(alloy_component_t h, const char *text);
ALLOY_API alloy_error_t alloy_get_text(alloy_component_t h,
                                        char *buf, size_t buf_len);
ALLOY_API alloy_error_t alloy_set_checked(alloy_component_t h, int checked);
ALLOY_API int           alloy_get_checked(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_value(alloy_component_t h, double value);
ALLOY_API double        alloy_get_value(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_enabled(alloy_component_t h, int enabled);
ALLOY_API int           alloy_get_enabled(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_visible(alloy_component_t h, int visible);
ALLOY_API int           alloy_get_visible(alloy_component_t h);
ALLOY_API alloy_error_t alloy_set_style(alloy_component_t h,
                                         const alloy_style_t *style);

// ── Layout ─────────────────────────────────────────────────────────────────

ALLOY_API alloy_error_t alloy_add_child(alloy_component_t container,
                                         alloy_component_t child);
ALLOY_API alloy_error_t alloy_set_flex(alloy_component_t h, float flex);
ALLOY_API alloy_error_t alloy_set_padding(alloy_component_t h,
                                           float top, float right,
                                           float bottom, float left);
ALLOY_API alloy_error_t alloy_set_margin(alloy_component_t h,
                                          float top, float right,
                                          float bottom, float left);
ALLOY_API alloy_error_t alloy_layout(alloy_component_t window);

// ── Events ─────────────────────────────────────────────────────────────────

ALLOY_API alloy_error_t alloy_set_event_callback(alloy_component_t handle,
                                                   alloy_event_type_t event,
                                                   alloy_event_cb_t callback,
                                                   void *userdata);

// ── Event loop ─────────────────────────────────────────────────────────────

ALLOY_API alloy_error_t alloy_run(alloy_component_t window);
ALLOY_API alloy_error_t alloy_terminate(alloy_component_t window);
ALLOY_API alloy_error_t alloy_dispatch(alloy_component_t window,
                                        void (*fn)(void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif // ALLOY_API_H
