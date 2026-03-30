#include "alloy/api.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    alloy_component_t win = alloy_create_window("Alloy GUI Test", 800, 600);
    if (!win) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }

    alloy_signal_t text_sig = alloy_signal_create_str("Click Me!");
    alloy_component_t btn = alloy_create_button(win, "Initial Label");
    alloy_bind_property(btn, ALLOY_PROP_LABEL, text_sig);

    alloy_component_t lbl = alloy_create_label(win, "Reactive Label");
    alloy_bind_property(lbl, ALLOY_PROP_TEXT, text_sig);

    std::thread([text_sig]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        alloy_signal_set_str(text_sig, "Updated via Signal!");
    }).detach();

    alloy_layout(win);
    alloy_run(win);

    return 0;
}
