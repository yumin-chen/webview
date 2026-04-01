#ifndef ALLOY_COMPONENTS_BUTTON_HH
#define ALLOY_COMPONENTS_BUTTON_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>

namespace alloy::detail {

class gtk_button : public component_base {
public:
  gtk_button(component_base* parent) : component_base(false) {
    m_widget = gtk_button_new();
    if (parent) {
      gtk_container_add(GTK_CONTAINER(parent->native_handle()), m_widget);
      gtk_widget_show(m_widget);
    }
  }

  alloy_error_t set_text(std::string_view text) override {
    gtk_button_set_label(GTK_BUTTON(m_widget), text.data());
    return ALLOY_OK;
  }

  alloy_error_t get_text(char *buf, size_t len) override {
    const char* label = gtk_button_get_label(GTK_BUTTON(m_widget));
    if (strlen(label) >= len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
    strcpy(buf, label);
    return ALLOY_OK;
  }

  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }

  alloy_error_t set_enabled(bool v) override {
    gtk_widget_set_sensitive(m_widget, v);
    return ALLOY_OK;
  }
  bool get_enabled() override { return gtk_widget_get_sensitive(m_widget); }

  alloy_error_t set_visible(bool v) override {
    gtk_widget_set_visible(m_widget, v);
    return ALLOY_OK;
  }
  bool get_visible() override { return gtk_widget_get_visible(m_widget); }

  alloy_error_t set_style(const alloy_style_t &s) override {
    GdkRGBA bg, fg;
    bg.red = ((s.background >> 24) & 0xFF) / 255.0;
    bg.green = ((s.background >> 16) & 0xFF) / 255.0;
    bg.blue = ((s.background >> 8) & 0xFF) / 255.0;
    bg.alpha = (s.background & 0xFF) / 255.0;

    fg.red = ((s.foreground >> 24) & 0xFF) / 255.0;
    fg.green = ((s.foreground >> 16) & 0xFF) / 255.0;
    fg.blue = ((s.foreground >> 8) & 0xFF) / 255.0;
    fg.alpha = (s.foreground & 0xFF) / 255.0;

    gtk_widget_override_background_color(m_widget, GTK_STATE_FLAG_NORMAL, &bg);
    gtk_widget_override_color(m_widget, GTK_STATE_FLAG_NORMAL, &fg);
    return ALLOY_OK;
  }
  void* native_handle() override { return m_widget; }

private:
  GtkWidget* m_widget;
};

} // namespace alloy::detail
#endif

#ifdef WEBVIEW_PLATFORM_DARWIN
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>

namespace alloy::detail {

class cocoa_button : public component_base {
public:
  cocoa_button(component_base* parent) : component_base(false) {
    m_widget = [[NSButton alloc] initWithFrame:NSZeroRect];
    [m_widget setButtonType:NSButtonTypeMomentaryLight];
    [m_widget setBezelStyle:NSBezelStyleRounded];
    if (parent) {
      NSView* parent_view = (NSView*)parent->native_handle();
      [parent_view addSubview:m_widget];
    }
  }

  alloy_error_t set_text(std::string_view text) override {
    [m_widget setTitle:[NSString stringWithUTF8String:text.data()]];
    return ALLOY_OK;
  }

  alloy_error_t get_text(char *buf, size_t len) override {
    const char* label = [[m_widget title] UTF8String];
    if (strlen(label) >= len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
    strcpy(buf, label);
    return ALLOY_OK;
  }

  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }

  alloy_error_t set_enabled(bool v) override {
    [m_widget setEnabled:v];
    return ALLOY_OK;
  }
  bool get_enabled() override { return [m_widget isEnabled]; }

  alloy_error_t set_visible(bool v) override {
    [m_widget setHidden:!v];
    return ALLOY_OK;
  }
  bool get_visible() override { return ![m_widget isHidden]; }

  alloy_error_t set_style(const alloy_style_t &s) override {
      // In Cocoa, basic styling of NSButton is limited unless using CALayer or custom cell
      return ALLOY_OK;
  }
  void* native_handle() override { return m_widget; }

private:
  NSButton* m_widget;
};

} // namespace alloy::detail
#endif
#endif

#endif // ALLOY_COMPONENTS_BUTTON_HH
