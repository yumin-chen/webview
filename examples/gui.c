#include "gui/alloy.h"
#include <stdio.h>

int main() {
    alloy_component_t win, sidebar, main_content, title, btn1, btn2, input;

    // Create Main Window
    alloy_create_window("AlloyScript Professional Dashboard", 1024, 768, &win);

    // Create Sidebar
    alloy_create_vstack(win, &sidebar);
    alloy_set_padding(sidebar, 20, 10, 20, 10);
    alloy_set_flex(sidebar, 0.3);

    alloy_create_label(sidebar, &title);
    alloy_set_text(title, "SETTINGS");

    alloy_create_button(sidebar, &btn1);
    alloy_set_text(btn1, "Profile");

    alloy_create_button(sidebar, &btn2);
    alloy_set_text(btn2, "Network");

    // Create Main Content
    alloy_create_vstack(win, &main_content);
    alloy_set_flex(main_content, 0.7);
    alloy_set_padding(main_content, 40, 40, 40, 40);

    alloy_create_label(main_content, &title);
    alloy_set_text(title, "User Profile");

    alloy_create_textfield(main_content, &input);
    alloy_set_text(input, "John Doe");

    alloy_create_button(main_content, &btn1);
    alloy_set_text(btn1, "Save Changes");

    // Start Event Loop
    printf("Starting AlloyScript GUI Example...\n");
    alloy_run(win);

    alloy_destroy(win);
    return 0;
}
