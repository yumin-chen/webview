#include "cron_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>

namespace alloy {

static std::string run_command(const std::string& cmd, const std::string& input = "") {
    int input_pipe[2];
    int output_pipe[2];
    if (pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
        throw std::runtime_error("Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error("Failed to fork");
    }

    if (pid == 0) { // Child
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);

        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(1);
    }

    // Parent
    close(input_pipe[0]);
    close(output_pipe[1]);

    if (!input.empty()) {
        write(input_pipe[1], input.c_str(), input.size());
    }
    close(input_pipe[1]);

    std::stringstream output;
    char buffer[4096];
    ssize_t n;
    while ((n = read(output_pipe[0], buffer, sizeof(buffer))) > 0) {
        output.write(buffer, n);
    }
    close(output_pipe[0]);

    int status;
    waitpid(pid, &status, 0);
    return output.str();
}

void cron_manager::register_job(const std::string& path, const std::string& schedule, const std::string& title) {
    char current_path[4096];
    if (getcwd(current_path, sizeof(current_path)) == nullptr) {
        throw std::runtime_error("Failed to get current directory");
    }
    std::string alloy_exe = std::string(current_path) + "/alloy_bin"; // Assuming the binary is named alloy_bin

    std::string current_crontab = run_command("crontab -l 2>/dev/null");
    std::stringstream ss(current_crontab);
    std::string line;
    std::vector<std::string> lines;
    std::string marker = "# Alloy-cron: " + title;
    bool in_old_job = false;

    while (std::getline(ss, line)) {
        if (line == marker) {
            in_old_job = true;
            continue;
        }
        if (in_old_job) {
            in_old_job = false;
            continue;
        }
        lines.push_back(line);
    }

    std::stringstream new_crontab;
    for (const auto& l : lines) {
        new_crontab << l << "\n";
    }

    // Add new job
    new_crontab << marker << "\n";
    new_crontab << schedule << " '" << alloy_exe << "' run --cron-title='" << title << "' --cron-period='" << schedule << "' '" << path << "'\n";

    run_command("crontab -", new_crontab.str());
}

void cron_manager::remove_job(const std::string& title) {
    std::string current_crontab = run_command("crontab -l 2>/dev/null");
    std::stringstream ss(current_crontab);
    std::string line;
    std::vector<std::string> lines;
    std::string marker = "# Alloy-cron: " + title;
    bool in_old_job = false;
    bool found = false;

    while (std::getline(ss, line)) {
        if (line == marker) {
            in_old_job = true;
            found = true;
            continue;
        }
        if (in_old_job) {
            in_old_job = false;
            continue;
        }
        lines.push_back(line);
    }

    if (found) {
        std::stringstream new_crontab;
        for (const auto& l : lines) {
            new_crontab << l << "\n";
        }
        run_command("crontab -", new_crontab.str());
    }
}

} // namespace alloy
