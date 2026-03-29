#include "cron_manager.hpp"
#include "cron_parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>

namespace alloy {

void cron_manager::register_job(const std::string& path, const std::string& schedule, const std::string& title) {
    auto expr = cron_parser::parse(schedule);

    char current_path[4096];
    if (getcwd(current_path, sizeof(current_path)) == nullptr) {
        throw std::runtime_error("Failed to get current directory");
    }
    std::string alloy_exe = std::string(current_path) + "\\alloy_bin.exe";

    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n";
    ss << "<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n";
    ss << "  <Triggers>\n";

    auto generate_triggers = [&](const std::set<int>& months, const std::set<int>& days, const std::set<int>& hours, const std::set<int>& minutes, const std::set<int>& weekdays) {
        int count = 0;
        // Check for Repetition (Step) compatibility
        bool minute_repetition = false;
        int interval = 0;
        if (minutes.size() > 1) {
            std::vector<int> sorted_minutes(minutes.begin(), minutes.end());
            int diff = sorted_minutes[1] - sorted_minutes[0];
            bool uniform = true;
            for (size_t i = 1; i < sorted_minutes.size(); ++i) {
                 if (sorted_minutes[i] - sorted_minutes[i-1] != diff) { uniform = false; break; }
            }
            if (uniform && 60 % diff == 0) {
                minute_repetition = true;
                interval = diff;
            }
        }

        // Simplistic trigger generation for demonstration/implementation
        if (minute_repetition && hours.size() == 24 && days.size() == 31 && months.size() == 12 && weekdays.size() == 7) {
            ss << "    <CalendarTrigger>\n";
            ss << "      <StartBoundary>2025-01-01T00:00:00</StartBoundary>\n";
            ss << "      <Repetition>\n";
            ss << "        <Interval>PT" << interval << "M</Interval>\n";
            ss << "      </Repetition>\n";
            ss << "      <ScheduleByDay><DaysInterval>1</DaysInterval></ScheduleByDay>\n";
            ss << "    </CalendarTrigger>\n";
            return 1;
        }

        // Expand individual triggers
        for (int m : months) {
            for (int d : days) {
                for (int h : hours) {
                    for (int mi : (minute_repetition ? std::set<int>{minutes.begin(), minutes.begin()} : minutes)) {
                        ss << "    <CalendarTrigger>\n";
                        ss << "      <StartBoundary>2025-" << (m < 10 ? "0" : "") << m << "-" << (d < 10 ? "0" : "") << d << "T" << (h < 10 ? "0" : "") << h << ":" << (mi < 10 ? "0" : "") << mi << ":00</StartBoundary>\n";
                        if (minute_repetition) {
                             ss << "      <Repetition><Interval>PT" << interval << "M</Interval></Repetition>\n";
                        }
                        ss << "      <ScheduleByMonth>\n";
                        ss << "        <DaysOfMonth><Day>" << d << "</Day></DaysOfMonth>\n";
                        ss << "        <Months><Month>" << m << "</Month></Months>\n";
                        ss << "      </ScheduleByMonth>\n";
                        ss << "    </CalendarTrigger>\n";
                        count++;
                    }
                }
            }
        }
        return count;
    };

    int trigger_count = generate_triggers(expr.months, expr.days_of_month, expr.hours, expr.minutes, expr.days_of_week);

    if (trigger_count > 48) {
        throw std::runtime_error("Windows Task Scheduler limit exceeded: max 48 triggers per task");
    }

    ss << "  </Triggers>\n";
    ss << "  <Actions Context=\"Author\">\n";
    ss << "    <Exec>\n";
    ss << "      <Command>\"" << alloy_exe << "\"</Command>\n";
    ss << "      <Arguments>run --cron-title=\"" << title << "\" --cron-period=\"" << schedule << "\" \"" << path << "\"</Arguments>\n";
    ss << "    </Exec>\n";
    ss << "  </Actions>\n";
    ss << "  <Principals><Principal id=\"Author\"><LogonType>S4U</LogonType></Principal></Principals>\n";
    ss << "</Task>\n";

    std::string xml_path = "Alloy-cron-" + title + ".xml";
    std::ofstream ofs(xml_path);
    ofs << ss.str();
    ofs.close();

    std::string cmd_line = "schtasks /create /xml \"" + xml_path + "\" /tn \"Alloy-cron-" + title + "\" /f";

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (CreateProcess(NULL, (LPSTR)cmd_line.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    DeleteFile(xml_path.c_str());
}

void cron_manager::remove_job(const std::string& title) {
    std::string task_name = "Alloy-cron-" + title;
    std::string cmd_line = "schtasks /delete /tn \"" + task_name + "\" /f";

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (CreateProcess(NULL, (LPSTR)cmd_line.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

} // namespace alloy
