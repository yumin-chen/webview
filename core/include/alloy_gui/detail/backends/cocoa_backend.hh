#ifndef ALLOY_GUI_COCOA_BACKEND_HH
#define ALLOY_GUI_COCOA_BACKEND_HH

#include "../component.hh"
#include "webview/detail/platform/darwin/cocoa/NSWindow.hh"
#include "webview/detail/platform/darwin/cocoa/NSView.hh"
#include "webview/detail/platform/darwin/cocoa/NSString.hh"

namespace alloy {
namespace detail {

class CocoaBackend {
public:
    static Component* create_window(const char *title, int width, int height) {
        using namespace webview::detail::cocoa;
        NSRect rect = { {0, 0}, {(double)width, (double)height} };
        id win = NSWindow_withContentRect(rect,
            (NSWindowStyleMask)(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable),
            NSBackingStoreBuffered, false);
        NSWindow_set_title(win, title);
        NSWindow_makeKeyAndOrderFront(win);
        return new Component(win);
    }

    static Component* create_button(Component */*parent*/) {
        using namespace webview::detail;
        id btn = objc::msg_send<id>(objc::get_class("NSButton"), objc::selector("alloc"));
        NSRect rect = { {0, 0}, {100, 30} };
        btn = objc::msg_send<id>(btn, objc::selector("initWithFrame:"), rect);
        objc::msg_send<void>(btn, objc::selector("setButtonType:"), 0); // NSButtonTypeMomentaryLight
        objc::msg_send<void>(btn, objc::selector("setBezelStyle:"), 2); // NSBezelStyleRounded
        return new Component(btn);
    }

    static Component* create_label(Component */*parent*/) {
        using namespace webview::detail;
        id lbl = objc::msg_send<id>(objc::get_class("NSTextField"), objc::selector("alloc"));
        NSRect rect = { {0, 0}, {100, 20} };
        lbl = objc::msg_send<id>(lbl, objc::selector("initWithFrame:"), rect);
        objc::msg_send<void>(lbl, objc::selector("setEditable:"), static_cast<BOOL>(false));
        objc::msg_send<void>(lbl, objc::selector("setBezeled:"), static_cast<BOOL>(false));
        objc::msg_send<void>(lbl, objc::selector("setDrawsBackground:"), static_cast<BOOL>(false));
        return new Component(lbl);
    }

    static Component* create_textfield(Component */*parent*/) {
        using namespace webview::detail;
        id txt = objc::msg_send<id>(objc::get_class("NSTextField"), objc::selector("alloc"));
        NSRect rect = { {0, 0}, {150, 25} };
        txt = objc::msg_send<id>(txt, objc::selector("initWithFrame:"), rect);
        return new Component(txt);
    }
};

} // namespace detail
} // namespace alloy

#endif
