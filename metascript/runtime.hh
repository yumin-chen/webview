#ifndef METASCRIPT_RUNTIME_HH
#define METASCRIPT_RUNTIME_HH

#include "webview/webview.h"
#include "cron_parser.hh"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
#include <chrono>

#if defined(__linux__)
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <shlwapi.h>
#endif

namespace metascript {

class runtime {
public:
    runtime(bool debug = false, void* window = nullptr) : m_webview(debug, window) {
        bind_meta();
    }

    void run() {
        m_webview.run();
    }

    void set_html(const std::string& html) {
        m_webview.set_html(html);
    }

    void set_title(const std::string& title) {
        m_webview.set_title(title);
    }

    void set_size(int width, int height, int hints) {
        m_webview.set_size(width, height, hints);
    }

    void navigate(const std::string& url) {
        m_webview.navigate(url);
    }

    static int handle_cli(int argc, char** argv) {
        std::vector<std::string> args(argv, argv + argc);
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "run") {
                std::string title;
                std::string period;
                std::string script_path;
                for (size_t j = i + 1; j < args.size(); ++j) {
                    if (args[j].find("--cron-title=") == 0) {
                        title = args[j].substr(13);
                    } else if (args[j].find("--cron-period=") == 0) {
                        period = args[j].substr(14);
                    } else if (args[j].find("-") != 0) {
                        script_path = args[j];
                    }
                }
                if (!script_path.empty()) {
                    return execute_scheduled(script_path, title, period);
                }
            }
        }
        return -1; // Not handled
    }

private:
    webview::webview m_webview;

    void bind_meta() {
        m_webview.init(R"js(
            window.Meta = {
                cron: {
                    parse: function(expression, relativeDate) {
                        const rel = relativeDate ? (relativeDate instanceof Date ? relativeDate.getTime() : relativeDate) : Date.now();
                        return window.__meta_cron_parse(expression, Math.floor(rel / 1000)).then(res => res ? new Date(res * 1000) : null);
                    },
                    remove: function(title) {
                        return window.__meta_cron_remove(title);
                    }
                }
            };
            const metaCron = function(path, schedule, title) {
                return window.__meta_cron_register(path, schedule, title);
            };
            Object.assign(metaCron, window.Meta.cron);
            window.Meta.cron = metaCron;
        )js");

        m_webview.bind("__meta_cron_parse", [](const std::string& req) -> std::string {
            // req is ["expression", relative_time_in_seconds]
            auto expr = webview::detail::json_parse(req, "", 0);
            auto rel_str = webview::detail::json_parse(req, "", 1);
            std::time_t rel = std::stoll(rel_str);
            std::time_t next = cron_parser::parse(expr, rel);
            if (next == 0) return "null";
            return std::to_string(next);
        });

        m_webview.bind("__meta_cron_register", [this](const std::string& req) -> std::string {
            auto path = webview::detail::json_parse(req, "", 0);
            auto schedule = webview::detail::json_parse(req, "", 1);
            auto title = webview::detail::json_parse(req, "", 2);
            register_cron_job(path, schedule, title);
            return "true";
        });

        m_webview.bind("__meta_cron_remove", [this](const std::string& req) -> std::string {
            auto title = webview::detail::json_parse(req, "", 0);
            remove_cron_job(title);
            return "true";
        });
    }

    void register_cron_job(const std::string& path, const std::string& schedule, const std::string& title) {
#if defined(__linux__)
        remove_cron_job(title); // Ensure it's not duplicated

        std::string command = "crontab -l 2>/dev/null";
        std::string current_crontab = exec(command.c_str());

        std::stringstream ss;
        ss << current_crontab;
        if (!current_crontab.empty() && current_crontab.back() != '\n') {
            ss << "\n";
        }

        char exe_path[4096];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
        if (len != -1) {
            exe_path[len] = '\0';
        } else {
            strcpy(exe_path, "metascript");
        }

        // Get absolute path of script
        char abs_script_path[4096];
        if (realpath(path.c_str(), abs_script_path) == nullptr) {
            strncpy(abs_script_path, path.c_str(), sizeof(abs_script_path));
        }

        ss << "# Meta-cron: " << title << "\n";
        ss << schedule << " " << shell_escape(exe_path) << " run --cron-title=" << shell_escape(title) << " --cron-period=" << shell_escape(schedule) << " " << shell_escape(abs_script_path) << "\n";

        std::string new_crontab = ss.str();
        FILE* pipe = popen("crontab -", "w");
        if (pipe) {
            fwrite(new_crontab.c_str(), 1, new_crontab.size(), pipe);
            pclose(pipe);
        }
#elif defined(__APPLE__)
        register_cron_job_macos(path, schedule, title);
#elif defined(_WIN32)
        register_cron_job_windows(path, schedule, title);
#endif
    }

    void remove_cron_job(const std::string& title) {
#if defined(__linux__)
        std::string current_crontab = exec("crontab -l 2>/dev/null");
        std::stringstream ss(current_crontab);
        std::string line;
        std::string new_crontab;
        std::string marker = "# Meta-cron: " + title;
        bool skip_next = false;

        while (std::getline(ss, line)) {
            if (line == marker) {
                skip_next = true;
                continue;
            }
            if (skip_next) {
                skip_next = false;
                continue;
            }
            new_crontab += line + "\n";
        }

        FILE* pipe = popen("crontab -", "w");
        if (pipe) {
            fwrite(new_crontab.c_str(), 1, new_crontab.size(), pipe);
            pclose(pipe);
        }
#elif defined(__APPLE__)
        remove_cron_job_macos(title);
#elif defined(_WIN32)
        remove_cron_job_windows(title);
#endif
    }

    static std::string shell_escape(const std::string& s) {
        std::string result = "'";
        for (char c : s) {
            if (c == '\'') {
                result += "'\\''";
            } else {
                result += c;
            }
        }
        result += "'";
        return result;
    }

    static std::string exec(const char* cmd) {
        char buffer[128];
        std::string result = "";
        FILE* pipe = popen(cmd, "r");
        if (!pipe) return "";
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
        return result;
    }

