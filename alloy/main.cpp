#include "webview/webview.h"
#include "runtime.hpp"
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR lpCmdLine, int /*nCmdShow*/) {
    int argc;
    LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        std::wstring warg(argv_w[i]);
        args.push_back(std::string(warg.begin(), warg.end()));
    }
    LocalFree(argv_w);
#else
int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) args.push_back(argv[i]);
#endif

    if (args.size() > 1 && args[1] == "run") {
        std::string cron_title, cron_period, script_path;
        for (size_t i = 2; i < args.size(); ++i) {
            std::string arg = args[i];
            if (arg.find("--cron-title=") == 0) {
                cron_title = arg.substr(13);
            } else if (arg.find("--cron-period=") == 0) {
                cron_period = arg.substr(14);
            } else if (arg[0] != '-') {
                script_path = arg;
            }
        }

        std::cout << "Executing cron job: " << cron_title << " (" << cron_period << ") with script: " << script_path << std::endl;

        char current_path[4096];
#ifdef _WIN32
        GetModuleFileName(NULL, current_path, sizeof(current_path));
        std::string alloy_exe = std::string(current_path);
        std::string dir = alloy_exe.substr(0, alloy_exe.find_last_of("\\/"));
        std::string wrapper_path = dir + "\\executor_wrapper.js";
#elif defined(__APPLE__)
        uint32_t size = sizeof(current_path);
        _NSGetExecutablePath(current_path, &size);
        std::string alloy_exe = std::string(current_path);
        std::string dir = alloy_exe.substr(0, alloy_exe.find_last_of("/"));
        std::string wrapper_path = dir + "/executor_wrapper.js";
#else
        readlink("/proc/self/exe", current_path, sizeof(current_path));
        std::string alloy_exe = std::string(current_path);
        std::string dir = alloy_exe.substr(0, alloy_exe.find_last_of("/"));
        std::string wrapper_path = dir + "/executor_wrapper.js";
#endif

        std::vector<std::string> node_cmd = {"node", wrapper_path, cron_title, cron_period, script_path};
        alloy::spawn_options options;
        options.stdout_mode = "inherit";
        options.stderr_mode = "inherit";

        alloy::subprocess proc(node_cmd, options);
        return proc.wait();
    }

    try {
        webview::webview w(true, nullptr);
        w.set_title("AlloyScript Runtime");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);

        alloy::runtime::init(w);

        w.set_html("<h1>AlloyScript Runtime</h1><p>Open devtools to interact with <code>window.Alloy.cron</code></p>");
        w.run();
        alloy::runtime::stop();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
