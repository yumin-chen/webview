#include "alloy/api.h"
#include <stdio.h>

void on_open_click(alloy_component_t h, alloy_event_type_t e, void* arg) {
    alloy_component_t win = (alloy_component_t)arg;
    const char* path = alloy_dialog_file_open(win, "Select a file");
    if (path) printf("Selected file: %s\n", path);
}

void on_color_click(alloy_component_t h, alloy_event_type_t e, void* arg) {
    alloy_component_t win = (alloy_component_t)arg;
    const char* color = alloy_dialog_color_picker(win, "Select a color");
    if (color) printf("Selected color: %s\n", color);
}

int main() {
    alloy_component_t win = alloy_create_window("Alloy Native Pro Demo", 800, 600);
    alloy_component_t vs = alloy_create_vstack(win);

    // Menu
    alloy_component_t mb = alloy_create_menubar(vs);
    alloy_component_t m_file = alloy_create_menuitem(mb);
    alloy_set_text(m_file, "File");
    alloy_component_t file_menu = alloy_create_menu(m_file);
    alloy_component_t mi_open = alloy_create_menuitem(file_menu);
    alloy_set_text(mi_open, "Open...");
    alloy_component_t mi_exit = alloy_create_menuitem(file_menu);
    alloy_set_text(mi_exit, "Exit");

    alloy_component_t lbl = alloy_create_label(vs);
    alloy_set_text(lbl, "Alloy Native Professional Dashboard");

    alloy_component_t hs = alloy_create_hstack(vs);
    alloy_component_t btn_open = alloy_create_button(hs);
    alloy_set_text(btn_open, "Choose File...");

    alloy_component_t btn_color = alloy_create_button(hs);
    alloy_set_text(btn_color, "Choose Color...");

    alloy_component_t spinner = alloy_create_spinner(vs);
    (void)spinner;

    alloy_component_t pb = alloy_create_progressbar(vs);
    alloy_set_value(pb, 0.45);

    alloy_set_event_callback(btn_open, ALLOY_EVENT_CLICK, on_open_click, win);
    alloy_set_event_callback(btn_color, ALLOY_EVENT_CLICK, on_color_click, win);

    printf("Starting Alloy GUI loop...\n");
    alloy_run(win);

    alloy_destroy(win);
    return 0;
}
