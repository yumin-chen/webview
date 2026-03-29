#include "alloy_gui/api.h"
#include <stdio.h>

void on_button_click(alloy_component_t handle, void *userdata) {
    printf("Button clicked!\n");
    alloy_set_text(handle, "Clicked!");
}

int main() {
    alloy_component_t win = alloy_create_window("Alloy GUI Demo", 400, 300);
    alloy_component_t vstack = alloy_create_vstack(win);
    alloy_add_child(win, vstack);

    alloy_component_t btn = alloy_create_button(vstack);
    alloy_set_text(btn, "Click Me");
    alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_button_click, NULL);
    alloy_add_child(vstack, btn);

    alloy_layout(win);
    alloy_run(win);
    alloy_destroy(win);
    return 0;
}
