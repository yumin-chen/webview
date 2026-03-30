#include "alloy/api.h"
#include <iostream>
#include <string>

class App {
public:
    App() {
        win = alloy_create_window("Alloy Modern C++ UI", 500, 400);
        auto vs = alloy_create_vstack(win);

        auto lbl = alloy_create_label(vs);
        alloy_set_text(lbl, "Subprocess Monitor");

        pb = alloy_create_progressbar(vs);
        alloy_set_value(pb, 0.0);

        auto btn = alloy_create_button(vs);
        alloy_set_text(btn, "Start Task");

        alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, [](alloy_component_t, alloy_event_type_t, void* arg) {
            auto self = static_cast<App*>(arg);
            self->on_click();
        }, this);
    }

    void on_click() {
        std::cout << "Task started..." << std::endl;
        alloy_set_value(pb, 0.8);
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
};

int main() {
    App app;
    app.run();
    return 0;
}
