#include "alloy/api.h"
#include <stdio.h>

void on_click(alloy_component_t handle, alloy_event_type_t event, void *userdata) {
    printf("Button clicked!\n");
}

int main() {
    // Dual-engine Architecture: Secure host process with reactive signals
    alloy_component_t window = alloy_create_window("Alloy gui.c (Dual Engine)", 800, 600);

    alloy_component_t btn = alloy_create_button(window);
    alloy_set_text(btn, "Click Me");
    alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_click, NULL);

    alloy_add_child(window, btn);

    alloy_run(window);
    alloy_destroy(window);

    return 0;
}
