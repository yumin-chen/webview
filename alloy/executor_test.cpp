#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "run") {
        std::string cron_title, cron_period, script_path;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
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
        if (getcwd(current_path, sizeof(current_path)) == nullptr) {
            return 1;
        }
        std::string wrapper_path = std::string(current_path) + "/alloy/executor_wrapper.js";

        std::string cmd = "node \"" + wrapper_path + "\" \"" + cron_title + "\" \"" + cron_period + "\" \"" + script_path + "\"";
        int ret = system(cmd.c_str());
        return WEXITSTATUS(ret);
    }
    return 0;
}
