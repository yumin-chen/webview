#include "alloy/api.h"
#include <iostream>
#include <string>

class App {
public:
    App() {
        win = alloy_create_window("Alloy Modern C++ UI", 600, 500);
        auto vs = alloy_create_vstack(win);

        auto title = alloy_create_label(vs);
        alloy_set_text(title, "Subprocess Monitor & Dashboard");

        auto input_row = alloy_create_hstack(vs);
        auto lbl_name = alloy_create_label(input_row);
        alloy_set_text(lbl_name, "Process Name:");

        name_entry = alloy_create_textfield(input_row);
        alloy_set_text(name_entry, "bun");

        pb = alloy_create_progressbar(vs);
        alloy_set_value(pb, 0.0);

        auto controls = alloy_create_hstack(vs);
        auto btn_start = alloy_create_button(controls);
        alloy_set_text(btn_start, "Launch Subprocess");

        auto btn_stop = alloy_create_button(controls);
        alloy_set_text(btn_stop, "Terminate");

        alloy_set_event_callback(btn_start, ALLOY_EVENT_CLICK, [](alloy_component_t, alloy_event_type_t, void* arg) {
            auto self = static_cast<App*>(arg);
            self->on_start();
        }, this);
    }

    void on_start() {
        std::cout << "Launching process..." << std::endl;
        alloy_set_value(pb, 0.5);
    }

    void run() {
        alloy_run(win);
    }

    ~App() {
        alloy_destroy(win);
    }

private:
    alloy_component_t win;
    alloy_component_t pb;
    alloy_component_t name_entry;
};

int main() {
    App app;
    app.run();
    return 0;
}
