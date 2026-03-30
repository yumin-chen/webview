#include "alloy/api.h"
#include <iostream>

void on_click(alloy_component_t handle, alloy_event_type_t event, void *userdata) {
    std::cout << "Button clicked!" << std::endl;
}

int main() {
    auto window = alloy_create_window("C++ GUI Example", 400, 300);

    auto btn = alloy_create_button(window);
    alloy_set_text(btn, "Click Me");
    alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_click, NULL);

    alloy_add_child(window, btn);

    alloy_run(window);
    alloy_destroy(window);

    return 0;
}