#if defined(__APPLE__)
    void register_cron_job_macos(const std::string& path, const std::string& schedule, const std::string& title) {
        remove_cron_job_macos(title);

        char exe_path[4096];
        uint32_t size = sizeof(exe_path);
        if (_NSGetExecutablePath(exe_path, &size) != 0) {
            strcpy(exe_path, "metascript");
        }

        char abs_script_path[4096];
        if (realpath(path.c_str(), abs_script_path) == nullptr) {
            strncpy(abs_script_path, path.c_str(), sizeof(abs_script_path));
        }

        std::string label = "Meta.cron." + title;
        std::string plist_path = std::string(getenv("HOME")) + "/Library/LaunchAgents/" + label + ".plist";

        cron_parser::cron_fields fields = cron_parser::parse_expression(schedule);

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        ss << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
        ss << "<plist version=\"1.0\">\n";
        ss << "<dict>\n";
        ss << "    <key>Label</key>\n";
        ss << "    <string>" << label << "</string>\n";
        ss << "    <key>ProgramArguments</key>\n";
        ss << "    <array>\n";
        ss << "        <string>" << exe_path << "</string>\n";
        ss << "        <string>run</string>\n";
        ss << "        <string>--cron-title=" << title << "</string>\n";
        ss << "        <string>--cron-period=" << schedule << "</string>\n";
        ss << "        <string>" << abs_script_path << "</string>\n";
        ss << "    </array>\n";
        ss << "    <key>StartCalendarInterval</key>\n";
        ss << "    <array>\n";

        // Improved Cartesian product of fields (avoid expanding wildcards)
        // launchd: Absence of a key means "all values".
        std::vector<int> months = fields.months.size() == 12 ? std::vector<int>{-1} : std::vector<int>(fields.months.begin(), fields.months.end());
        std::vector<int> days = fields.days_of_month.size() == 31 ? std::vector<int>{-1} : std::vector<int>(fields.days_of_month.begin(), fields.days_of_month.end());
        std::vector<int> dows = fields.days_of_week.size() == 7 ? std::vector<int>{-1} : std::vector<int>(fields.days_of_week.begin(), fields.days_of_week.end());
        std::vector<int> hours = fields.hours.size() == 24 ? std::vector<int>{-1} : std::vector<int>(fields.hours.begin(), fields.hours.end());
        std::vector<int> minutes = fields.minutes.size() == 60 ? std::vector<int>{-1} : std::vector<int>(fields.minutes.begin(), fields.minutes.end());

        for (int month : months) {
            for (int day : days) {
                for (int dow : dows) {
                    for (int hour : hours) {
                        for (int min : minutes) {
                            ss << "        <dict>\n";
                            if (month != -1) ss << "            <key>Month</key><integer>" << month << "</integer>\n";
                            if (day != -1) ss << "            <key>Day</key><integer>" << day << "</integer>\n";
                            if (dow != -1) ss << "            <key>Weekday</key><integer>" << dow << "</integer>\n";
                            if (hour != -1) ss << "            <key>Hour</key><integer>" << hour << "</integer>\n";
                            if (min != -1) ss << "            <key>Minute</key><integer>" << min << "</integer>\n";
                            ss << "        </dict>\n";
                        }
                    }
                }
            }
        }

        ss << "    </array>\n";
        ss << "    <key>StandardOutPath</key>\n";
        ss << "    <string>/tmp/" << label << ".stdout.log</string>\n";
        ss << "    <key>StandardErrorPath</key>\n";
        ss << "    <string>/tmp/" << label << ".stderr.log</string>\n";
        ss << "</dict>\n";
        ss << "</plist>\n";

        std::ofstream ofs(plist_path);
        ofs << ss.str();
        ofs.close();

        std::string cmd = "launchctl bootstrap gui/$(id -u) " + shell_escape(plist_path);
        system(cmd.c_str());
    }

    void remove_cron_job_macos(const std::string& title) {
        std::string label = "Meta.cron." + title;
        std::string plist_path = std::string(getenv("HOME")) + "/Library/LaunchAgents/" + label + ".plist";

        std::string cmd = "launchctl bootout gui/$(id -u) " + shell_escape(plist_path) + " 2>/dev/null";
        system(cmd.c_str());

        remove(plist_path.c_str());
    }
