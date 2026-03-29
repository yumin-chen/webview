#ifndef WEBVIEW_DETAIL_CRON_HH
#define WEBVIEW_DETAIL_CRON_HH

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <taskschd.h>
#include <comutil.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#else
#include <unistd.h>
#endif

namespace webview {
namespace detail {

class cron_manager {
public:
    static bool register_job(const std::string& path, const std::string& schedule, const std::string& title) {
#ifdef _WIN32
        return register_windows(path, schedule, title);
#elif defined(__APPLE__)
        return register_macos(path, schedule, title);
#else
        return register_linux(path, schedule, title);
#endif
    }

    static bool remove_job(const std::string& title) {
#ifdef _WIN32
        return remove_windows(title);
#elif defined(__APPLE__)
        return remove_macos(title);
#else
        return remove_linux(title);
#endif
    }

private:
#ifndef _WIN32
    static std::string exec(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    static bool register_linux(const std::string& path, const std::string& schedule, const std::string& title) {
        remove_linux(title);
        std::string Alloy_path = "/proc/self/exe"; // Simplified
        std::string line = "# Alloy-cron: " + title + "\n" + schedule + " '" + Alloy_path + "' run --cron-title=" + title + " --cron-period='" + schedule + "' '" + path + "'";

        std::string current_cron = exec("crontab -l 2>/dev/null");
        current_cron += line + "\n";

        FILE* fp = popen("crontab -", "w");
        if (!fp) return false;
        fprintf(fp, "%s", current_cron.c_str());
        return pclose(fp) == 0;
    }

    static bool remove_linux(const std::string& title) {
        std::string current_cron = exec("crontab -l 2>/dev/null");
        if (current_cron.empty()) return true;

        std::string marker = "# Alloy-cron: " + title;
        size_t pos = current_cron.find(marker);
        if (pos != std::string::npos) {
            size_t next_line = current_cron.find('\n', pos);
            if (next_line != std::string::npos) {
                size_t end_of_job = current_cron.find('\n', next_line + 1);
                current_cron.erase(pos, end_of_job == std::string::npos ? std::string::npos : end_of_job - pos + 1);
            }
        }

        FILE* fp = popen("crontab -", "w");
        if (!fp) return false;
        fprintf(fp, "%s", current_cron.c_str());
        return pclose(fp) == 0;
    }

    static bool register_macos(const std::string& path, const std::string& schedule, const std::string& title) {
        // launchd implementation...
        return false;
    }

    static bool remove_macos(const std::string& title) {
        // launchd implementation...
        return false;
    }
#else
    static bool register_windows(const std::string& path, const std::string& schedule, const std::string& title) {
        // Task Scheduler implementation...
        return false;
    }

    static bool remove_windows(const std::string& title) {
        // Task Scheduler implementation...
        return false;
    }
#endif
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_CRON_HH
