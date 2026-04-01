import { build } from "bun";
import { writeFileSync } from "fs";

async function createReplHost() {
  const cppReplHost = `
#include "webview/webview.h"
#ifdef WEBVIEW_USE_MJS
#include "mquickjs.h"
#endif
#include <iostream>
#include <string>
#include <vector>

int main() {
    webview::webview w(true, nullptr);
    w.set_title("AlloyScript REPL - Dual Engine");
    w.set_visible(false); // Hide webview window by default

    #ifdef WEBVIEW_USE_MJS
    JSContext *ctx = JS_NewContext(nullptr, 0, nullptr);
    #endif

    std::string line;
    std::cout << "AlloyScript REPL (MicroQuickJS + WebView)" << std::endl;
    std::cout << "> ";

    while (std::getline(std::cin, line)) {
        if (line == "exit") break;

        #ifdef WEBVIEW_USE_MJS
        // Initialize Alloy bindings if first run
        static bool initialized = false;
        if (!initialized) {
            // In a real build, we'd call setup_mjs_alloy_bindings(ctx)
            initialized = true;
        }

        // Evaluate in MicroQuickJS host first
        JSValue val = JS_Eval(ctx, line.c_str(), line.size(), "<repl>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(val)) {
            JSValue exc = JS_GetException(ctx);
            const char* str = JS_ToCString(ctx, exc, nullptr);
            std::cerr << "Error: " << str << std::endl;
            JS_FreeCString(ctx, str, nullptr);
            JS_FreeValue(ctx, exc);
        } else {
            const char* str = JS_ToCString(ctx, val, nullptr);
            std::cout << str << std::endl;
            JS_FreeCString(ctx, str, nullptr);
        }
        JS_FreeValue(ctx, val);
        #endif

        std::cout << "> ";
    }

    return 0;
}
`;
  writeFileSync("repl_host.cc", cppReplHost);
}
createReplHost();
