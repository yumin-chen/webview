#include "webview/test_driver.hh"
#include "alloy_gui/api.h"
#include <string>

TEST_CASE("Native GUI - Window and Button") {
    alloy_component_t win = alloy_create_window("Test Window", 400, 300);
    REQUIRE(win != nullptr);

    alloy_component_t btn = alloy_create_button(win);
    REQUIRE(btn != nullptr);

    alloy_set_text(btn, "Click Me");
    alloy_set_text(win, "New Title");

    REQUIRE(alloy_destroy(win) == ALLOY_OK);
}

TEST_CASE("Native GUI - Containers") {
    alloy_component_t win = alloy_create_window("Container Test", 400, 300);
    alloy_component_t vstack = alloy_create_vstack(win);
    REQUIRE(vstack != nullptr);

    alloy_component_t hstack = alloy_create_hstack(vstack);
    REQUIRE(hstack != nullptr);

    alloy_add_child(win, vstack);
    alloy_add_child(vstack, hstack);

    alloy_component_t btn1 = alloy_create_button(hstack);
    alloy_add_child(hstack, btn1);

    REQUIRE(alloy_destroy(win) == ALLOY_OK);
}

TEST_CASE("Native GUI - Signals") {
    alloy_signal_t sig = alloy_signal_create_str("initial");
    REQUIRE(sig != nullptr);

    alloy_component_t win = alloy_create_window("Signal Test", 100, 100);
    alloy_component_t btn = alloy_create_button(win);

    REQUIRE(alloy_bind_property(btn, ALLOY_PROP_TEXT, sig) == ALLOY_OK);
    REQUIRE(alloy_signal_set_str(sig, "updated") == ALLOY_OK);

    alloy_destroy(win);
    alloy_signal_destroy(sig);
}

TEST_CASE("Native GUI - Selection") {
    alloy_component_t win = alloy_create_window("Selection Test", 100, 100);
    REQUIRE(alloy_create_checkbox(win) != nullptr);
    REQUIRE(alloy_create_radiobutton(win) != nullptr);
    REQUIRE(alloy_create_combobox(win) != nullptr);
    REQUIRE(alloy_create_slider(win) != nullptr);
    REQUIRE(alloy_create_spinner(win) != nullptr);
    REQUIRE(alloy_create_progressbar(win) != nullptr);
    REQUIRE(alloy_create_switch(win) != nullptr);
    alloy_destroy(win);
}

TEST_CASE("Native GUI - Navigation") {
    alloy_component_t win = alloy_create_window("Navigation Test", 100, 100);
    REQUIRE(alloy_create_tabview(win) != nullptr);
    REQUIRE(alloy_create_listview(win) != nullptr);
    REQUIRE(alloy_create_treeview(win) != nullptr);
    REQUIRE(alloy_create_webview(win) != nullptr);
    alloy_destroy(win);
}

TEST_CASE("Native GUI - Dialogs") {
    alloy_component_t win = alloy_create_window("Dialog Test", 100, 100);
    REQUIRE(alloy_create_dialog("Dialog", 100, 100) != nullptr);
    REQUIRE(alloy_create_filedialog(win) != nullptr);
    REQUIRE(alloy_create_colorpicker(win) != nullptr);
    REQUIRE(alloy_create_datepicker(win) != nullptr);
    REQUIRE(alloy_create_timepicker(win) != nullptr);
    alloy_destroy(win);
}

TEST_CASE("Native GUI - Extra") {
    alloy_component_t win = alloy_create_window("Extra Test", 100, 100);
    REQUIRE(alloy_create_image(win) != nullptr);
    REQUIRE(alloy_create_menubar(win) != nullptr);
    REQUIRE(alloy_create_toolbar(win) != nullptr);
    REQUIRE(alloy_create_statusbar(win) != nullptr);
    REQUIRE(alloy_create_separator(win) != nullptr);
    REQUIRE(alloy_create_groupbox(win) != nullptr);
    REQUIRE(alloy_create_accordion(win) != nullptr);
    REQUIRE(alloy_create_badge(win) != nullptr);
    REQUIRE(alloy_create_chip(win) != nullptr);
    REQUIRE(alloy_create_card(win) != nullptr);
    REQUIRE(alloy_create_link(win) != nullptr);
    alloy_destroy(win);
}
