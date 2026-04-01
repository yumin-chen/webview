#include "alloy_gui/api.h"
#include <stdio.h>

void on_button_click(alloy_component_t handle, alloy_event_type_t event, void *userdata) {
    printf("Button clicked! Userdata: %s\n", (char*)userdata);
}

int main() {
    alloy_component_t win = alloy_create_window("Alloy GUI C Example", 400, 300);
    alloy_component_t vstack = alloy_create_vstack(win);

    alloy_component_t lbl = alloy_create_label(vstack);
    alloy_set_text(lbl, "Welcome to Alloy GUI");

    alloy_component_t btn = alloy_create_button(vstack);
    alloy_set_text(btn, "Click Me");
    alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_button_click, "some_data");

    alloy_layout(win);
    alloy_run(win);
    alloy_destroy(win);
    return 0;
}
