#include "webview/webview.h"
#include "webview/meta.hh"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>

#include "bundle.h"

int main(int argc, char** argv) {
    std::string cron_title, cron_period, script_path;
    bool is_cron = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--cron-title=") == 0) { cron_title = arg.substr(13); is_cron = true; }
        else if (arg.find("--cron-period=") == 0) cron_period = arg.substr(14);
        else if (arg.find("run") != std::string::npos) {}
        else if (arg.find("--") != 0) script_path = arg;
    }

    try {
        auto w = std::make_shared<webview::webview>(is_cron ? false : true, nullptr);

        auto mgr = std::make_shared<webview::meta::SubprocessManager>(w.get());
        mgr->bind(*w);
        w->init(ALLOYSCRIPT_BUNDLE);

        if (is_cron) {
            std::string js = "window.onload = () => { if (typeof window.defaultExport !== 'undefined' && window.defaultExport.scheduled) { "
                             "window.defaultExport.scheduled({ cron: '" + cron_period + "', type: 'scheduled', scheduledTime: Date.now() }); "
                             "} else { console.error('No scheduled() handler found'); } "
                             "setTimeout(() => window.__alloy_terminate(), 1000); };";
            w->bind("__alloy_terminate", [&](const std::string&) -> std::string { w->terminate(); return ""; });
            w->init(js);
        } else {
            w->set_title("AlloyScript Application");
            w->set_size(1024, 768, WEBVIEW_HINT_NONE);
            w->set_html("<!DOCTYPE html><html><body><div id='root'></div></body></html>");
        }

        w->run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
