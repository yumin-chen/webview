#include "gui/alloy.h"
#include <iostream>
#include <vector>
#include <string>

class AlloyApp {
public:
    AlloyApp() {
        alloy_create_window("AlloyScript C++ Dashboard", 1024, 768, &win);
        build_ui();
    }

    void run() {
        std::cout << "Running C++ Professional GUI Example..." << std::endl;
        alloy_run(win);
    }

    ~AlloyApp() {
        alloy_destroy(win);
    }

private:
    alloy_component_t win;

    void build_ui() {
        alloy_component_t h_layout;
        alloy_create_hstack(win, &h_layout);

        // Sidebar
        alloy_component_t sidebar;
        alloy_create_vstack(h_layout, &sidebar);
        alloy_set_flex(sidebar, 0.25);

        const std::vector<std::string> nav_items = {"Dashboard", "Analytics", "Settings", "Help"};
        for (const auto& item : nav_items) {
            alloy_component_t btn;
            alloy_create_button(sidebar, &btn);
            alloy_set_text(btn, item.c_str());
        }

        // Content Area
        alloy_component_t content;
        alloy_create_vstack(h_layout, &content);
        alloy_set_flex(content, 0.75);

        alloy_component_t header;
        alloy_create_label(content, &header);
        alloy_set_text(header, "Welcome back, Developer");
    }
};

int main() {
    AlloyApp app;
    app.run();
    return 0;
}
