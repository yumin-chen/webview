#include "cron_manager.hpp"
#include "cron_parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>

namespace alloy {

void cron_manager::register_job(const std::string& path, const std::string& schedule, const std::string& title) {
    auto expr = cron_parser::parse(schedule);

    char current_path[4096];
    if (getcwd(current_path, sizeof(current_path)) == nullptr) {
        throw std::runtime_error("Failed to get current directory");
    }
    std::string alloy_exe = std::string(current_path) + "/alloy_bin";

    struct passwd *pw = getpwuid(getuid());
    std::string home_dir = pw->pw_dir;
    std::string plist_path = home_dir + "/Library/LaunchAgents/Alloy.cron." + title + ".plist";

    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    ss << "<plist version=\"1.0\">\n";
    ss << "<dict>\n";
    ss << "    <key>Label</key>\n";
    ss << "    <string>Alloy.cron." << title << "</string>\n";
    ss << "    <key>ProgramArguments</key>\n";
    ss << "    <array>\n";
    ss << "        <string>" << alloy_exe << "</string>\n";
    ss << "        <string>run</string>\n";
    ss << "        <string>--cron-title=" << title << "</string>\n";
    ss << "        <string>--cron-period=" << schedule << "</string>\n";
    ss << "        <string>" << path << "</string>\n";
    ss << "    </array>\n";
    ss << "    <key>StartCalendarInterval</key>\n";
    ss << "    <array>\n";

    auto generate_intervals = [&](const std::set<int>& months, const std::set<int>& days, const std::set<int>& hours, const std::set<int>& minutes, const std::set<int>& weekdays) {
        for (int m : months) {
            for (int d : days) {
                for (int h : hours) {
                    for (int mi : minutes) {
                        for (int w : weekdays) {
                            ss << "        <dict>\n";
                            if (m != -1) ss << "            <key>Month</key><integer>" << m << "</integer>\n";
                            if (d != -1) ss << "            <key>Day</key><integer>" << d << "</integer>\n";
                            if (h != -1) ss << "            <key>Hour</key><integer>" << h << "</integer>\n";
                            if (mi != -1) ss << "            <key>Minute</key><integer>" << mi << "</integer>\n";
                            if (w != -1) ss << "            <key>Weekday</key><integer>" << w << "</integer>\n";
                            ss << "        </dict>\n";
                        }
                    }
                }
            }
        }
    };

    std::set<int> m_set = expr.months.size() == 12 ? std::set<int>{-1} : expr.months;
    std::set<int> d_set = expr.days_of_month.size() == 31 ? std::set<int>{-1} : expr.days_of_month;
    std::set<int> h_set = expr.hours.size() == 24 ? std::set<int>{-1} : expr.hours;
    std::set<int> mi_set = expr.minutes.size() == 60 ? std::set<int>{-1} : expr.minutes;
    std::set<int> w_set = expr.days_of_week.size() == 7 ? std::set<int>{-1} : expr.days_of_week;

    if (expr.dom_restricted && expr.dow_restricted) {
        // POSIX OR logic: matches if DOM matches OR DOW matches.
        generate_intervals(m_set, d_set, h_set, mi_set, std::set<int>{-1});
        generate_intervals(m_set, std::set<int>{-1}, h_set, mi_set, w_set);
    } else if (expr.dom_restricted) {
        generate_intervals(m_set, d_set, h_set, mi_set, std::set<int>{-1});
    } else if (expr.dow_restricted) {
        generate_intervals(m_set, std::set<int>{-1}, h_set, mi_set, w_set);
    } else {
        generate_intervals(m_set, std::set<int>{-1}, h_set, mi_set, std::set<int>{-1});
    }

    ss << "    </array>\n";
    ss << "    <key>StandardOutPath</key>\n";
    ss << "    <string>/tmp/Alloy.cron." << title << ".stdout.log</string>\n";
    ss << "    <key>StandardErrorPath</key>\n";
    ss << "    <string>/tmp/Alloy.cron." << title << ".stderr.log</string>\n";
    ss << "</dict>\n";
    ss << "</plist>\n";

    std::ofstream ofs(plist_path);
    ofs << ss.str();
    ofs.close();

    std::string cmd = "launchctl load " + plist_path;
    system(cmd.c_str());
}

void cron_manager::remove_job(const std::string& title) {
    struct passwd *pw = getpwuid(getuid());
    std::string home_dir = pw->pw_dir;
    std::string plist_path = home_dir + "/Library/LaunchAgents/Alloy.cron." + title + ".plist";

    std::string cmd = "launchctl unload " + plist_path + " 2>/dev/null";
    system(cmd.c_str());
    unlink(plist_path.c_str());
}

} // namespace alloy
