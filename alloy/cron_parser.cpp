#include "cron_parser.hpp"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <map>

namespace alloy {

#ifdef _WIN32
#define gmtime_r(t, tm) gmtime_s(tm, t)
static time_t timegm(struct tm* tm) { return _mkgmtime(tm); }
#endif

static const std::vector<std::string> MONTH_NAMES = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const std::vector<std::string> DAY_NAMES = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

std::string cron_parser::normalize_expression(const std::string& expression) {
    if (expression == "@yearly" || expression == "@annually") return "0 0 1 1 *";
    if (expression == "@monthly") return "0 0 1 * *";
    if (expression == "@weekly") return "0 0 * * 0";
    if (expression == "@daily" || expression == "@midnight") return "0 0 * * *";
    if (expression == "@hourly") return "0 * * * *";
    return expression;
}

cron_parser::cron_expression cron_parser::parse(const std::string& expression) {
    std::string norm_expr = normalize_expression(expression);
    std::istringstream iss(norm_expr);
    std::vector<std::string> fields;
    std::string field;
    while (iss >> field) {
        fields.push_back(field);
    }

    if (fields.size() != 5) {
        throw std::runtime_error("Invalid cron expression: expected 5 fields");
    }

    cron_expression expr;
    expr.original_expression = expression;
    expr.minutes = parse_field(fields[0], 0, 59);
    expr.hours = parse_field(fields[1], 0, 23);
    expr.days_of_month = parse_field(fields[2], 1, 31);
    expr.months = parse_field(fields[3], 1, 12, MONTH_NAMES);
    expr.days_of_week = parse_field(fields[4], 0, 7, DAY_NAMES);

    if (expr.days_of_week.count(7)) {
        expr.days_of_week.insert(0);
        expr.days_of_week.erase(7);
    }

    expr.dom_restricted = (fields[2] != "*");
    expr.dow_restricted = (fields[4] != "*");

    return expr;
}

std::set<int> cron_parser::parse_field(const std::string& field, int min_val, int max_val, const std::vector<std::string>& names) {
    std::set<int> values;
    std::string normalized_field = field;
    std::transform(normalized_field.begin(), normalized_field.end(), normalized_field.begin(), ::toupper);

    auto parse_val = [&](const std::string& s) -> int {
        if (std::all_of(s.begin(), s.end(), ::isdigit)) {
            return std::stoi(s);
        }
        for (size_t i = 0; i < names.size(); ++i) {
            if (s.find(names[i]) == 0) {
                return static_cast<int>(i) + (min_val == 1 ? 1 : 0);
            }
        }
        throw std::runtime_error("Invalid value in cron field: " + s);
    };

    std::istringstream iss(normalized_field);
    std::string part;
    while (std::getline(iss, part, ',')) {
        size_t slash_pos = part.find('/');
        int step = 1;
        std::string range_part = part;
        if (slash_pos != std::string::npos) {
            step = std::stoi(part.substr(slash_pos + 1));
            range_part = part.substr(0, slash_pos);
        }

        int start = min_val, end = max_val;
        if (range_part != "*") {
            size_t dash_pos = range_part.find('-');
            if (dash_pos != std::string::npos) {
                start = parse_val(range_part.substr(0, dash_pos));
                end = parse_val(range_part.substr(dash_pos + 1));
            } else {
                start = end = parse_val(range_part);
            }
        }

        for (int i = start; i <= end; i += step) {
            if (i >= min_val && i <= max_val) {
                values.insert(i);
            }
        }
    }
    return values;
}

std::chrono::system_clock::time_point cron_parser::next(const cron_expression& expr, std::chrono::system_clock::time_point relative_to) {
    std::time_t t = std::chrono::system_clock::to_time_t(relative_to);
    std::tm tm_buf;
    gmtime_r(&t, &tm_buf);

    tm_buf.tm_sec = 0;
    tm_buf.tm_min++;

    for (int i = 0; i < 4 * 366; ++i) {
        std::time_t current_t = timegm(&tm_buf);
        gmtime_r(&current_t, &tm_buf);

        if (expr.months.count(tm_buf.tm_mon + 1) == 0) {
            auto it = expr.months.lower_bound(tm_buf.tm_mon + 2);
            if (it == expr.months.end()) {
                tm_buf.tm_year++;
                tm_buf.tm_mon = *expr.months.begin() - 1;
            } else {
                tm_buf.tm_mon = *it - 1;
            }
            tm_buf.tm_mday = 1;
            tm_buf.tm_hour = 0;
            tm_buf.tm_min = 0;
            continue;
        }

        bool dom_match = expr.days_of_month.count(tm_buf.tm_mday) > 0;
        bool dow_match = expr.days_of_week.count(tm_buf.tm_wday) > 0;
        bool day_match = (expr.dom_restricted && expr.dow_restricted) ? (dom_match || dow_match) : (expr.dom_restricted ? dom_match : (expr.dow_restricted ? dow_match : true));

        if (!day_match) {
            tm_buf.tm_mday++;
            tm_buf.tm_hour = 0;
            tm_buf.tm_min = 0;
            continue;
        }

        if (expr.hours.count(tm_buf.tm_hour) == 0) {
            auto it = expr.hours.lower_bound(tm_buf.tm_hour + 1);
            if (it == expr.hours.end()) {
                tm_buf.tm_mday++;
                tm_buf.tm_hour = *expr.hours.begin();
            } else {
                tm_buf.tm_hour = *it;
            }
            tm_buf.tm_min = 0;
            continue;
        }

        if (expr.minutes.count(tm_buf.tm_min) == 0) {
            auto it = expr.minutes.lower_bound(tm_buf.tm_min + 1);
            if (it == expr.minutes.end()) {
                tm_buf.tm_hour++;
                tm_buf.tm_min = *expr.minutes.begin();
            } else {
                tm_buf.tm_min = *it;
            }
            continue;
        }

        return std::chrono::system_clock::from_time_t(timegm(&tm_buf));
    }

    throw std::runtime_error("No matching time found within 4 years");
}

} // namespace alloy
