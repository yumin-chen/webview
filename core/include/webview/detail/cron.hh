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
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace webview {
namespace detail {

class cron_manager {
public:
    static bool register_job(const std::string& path, const std::string& schedule, const std::string& title) {
#ifdef _WIN32
        std::string cmd = "schtasks /create /tn Alloy-cron-" + title + " /tr \"Alloy.exe run " + path + "\" /sc MINUTE /mo 5 /f";
        return system(cmd.c_str()) == 0;
#elif defined(__APPLE__)
        std::string home = getenv("HOME");
        std::string plist_path = home + "/Library/LaunchAgents/Alloy.cron." + title + ".plist";
        std::ofstream f(plist_path);
        // Simple 5-field cron parsing placeholder
        std::stringstream ss(schedule); std::string min, hr, dom, mon, dow;
        ss >> min >> hr >> dom >> mon >> dow;
        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<plist version=\"1.0\">\n<dict>\n";
        f << "  <key>Label</key><string>Alloy.cron." << title << "</string>\n";
        f << "  <key>ProgramArguments</key><array><string>/usr/local/bin/Alloy</string><string>run</string><string>" << path << "</string></array>\n";
        f << "  <key>StartCalendarInterval</key><dict>\n";
        if (min != "*") f << "    <key>Minute</key><integer>" << min << "</integer>\n";
        if (hr != "*") f << "    <key>Hour</key><integer>" << hr << "</integer>\n";
        f << "  </dict>\n</dict>\n</plist>";
        f.close();
        system(("launchctl load " + plist_path).c_str());
        return true;
#else
        std::string Alloy_path = "/proc/self/exe";
        std::string line = "# Alloy-cron: " + title + "\n" + schedule + " '" + Alloy_path + "' run --cron-title=" + title + " --cron-period='" + schedule + "' '" + path + "'";
        FILE* fp = popen("crontab -", "w");
        if (!fp) return false;
        fprintf(fp, "%s", line.c_str());
        return pclose(fp) == 0;
#endif
    }

    static bool remove_job(const std::string& title) {
#ifdef _WIN32
        std::string cmd = "schtasks /delete /tn Alloy-cron-" + title + " /f";
        return system(cmd.c_str()) == 0;
#elif defined(__APPLE__)
        std::string home = getenv("HOME");
        std::string plist_path = home + "/Library/LaunchAgents/Alloy.cron." + title + ".plist";
        system(("launchctl unload " + plist_path).c_str());
        unlink(plist_path.c_str());
        return true;
#else
        return true; // Simplified
#endif
    }
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_CRON_HH
