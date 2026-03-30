#include "alloy/api.h"
#include <iostream>
#include <vector>

int main() {
    alloy_component_t win = alloy_create_window("Alloy Comprehensive Demo", 1024, 768);

    alloy_component_t vstack = alloy_create_vstack(win);

    alloy_component_t title = alloy_create_label(vstack, "Native UI Components");

    alloy_component_t hstack = alloy_create_hstack(vstack);
    alloy_create_button(hstack, "Native Button");
    alloy_create_checkbox(hstack, "Native Checkbox");

    alloy_create_textfield(vstack);
    alloy_create_textarea(vstack);

    alloy_component_t combo = alloy_create_combobox(vstack);
    // Add items to combo...

    alloy_create_slider(vstack);
    alloy_create_progressbar(vstack);

    alloy_layout(win);
    alloy_run(win);

    return 0;
}
