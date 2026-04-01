#include "alloy_gui/api.h"
#include "alloy_gui/detail/component.hh"

namespace alloy {
namespace detail {

void signal_base::notify() {
    for (auto& sub : subscribers) {
        sub.first->on_signal_changed(sub.second, value);
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

alloy_signal_t alloy_signal_create_double(double initial) {
    signal_base *s = new signal_base();
    s->value.type = signal_type::DOUBLE;
    s->value.d = initial;
    return (alloy_signal_t)s;
}

alloy_signal_t alloy_signal_create_int(int initial) {
    signal_base *s = new signal_base();
    s->value.type = signal_type::INT;
    s->value.i = initial;
    return (alloy_signal_t)s;
}

alloy_signal_t alloy_signal_create_bool(int initial) {
    signal_base *s = new signal_base();
    s->value.type = signal_type::BOOL;
    s->value.b = static_cast<bool>(initial);
    return (alloy_signal_t)s;
}

alloy_error_t alloy_signal_set_str(alloy_signal_t s, const char *v) {
    signal_base *sig = (signal_base*)s;
    sig->value.s = v ? v : "";
    sig->notify();
    return ALLOY_OK;
}

alloy_error_t alloy_signal_set_double(alloy_signal_t s, double v) {
    signal_base *sig = (signal_base*)s;
    sig->value.d = v;
    sig->notify();
    return ALLOY_OK;
}

alloy_error_t alloy_signal_set_int(alloy_signal_t s, int v) {
    signal_base *sig = (signal_base*)s;
    sig->value.i = v;
    sig->notify();
    return ALLOY_OK;
}

alloy_error_t alloy_signal_set_bool(alloy_signal_t s, int v) {
    signal_base *sig = (signal_base*)s;
    sig->value.b = static_cast<bool>(v);
    sig->notify();
    return ALLOY_OK;
}

const char* alloy_signal_get_str(alloy_signal_t s) { return ((signal_base*)s)->value.s.c_str(); }
double alloy_signal_get_double(alloy_signal_t s) { return ((signal_base*)s)->value.d; }
int alloy_signal_get_int(alloy_signal_t s) { return ((signal_base*)s)->value.i; }
int alloy_signal_get_bool(alloy_signal_t s) { return ((signal_base*)s)->value.b ? 1 : 0; }

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

alloy_error_t alloy_unbind_property(alloy_component_t component, alloy_prop_id_t property) {
    (void)component; (void)property;
    return ALLOY_OK;
}

alloy_computed_t alloy_computed_create(alloy_signal_t*, size_t, alloy_compute_cb_t, void*) { return nullptr; }
alloy_effect_t alloy_effect_create(alloy_signal_t*, size_t, void (*)(void*), void*) { return nullptr; }
alloy_error_t alloy_computed_destroy(alloy_computed_t) { return ALLOY_OK; }
alloy_error_t alloy_effect_destroy(alloy_effect_t) { return ALLOY_OK; }

}
