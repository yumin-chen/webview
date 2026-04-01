/*
 * AlloyScript Runtime - CC0 Unlicense Public Domain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ALLOY_ENGINE_CRON_HH
#define ALLOY_ENGINE_CRON_HH

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

namespace alloy::engine {

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

} // namespace alloy::engine

#endif // ALLOY_ENGINE_CRON_HH
