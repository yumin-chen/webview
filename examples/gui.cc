#include "gui/alloy.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>

class AlloyApp {
public:
    AlloyApp() {
        if (alloy_create_window("AlloyScript Professional C++ Dashboard", 1200, 800, &win) != ALLOY_OK) {
            throw std::runtime_error("Failed to create window");
        }
        build_ui();
    }

    void run() {
        std::cout << "Running C++ Professional GUI Example..." << std::endl;
        alloy_layout(win);
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
        alloy_add_child(win, h_layout);

        // --- Sidebar ---
        alloy_component_t sidebar;
        alloy_create_vstack(h_layout, &sidebar);
        alloy_set_flex(sidebar, 0.25);
        alloy_set_padding(sidebar, 15, 15, 15, 15);

        const std::vector<std::string> nav_items = {"Dashboard", "Analytics", "Settings", "Help", "Logout"};
        for (const auto& item : nav_items) {
            alloy_component_t btn;
            alloy_create_button(sidebar, &btn);
            alloy_set_text(btn, item.c_str());
            alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, [](alloy_component_t c, void* ud) {
                std::cout << "Navigating to: " << *(static_cast<std::string*>(ud)) << std::endl;
            }, new std::string(item));
        }

        // --- Content Area ---
        alloy_component_t content;
        alloy_create_vstack(h_layout, &content);
        alloy_set_flex(content, 0.75);
        alloy_set_padding(content, 30, 30, 30, 30);

        alloy_component_t header;
        alloy_create_label(content, &header);
        alloy_set_text(header, "System Status: Online");

        alloy_style_t header_style = {0};
        header_style.foreground = 0x00FF00FF; // Green
        header_style.font_size = 20;
        alloy_set_style(header, &header_style);

        alloy_component_t card;
        alloy_create_card(content, &card);
        alloy_set_padding(card, 20, 20, 20, 20);

        alloy_component_t card_lbl;
        alloy_create_label(card, &card_lbl);
        alloy_set_text(card_lbl, "System Resource Usage");

        alloy_component_t progress;
        alloy_create_progressbar(card, &progress);
        alloy_set_value(progress, 0.65);

        alloy_component_t footer;
        alloy_create_hstack(content, &footer);
        alloy_set_flex(footer, 0.1);

        alloy_component_t footer_lbl;
        alloy_create_label(footer, &footer_lbl);
        alloy_set_text(footer_lbl, "Build: v2.5.0-alloy");
    }
};

int main() {
    try {
        AlloyApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