#endif

#if defined(_WIN32)
    void register_cron_job_windows(const std::string& path, const std::string& schedule, const std::string& title) {
        remove_cron_job_windows(title);

        char exe_path[MAX_PATH];
        GetModuleFileNameA(NULL, exe_path, MAX_PATH);

        char abs_script_path[MAX_PATH];
        _fullpath(abs_script_path, path.c_str(), MAX_PATH);

        std::string task_name = "Meta-cron-" + title;
        cron_parser::cron_fields fields = cron_parser::parse_expression(schedule);

        // Improved check for 48 triggers limit (avoid counting wildcards)
        size_t count_months = fields.months.size() == 12 ? 1 : fields.months.size();
        size_t count_days = fields.days_of_month.size() == 31 ? 1 : fields.days_of_month.size();
        size_t count_dows = fields.days_of_week.size() == 7 ? 1 : fields.days_of_week.size();
        size_t count_hours = fields.hours.size() == 24 ? 1 : fields.hours.size();

        size_t trigger_count = count_months * count_hours;
        if (fields.dom_restricted && fields.dow_restricted) {
            trigger_count *= (count_days + count_dows);
        } else {
            trigger_count *= (fields.dom_restricted ? count_days : count_dows);
        }

        bool use_repetition = false;
        if (fields.minutes.size() > 0 && fields.minutes.size() < 60) {
            // Check if it's a step that divides 60
            if (60 % (60 / fields.minutes.size()) == 0) {
                 use_repetition = true;
            } else {
                 trigger_count *= fields.minutes.size();
            }
        }

        if (trigger_count > 48) {
            throw std::runtime_error("Cron expression exceeds 48 triggers limit on Windows");
        }

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n";
        ss << "<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n";
        ss << "  <RegistrationInfo>\n";
        ss << "    <Description>MetaScript cron job: " << title << "</Description>\n";
        ss << "  </RegistrationInfo>\n";
        ss << "  <Triggers>\n";

        // Improved XML generation for triggers (POSIX compliant OR for DOM/DOW)
        std::vector<int> hours = fields.hours.size() == 24 ? std::vector<int>{-1} : std::vector<int>(fields.hours.begin(), fields.hours.end());

        for (int hour : hours) {
            auto write_trigger = [&](bool use_dom, bool use_dow) {
                ss << "    <CalendarTrigger>\n";
                ss << "      <StartBoundary>2025-01-01T" << std::setfill('0') << std::setw(2) << (hour >= 0 ? hour : 0) << ":00:00</StartBoundary>\n";

                if (use_dom) {
                    ss << "      <ScheduleByMonth>\n";
                    ss << "        <DaysOfMonth>\n";
                    for (int day : fields.days_of_month) ss << "          <Day>" << day << "</Day>\n";
                    ss << "        </DaysOfMonth>\n";
                    ss << "        <Months>\n";
                    for (int month : (fields.months.size() == 12 ? std::set<int>{1,2,3,4,5,6,7,8,9,10,11,12} : fields.months))
                        ss << "          <" << month_to_win(month) << " />\n";
                    ss << "        </Months>\n";
                    ss << "      </ScheduleByMonth>\n";
                } else if (use_dow) {
                    ss << "      <ScheduleByWeek>\n";
                    ss << "        <DaysOfWeek>\n";
                    for (int dow : fields.days_of_week) ss << "          <" << dow_to_win(dow) << " />\n";
                    ss << "        </DaysOfWeek>\n";
                    ss << "        <WeeksInterval>1</WeeksInterval>\n";
                    ss << "      </ScheduleByWeek>\n";
                } else {
                    ss << "      <ScheduleByDay><DaysInterval>1</DaysInterval></ScheduleByDay>\n";
                }

                if (use_repetition) {
                    int interval = 60 / fields.minutes.size();
                    ss << "      <Repetition>\n";
                    ss << "        <Interval>PT" << interval << "M</Interval>\n";
                    ss << "        <Duration>P1D</Duration>\n";
                    ss << "      </Repetition>\n";
                }
                ss << "    </CalendarTrigger>\n";
            };

            if (fields.dom_restricted && fields.dow_restricted) {
                write_trigger(true, false);
                write_trigger(false, true);
            } else {
                write_trigger(fields.dom_restricted, fields.dow_restricted);
            }
        }

        ss << "  </Triggers>\n";
        ss << "  <Principals>\n";
        ss << "    <Principal id=\"Author\">\n";
        ss << "      <LogonType>S4U</LogonType>\n";
        ss << "    </Principal>\n";
        ss << "  </Principals>\n";
        ss << "  <Settings>\n";
        ss << "    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>\n";
        ss << "    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\n";
        ss << "    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\n";
        ss << "    <Enabled>true</Enabled>\n";
        ss << "  </Settings>\n";
        ss << "  <Actions Context=\"Author\">\n";
        ss << "    <Exec>\n";
        ss << "      <Command>\"" << exe_path << "\"</Command>\n";
        ss << "      <Arguments>run --cron-title=\"" << title << "\" --cron-period=\"" << schedule << "\" \"" << abs_script_path << "\"</Arguments>\n";
        ss << "    </Exec>\n";
        ss << "  </Actions>\n";
        ss << "</Task>";

        std::string xml_path = "temp_task.xml";
        std::ofstream ofs(xml_path);
        ofs << ss.str();
        ofs.close();

        std::string cmd = "schtasks /create /xml " + win_escape(xml_path) + " /tn " + win_escape(task_name) + " /f";
        system(cmd.c_str());
        remove(xml_path.c_str());
    }

    void remove_cron_job_windows(const std::string& title) {
        std::string task_name = "Meta-cron-" + title;
        std::string cmd = "schtasks /delete /tn " + win_escape(task_name) + " /f 2>nul";
        system(cmd.c_str());
    }
