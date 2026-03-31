#include "gui/alloy.h"
#include <stdio.h>

void on_button_click(alloy_component_t component, void *userdata) {
    printf("Button clicked: %s\n", (const char*)userdata);
}

int main() {
    alloy_component_t win, root, header, sidebar, content, btn, input, lbl, check, slider, progress;

    // Create Main Window
    if (alloy_create_window("AlloyScript Comprehensive C Example", 1024, 768, &win) != ALLOY_OK) {
        return 1;
    }

    // Main Layout (HStack for Sidebar + Content)
    alloy_create_hstack(win, &root);
    alloy_add_child(win, root);

    // Sidebar
    alloy_create_vstack(root, &sidebar);
    alloy_set_flex(sidebar, 0.2);
    alloy_set_padding(sidebar, 10, 10, 10, 10);

    const char* nav[] = {"Dashboard", "Users", "Settings", "Logs"};
    for(int i=0; i<4; i++) {
        alloy_create_button(sidebar, &btn);
        alloy_set_text(btn, nav[i]);
        alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_button_click, (void*)nav[i]);
    }

    // Content Area
    alloy_create_vstack(root, &content);
    alloy_set_flex(content, 0.8);
    alloy_set_padding(content, 20, 20, 20, 20);

    alloy_create_label(content, &header);
    alloy_set_text(header, "User Registration Form");
    // Style the header
    alloy_style_t header_style = {0};
    header_style.font_size = 24;
    alloy_set_style(header, &header_style);

    alloy_create_label(content, &lbl);
    alloy_set_text(lbl, "Full Name:");
    alloy_create_textfield(content, &input);
    alloy_set_text(input, "Enter name...");

    alloy_create_checkbox(content, &check);
    alloy_set_text(check, "Subscribe to newsletter");
    alloy_set_checked(check, 1);

    alloy_create_label(content, &lbl);
    alloy_set_text(lbl, "Difficulty Level:");
    alloy_create_slider(content, &slider);
    alloy_set_value(slider, 0.5);

    alloy_create_progressbar(content, &progress);
    alloy_set_value(progress, 0.75);

    alloy_create_button(content, &btn);
    alloy_set_text(btn, "Submit Data");
    alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_button_click, "Submit");

    // Start Event Loop
    printf("Starting AlloyScript GUI Example...\n");
    alloy_layout(win);
    alloy_run(win);

    alloy_destroy(win);
    return 0;
}
