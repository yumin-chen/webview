#ifndef ALLOY_COCOA_GUI_HH
#define ALLOY_COCOA_GUI_HH

#include "../component_base.hh"
#include <objc/objc-runtime.h>
#include <objc/message.h>

namespace alloy::detail {

class cocoa_component : public component_base {
public:
    cocoa_component(id view, bool is_container = false)
        : component_base(is_container), m_view(view) {}

    virtual ~cocoa_component() {
        if (m_view) {
            ((void (*)(id, SEL))objc_msgSend)(m_view, sel_registerName("removeFromSuperview"));
            ((void (*)(id, SEL))objc_msgSend)(m_view, sel_registerName("release"));
        }
    }

    alloy_error_t set_text(std::string_view text) override {
        id str = ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), text.data());
        ((void (*)(id, SEL, id))objc_msgSend)(m_view, sel_registerName("setTitle:"), str);
        return ALLOY_OK;
    }

    alloy_error_t get_text(char *buf, size_t len) override {
        return ALLOY_ERROR_NOT_SUPPORTED;
    }

    alloy_error_t set_checked(bool v) override {
        ((void (*)(id, SEL, long))objc_msgSend)(m_view, sel_registerName("setState:"), v ? 1 : 0);
        return ALLOY_OK;
    }

    bool get_checked() override {
        return ((long (*)(id, SEL))objc_msgSend)(m_view, sel_registerName("state")) == 1;
    }

    alloy_error_t set_value(double v) override {
        ((void (*)(id, SEL, double))objc_msgSend)(m_view, sel_registerName("setDoubleValue:"), v);
        return ALLOY_OK;
    }

    double get_value() override {
        return ((double (*)(id, SEL))objc_msgSend)(m_view, sel_registerName("doubleValue"));
    }

    alloy_error_t set_enabled(bool v) override {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(m_view, sel_registerName("setEnabled:"), v ? YES : NO);
        return ALLOY_OK;
    }

    bool get_enabled() override {
        return ((BOOL (*)(id, SEL))objc_msgSend)(m_view, sel_registerName("isEnabled"));
    }

    alloy_error_t set_visible(bool v) override {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(m_view, sel_registerName("setHidden:"), v ? NO : YES);
        return ALLOY_OK;
    }

    bool get_visible() override {
        return !((BOOL (*)(id, SEL))objc_msgSend)(m_view, sel_registerName("isHidden"));
    }

    alloy_error_t set_style(const alloy_style_t &s) override {
        return ALLOY_OK;
    }

    void *native_handle() override { return m_view; }

protected:
    id m_view;
};

}

#endif
