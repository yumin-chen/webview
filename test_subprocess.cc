#include "webview/detail/subprocess.hh"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

int main() {
    webview::detail::subprocess::options opts;
#ifdef _WIN32
    opts.cmd = {"cmd.exe", "/c", "echo hello world"};
#else
    opts.cmd = {"echo", "hello world"};
#endif

    webview::detail::subprocess proc(opts);
    bool success = proc.spawn({
        [](const std::string& data, bool is_stderr) {
            if (is_stderr) std::cerr << "ERR: ";
            std::cout << data;
        },
        [](int exit_code) {
            std::cout << "\nProcess exited with code: " << exit_code << std::endl;
        }
    });

    if (!success) {
        std::cerr << "Failed to spawn process" << std::endl;
        return 1;
    }

    std::cout << "Spawned process with PID: " << proc.get_pid() << std::endl;

    // Wait for exit or timeout
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
