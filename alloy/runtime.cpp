#include "runtime.hpp"
#include "cron_parser.hpp"
#include "cron_manager.hpp"
#include "webview/json_deprecated.hh"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace alloy {

void runtime::init(webview::webview& w) {
    w.bind("Alloy_cron_parse", [&](const std::string& req) -> std::string {
        try {
            std::string expression = webview::json_parse(req, "", 0);
            if (expression.empty()) return "null";

            auto expr = cron_parser::parse(expression);
            auto now = std::chrono::system_clock::now();
            auto next_time = cron_parser::next(expr, now);

            std::time_t t = std::chrono::system_clock::to_time_t(next_time);
            std::tm tm_buf;
#ifdef _WIN32
            gmtime_s(&tm_buf, &t);
#else
            gmtime_r(&t, &tm_buf);
#endif
            std::ostringstream oss;
            oss << "\"" << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S.000Z") << "\"";
            return oss.str();
        } catch (...) {
            return "null";
        }
    });

    w.bind("Alloy_cron_register", [&](const std::string& req) -> std::string {
        try {
            std::string path = webview::json_parse(req, "", 0);
            std::string schedule = webview::json_parse(req, "", 1);
            std::string title = webview::json_parse(req, "", 2);

            if (path.empty() || schedule.empty() || title.empty()) return "false";

            cron_manager::register_job(path, schedule, title);
            return "true";
        } catch (...) {
            return "false";
        }
    });

    w.bind("Alloy_cron_remove", [&](const std::string& req) -> std::string {
        try {
            std::string title = webview::json_parse(req, "", 0);
            if (title.empty()) return "false";

            cron_manager::remove_job(title);
            return "true";
        } catch (...) {
            return "false";
        }
    });

    w.init(R"js(
        window.Alloy = window.Alloy || {};
        window.Alloy.cron = function(path, schedule, title) {
            return window.Alloy_cron_register(path, schedule, title);
        };
        window.Alloy.cron.parse = function(expression, relativeDate) {
            const result = window.Alloy_cron_parse(expression, relativeDate);
            return result ? new Date(result) : null;
        };
        window.Alloy.cron.remove = function(title) {
            return window.Alloy_cron_remove(title);
        };
    )js");
}

} // namespace alloy
