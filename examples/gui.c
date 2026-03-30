#include "alloy/api.h"
#include <stdio.h>

int main() {
    alloy_component_t win = alloy_create_window("Alloy Native GUI Demo", 600, 500);
    alloy_component_t vs = alloy_create_vstack(win);

    alloy_component_t lbl = alloy_create_label(vs);
    alloy_set_text(lbl, "Alloy Native Form Demo");

    alloy_component_t entry = alloy_create_textfield(vs);
    alloy_set_text(entry, "Enter your name...");

    alloy_component_t hs = alloy_create_hstack(vs);
    alloy_component_t cb = alloy_create_checkbox(hs);
    alloy_set_text(cb, "Enable notifications");

    alloy_component_t sw = alloy_create_switch(hs);

    alloy_component_t slider = alloy_create_slider(vs);
    alloy_set_value(slider, 75.0);

    alloy_component_t pb = alloy_create_progressbar(vs);
    alloy_set_value(pb, 0.75);

    alloy_component_t btn = alloy_create_button(vs);
    alloy_set_text(btn, "Submit Form");

    printf("Starting Alloy GUI loop...\n");
    alloy_run(win);

    alloy_destroy(win);
    return 0;
}
