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
        while (fgets(buffer.data(), (int)buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    static bool register_linux(const std::string& path, const std::string& schedule, const std::string& title) {
        remove_linux(title);
        std::string Alloy_path = "/proc/self/exe";
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
        // macOS uses launchd. Plist in ~/Library/LaunchAgents
        std::string home = getenv("HOME");
        std::string plist_path = home + "/Library/LaunchAgents/Alloy.cron." + title + ".plist";
        std::ofstream f(plist_path);
        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        f << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
        f << "<plist version=\"1.0\">\n<dict>\n";
        f << "  <key>Label</key><string>Alloy.cron." << title << "</string>\n";
        f << "  <key>ProgramArguments</key><array><string>/usr/local/bin/Alloy</string><string>run</string><string>" << path << "</string></array>\n";
        f << "  <key>StartInterval</key><integer>3600</integer>\n"; // Simplified schedule
        f << "</dict>\n</plist>";
        f.close();
        exec(("launchctl load " + plist_path).c_str());
        return true;
    }

    static bool remove_macos(const std::string& title) {
        std::string home = getenv("HOME");
        std::string plist_path = home + "/Library/LaunchAgents/Alloy.cron." + title + ".plist";
        exec(("launchctl unload " + plist_path).c_str());
        unlink(plist_path.c_str());
        return true;
    }
#else
    static bool register_windows(const std::string& path, const std::string& schedule, const std::string& title) {
        // Use schtasks command for simplicity in this draft
        std::string cmd = "schtasks /create /tn Alloy-cron-" + title + " /tr \"Alloy.exe run " + path + "\" /sc HOURLY /f";
        return system(cmd.c_str()) == 0;
    }

    static bool remove_windows(const std::string& title) {
        std::string cmd = "schtasks /delete /tn Alloy-cron-" + title + " /f";
        return system(cmd.c_str()) == 0;
    }
#endif
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_CRON_HH
