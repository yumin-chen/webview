#include "alloy/api.h"
#include <stdio.h>

int main() {
    alloy_component_t win = alloy_create_window("Alloy Native GUI Demo", 400, 300);
    alloy_component_t vs = alloy_create_vstack(win);

    alloy_component_t lbl = alloy_create_label(vs);
    alloy_set_text(lbl, "Welcome to Alloy Native");

    alloy_component_t pb = alloy_create_progressbar(vs);
    alloy_set_value(pb, 0.45);

    alloy_component_t hs = alloy_create_hstack(vs);

    alloy_component_t btn1 = alloy_create_button(hs);
    alloy_set_text(btn1, "Action A");

    alloy_component_t btn2 = alloy_create_button(hs);
    alloy_set_text(btn2, "Action B");

    printf("Starting Alloy GUI loop...\n");
    alloy_run(win);

    alloy_destroy(win);
    return 0;
}