#endif

    static std::string month_to_win(int m) {
        static const char* months[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        return months[m];
    }

    static std::string dow_to_win(int d) {
        static const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        return days[d];
    }

    static std::string win_escape(const std::string& s) {
        std::string result = "\"";
        for (char c : s) {
            if (c == '"') {
                result += "\"\"";
            } else {
                result += c;
            }
        }
        result += "\"";
        return result;
    }

    static int execute_scheduled(const std::string& script_path, const std::string& title, const std::string& period) {
        // In a real implementation, we would load the script and call its scheduled() method.
        // For this demo/task, we'll simulate the environment.
        std::cout << "Executing scheduled job: " << title << " (" << period << ") at " << script_path << std::endl;

        // Simulating the Cloudflare Workers Cron Triggers API
        // export default { scheduled(controller) { ... } }

        // For MetaScript, we might use another webview instance or a JS engine to run the script.
        // Since we are building the runtime that binds to WebView, let's use a hidden webview to run the script.

        try {
            webview::webview w(false, nullptr);
            std::string js = "const controller = { cron: " + webview::detail::json_escape(period) +
                             ", type: 'scheduled', scheduledTime: " + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000) + " };\n";

            // Minimal shim to "import" the script and call scheduled
            // Assuming the script is a ES module as described.
            // Since we're in a webview, we can't easily read local files without a server or --allow-file-access-from-files.
            // But we can read it in C++ and inject it.

            std::ifstream ifs(script_path);
            if (!ifs.is_open()) {
                std::cerr << "Failed to open script: " << script_path << std::endl;
                return 1;
            }
            std::string script_content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

            // Improved module loading using data URI and dynamic import
            // This is safer and supports 'export default' correctly.
            std::string encoded_script;
            // Simplified base64 or just use escape
            std::string escaped_script = "";
            for (char c : script_content) {
                if (c == '`') escaped_script += "\\`";
                else if (c == '\\') escaped_script += "\\\\";
                else if (c == '$') escaped_script += "\\$";
                else escaped_script += c;
            }

            js += "const script = `" + escaped_script + "`;\n";
            js += "const blob = new Blob([script], { type: 'application/javascript' });\n";
            js += "const url = URL.createObjectURL(blob);\n";
            js += "import(url).then(m => {\n";
            js += "  const worker = m.default;\n";
            js += "  if (worker && typeof worker.scheduled === 'function') {\n";
            js += "    return Promise.resolve(worker.scheduled(controller));\n";
            js += "  }\n";
            js += "}).then(() => { window.__done(); }).catch(e => { console.error(e); window.__done(); });\n";

            bool finished = false;
            w.bind("__done", [&](const std::string&) -> std::string {
                finished = true;
                return "";
            });

            w.init(js);
            w.set_html("<html><body>Cron Worker</body></html>");

            // We need to run the event loop until __done is called
            // In webview.h, run() blocks. We might need a different approach if we want to wait for the promise.
            // But here we can just use the fact that webview_run() will process the init script.
            // Actually, we need to wait.

            // Since this is a CLI mode, we can just run it.
            // If the script is async, __done will be called later.
            // webview::webview doesn't have a "run until" but we can use terminate() from the binding.

            w.bind("__done", [&](const std::string&) -> std::string {
                w.terminate();
                finished = true;
                return "";
            });

            w.run();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error executing scheduled job: " << e.what() << std::endl;
            return 1;
        }
    }
};

} // namespace metascript

#endif // METASCRIPT_RUNTIME_HH
