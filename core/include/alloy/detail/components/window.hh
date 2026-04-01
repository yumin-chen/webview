/*
 * AlloyScript Runtime - CC0 Unlicense Public Domain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ALLOY_COMPONENTS_WINDOW_HH
#define ALLOY_COMPONENTS_WINDOW_HH

#include "../component_base.hh"

#ifdef WEBVIEW_PLATFORM_LINUX
#include <gtk/gtk.h>

namespace alloy::detail {

class gtk_window : public component_base {
public:
  gtk_window(const char* title, int width, int height) : component_base(true) {
    m_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_widget), title);
    gtk_window_set_default_size(GTK_WINDOW(m_widget), width, height);

    m_fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(m_widget), m_fixed);
    gtk_widget_show(m_fixed);
    gtk_widget_show(m_widget);
  }

  void *native_handle() override { return m_fixed; }

  alloy_error_t set_text(std::string_view text) override {
    gtk_window_set_title(GTK_WINDOW(m_widget), text.data());
    return ALLOY_OK;
  }

  alloy_error_t get_text(char *buf, size_t len) override {
    const char* text = gtk_window_get_title(GTK_WINDOW(m_widget));
    if (strlen(text) >= len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
    strcpy(buf, text);
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

  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }

private:
  GtkWidget* m_widget;
  GtkWidget* m_fixed;
};

} // namespace alloy::detail
#endif

#ifdef WEBVIEW_PLATFORM_DARWIN
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>

namespace alloy::detail {

class cocoa_window : public component_base {
public:
  cocoa_window(const char* title, int width, int height) : component_base(true) {
    m_window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
                                           styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    [m_window setTitle:[NSString stringWithUTF8String:title]];
    [m_window makeKeyAndOrderFront:nil];
    m_content_view = [m_window contentView];
  }

  alloy_error_t set_text(std::string_view text) override {
    [m_window setTitle:[NSString stringWithUTF8String:text.data()]];
    return ALLOY_OK;
  }

  alloy_error_t get_text(char *buf, size_t len) override {
    const char* title = [[m_window title] UTF8String];
    if (strlen(title) >= len) return ALLOY_ERROR_BUFFER_TOO_SMALL;
    strcpy(buf, title);
    return ALLOY_OK;
  }

  alloy_error_t set_checked(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_checked() override { return false; }
  alloy_error_t set_value(double v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  double get_value() override { return 0; }
  alloy_error_t set_enabled(bool v) override { return ALLOY_ERROR_NOT_SUPPORTED; }
  bool get_enabled() override { return true; }

  alloy_error_t set_visible(bool v) override {
    if (v) [m_window orderFront:nil];
    else [m_window orderOut:nil];
    return ALLOY_OK;
  }
  bool get_visible() override { return [m_window isVisible]; }

  alloy_error_t set_style(const alloy_style_t &s) override { return ALLOY_OK; }
  void* native_handle() override { return m_content_view; }

private:
  NSWindow* m_window;
  NSView* m_content_view;
};

} // namespace alloy::detail
#endif
#endif

#endif // ALLOY_COMPONENTS_WINDOW_HH
