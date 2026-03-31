#include "alloy/api.h"
#include "alloy/detail/component_base.hh"
#ifdef __linux__
#include "alloy/detail/backends/gtk_gui.hh"
#endif

using namespace alloy::detail;

extern "C" {
#ifdef __linux__
ALLOY_API alloy_component_t alloy_create_link(alloy_component_t parent, const char* text, const char* url) {
    auto* l = new gtk_component(gtk_link_button_new_with_label(url, text));
    if (parent) static_cast<component_base*>(parent)->add_child(l);
    return l;
}

ALLOY_API alloy_component_t alloy_create_tooltip(alloy_component_t parent, const char* text) {
    auto* l = new gtk_component(gtk_label_new(text));
    if (parent) static_cast<component_base*>(parent)->add_child(l);
    return l;
}

ALLOY_API alloy_component_t alloy_create_divider(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_separator(alloy_component_t parent) {
    auto* s = new gtk_component(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    if (parent) static_cast<component_base*>(parent)->add_child(s);
    return s;
}

ALLOY_API alloy_component_t alloy_create_badge(alloy_component_t parent, const char* text) {
    auto* l = new gtk_component(gtk_label_new(text));
    if (parent) static_cast<component_base*>(parent)->add_child(l);
    return l;
}

ALLOY_API alloy_component_t alloy_create_chip(alloy_component_t parent, const char* label) {
    auto* b = new gtk_component(gtk_button_new_with_label(label));
    if (parent) static_cast<component_base*>(parent)->add_child(b);
    return b;
}

ALLOY_API alloy_component_t alloy_create_rating(alloy_component_t parent) {
    auto* b = new gtk_container(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    if (parent) static_cast<component_base*>(parent)->add_child(b);
    return b;
}

ALLOY_API alloy_component_t alloy_create_richtexteditor(alloy_component_t parent) {
    auto* tv = gtk_text_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}

ALLOY_API alloy_component_t alloy_create_codeeditor(alloy_component_t parent) {
    auto* tv = gtk_text_view_new();
    auto* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_widget_show(tv);
    auto* comp = new gtk_component(sw);
    if (parent) static_cast<component_base*>(parent)->add_child(comp);
    return comp;
}
#else
ALLOY_API alloy_component_t alloy_create_link(alloy_component_t, const char*, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_tooltip(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_divider(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_separator(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_badge(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_chip(alloy_component_t, const char*) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_rating(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_richtexteditor(alloy_component_t) { return nullptr; }
ALLOY_API alloy_component_t alloy_create_codeeditor(alloy_component_t) { return nullptr; }
#endif
}
