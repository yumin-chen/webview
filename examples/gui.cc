#include "alloy_gui/api.h"
#include <iostream>
#include <string>

int main() {
    auto win = alloy_create_window("Alloy GUI C++ Example", 500, 400);
    auto hstack = alloy_create_hstack(win);

    auto sig = alloy_signal_create_str("Initial Signal Text");

    auto lbl = alloy_create_label(hstack);
    alloy_bind_property(lbl, ALLOY_PROP_TEXT, sig);

    auto btn = alloy_create_button(hstack);
    alloy_set_text(btn, "Update Signal");

    alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, [](alloy_component_t, alloy_event_type_t, void* s) {
        alloy_signal_set_str((alloy_signal_t)s, "Signal Updated!");
    }, sig);

    alloy_layout(win);
    alloy_run(win);

    alloy_destroy(win);
    alloy_signal_destroy(sig);
    return 0;
}
