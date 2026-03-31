#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_signal_t alloy_signal_create_str(const char *initial) {
    return new signal(signal_value(std::string(initial)));
}
ALLOY_API alloy_signal_t alloy_signal_create_double(double initial) {
    return new signal(signal_value(initial));
}
ALLOY_API alloy_signal_t alloy_signal_create_int(int initial) {
    return new signal(signal_value(initial));
}
ALLOY_API alloy_signal_t alloy_signal_create_bool(int initial) {
    return new signal(signal_value(initial != 0));
}

ALLOY_API alloy_error_t alloy_signal_set_str(alloy_signal_t s, const char *v) {
    static_cast<signal*>(s)->set_value(signal_value(std::string(v)));
    return ALLOY_OK;
}
ALLOY_API alloy_error_t alloy_signal_set_double(alloy_signal_t s, double v) {
    static_cast<signal*>(s)->set_value(signal_value(v));
    return ALLOY_OK;
}
ALLOY_API alloy_error_t alloy_signal_set_int(alloy_signal_t s, int v) {
    static_cast<signal*>(s)->set_value(signal_value(v));
    return ALLOY_OK;
}
ALLOY_API alloy_error_t alloy_signal_set_bool(alloy_signal_t s, int v) {
    static_cast<signal*>(s)->set_value(signal_value(v != 0));
    return ALLOY_OK;
}

ALLOY_API alloy_error_t alloy_bind_property(alloy_component_t component, alloy_prop_id_t property, alloy_signal_t signal) {
    static_cast<component_base*>(component)->bind_property(property, static_cast<signal_base*>(signal));
    return ALLOY_OK;
}
#else
ALLOY_API alloy_signal_t alloy_signal_create_str(const char *) { return nullptr; }
ALLOY_API alloy_signal_t alloy_signal_create_double(double) { return nullptr; }
ALLOY_API alloy_signal_t alloy_signal_create_int(int) { return nullptr; }
ALLOY_API alloy_signal_t alloy_signal_create_bool(int) { return nullptr; }
ALLOY_API alloy_error_t alloy_signal_set_str(alloy_signal_t, const char *) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_signal_set_double(alloy_signal_t, double) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_signal_set_int(alloy_signal_t, int) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_signal_set_bool(alloy_signal_t, int) { return ALLOY_OK; }
ALLOY_API alloy_error_t alloy_bind_property(alloy_component_t, alloy_prop_id_t, alloy_signal_t) { return ALLOY_OK; }
#endif
}
