#include "webview/webview.h"
#include "webview/meta.hh"
#include <iostream>
#include <string>
#include <memory>

#include "bundle.h"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--cron-title=") == 0) {
            std::string title = arg.substr(13);
            std::string period;
            std::string script_path;
            for (int j = 1; j < argc; ++j) {
                std::string a = argv[j];
                if (a.find("--cron-period=") == 0) period = a.substr(14);
                else if (a.find("--") != 0 && a.find("run") != 0) script_path = a;
            }
            std::cout << "Cron Job Execution Simulation: " << title << "\n";
            return 0;
        }
    }

    try {
        auto w = std::make_shared<webview::webview>(true, nullptr);
        w->set_title("MetaScript Executable");
        w->set_size(1024, 768, WEBVIEW_HINT_NONE);

        auto mgr = std::make_shared<webview::meta::SubprocessManager>(w.get());
        mgr->bind(*w);

        w->init(METASCRIPT_BUNDLE);
        w->set_html("<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><div id='root'></div></body></html>");
        w->run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
