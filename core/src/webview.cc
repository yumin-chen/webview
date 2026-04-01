#include "webview/webview.h"
#include "webview/alloy.hh"
#include <map>
#include <memory>

namespace {
    static std::map<webview_t, std::shared_ptr<webview::detail::AlloyRuntime>> g_runtimes;
}

extern "C" {
    void webview_alloy_setup(webview_t w) {
        if (!w) return;
        // In the C API, webview_t is a pointer to a webview::webview instance
        auto* wv = static_cast<webview::webview*>(w);
        if (g_runtimes.find(w) == g_runtimes.end()) {
            g_runtimes[w] = std::make_shared<webview::detail::AlloyRuntime>(wv);
            wv->init(webview::detail::alloy_js_code);
        }
    }
}
