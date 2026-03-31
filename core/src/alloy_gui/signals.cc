#include "alloy_gui/api.h"
#include "alloy_gui/detail/backends/gtk_backend.hh"

namespace alloy {
namespace detail {

void signal_base::notify() {
    for (auto& sub : subscribers) {
        static_cast<Component*>(sub.first)->on_signal_changed(sub.second, value);
    }
}

} // namespace detail
} // namespace alloy

using namespace alloy::detail;

extern "C" {

alloy_signal_t alloy_signal_create_str(const char *initial) {
    signal_base *s = new signal_base();
    s->value.type = signal_type::STR;
    s->value.s = initial ? initial : "";
    return (alloy_signal_t)s;
}

alloy_error_t alloy_signal_set_str(alloy_signal_t s, const char *v) {
    signal_base *sig = (signal_base*)s;
    sig->value.s = v ? v : "";
    sig->notify();
    return ALLOY_OK;
}

alloy_error_t alloy_signal_destroy(alloy_signal_t s) {
    delete (signal_base*)s;
    return ALLOY_OK;
}

alloy_error_t alloy_bind_property(alloy_component_t component, alloy_prop_id_t property, alloy_signal_t signal) {
    Component *comp = (Component*)component;
    signal_base *sig = (signal_base*)signal;
    if (!comp || !sig) return ALLOY_ERROR_INVALID_ARGUMENT;
    sig->subscribers.push_back({comp, property});
    comp->on_signal_changed(property, sig->value);
    return ALLOY_OK;
}

}
