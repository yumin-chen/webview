#include "webview/webview.h"
#include "webview/meta.hh"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>

int main(int argc, char** argv) {
    // CLI Argument handling for Cron jobs
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--cron-title=") == 0) {
            std::cout << "Cron Job Execution Simulation: " << arg.substr(13) << "\n";
            return 0;
        }
    }

    try {
        auto w = std::make_shared<webview::webview>(true, nullptr);
        w->set_title("MetaScript Runtime (Native GUI + Web)");
        w->set_size(1024, 768, WEBVIEW_HINT_NONE);

        auto mgr = std::make_shared<webview::meta::SubprocessManager>(w.get());
        mgr->bind(*w);

        // Load runtime and user code from dist if available, or fallback to a simple bootstrap
        std::ifstream f("dist/bundle.js");
        if (f.good()) {
            std::string bundle((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            w->init(bundle);
        } else {
            std::cerr << "Warning: dist/bundle.js not found. Run 'bun build.ts' first.\n";
        }

        w->set_html("<!DOCTYPE html><html><body><h1>MetaScript Environment</h1><div id='root'></div></body></html>");
        w->run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
