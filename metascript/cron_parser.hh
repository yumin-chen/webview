#ifndef METASCRIPT_CRON_PARSER_HH
#define METASCRIPT_CRON_PARSER_HH

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace metascript {

class cron_parser {
public:
  struct cron_fields {
    std::set<int> minutes;
    std::set<int> hours;
    std::set<int> days_of_month;
    std::set<int> months;
    std::set<int> days_of_week;
    bool dom_restricted = false;
    bool dow_restricted = false;
  };

  static cron_fields parse_expression(const std::string &expression) {
    std::string expr = expression;
    if (expr.empty())
      return {};

    if (expr[0] == '@') {
      if (expr == "@yearly" || expr == "@annually")
        expr = "0 0 1 1 *";
      else if (expr == "@monthly")
        expr = "0 0 1 * *";
      else if (expr == "@weekly")
        expr = "0 0 * * 0";
      else if (expr == "@daily" || expr == "@midnight")
        expr = "0 0 * * *";
      else if (expr == "@hourly")
        expr = "0 * * * *";
    }

    std::vector<std::string> parts;
    std::stringstream ss(expr);
    std::string part;
    while (ss >> part) {
      parts.push_back(part);
    }

    if (parts.size() != 5) {
      throw std::runtime_error("Invalid cron expression: must have 5 fields");
    }

    cron_fields fields;
    fields.minutes = parse_field(parts[0], 0, 59);
    fields.hours = parse_field(parts[1], 0, 23);
    fields.days_of_month = parse_field(parts[2], 1, 31);
    fields.months = parse_field(parts[3], 1, 12, month_names);
    fields.days_of_week = parse_field(parts[4], 0, 7, day_names);

    // Standard cron behavior: if both dom and dow are restricted, it's an OR
    // match.
    fields.dom_restricted = (parts[2] != "*");
    fields.dow_restricted = (parts[4] != "*");

    // Normalize days of week (0 and 7 are both Sunday)
    if (fields.days_of_week.count(7)) {
      fields.days_of_week.insert(0);
      fields.days_of_week.erase(7);
    }

    return fields;
  }

  static std::time_t parse(const std::string &expression,
                           std::time_t relative_time = std::time(nullptr)) {
    cron_fields fields = parse_expression(expression);
    return next_occurrence(fields, relative_time);
  }

  static std::time_t next_occurrence(const cron_fields &fields,
                                     std::time_t relative_time) {
    // Start from the next minute
    std::time_t current = relative_time + 60;
    std::tm tm_buf;
    std::tm *t = gmtime_portable(&current, &tm_buf);
    t->tm_sec = 0;
    current = timegm(t);

    // Limit search to ~4 years
    std::time_t end = relative_time + (4 * 366 * 24 * 60 * 60);

    while (current < end) {
      t = gmtime_portable(&current, &tm_buf);

      if (fields.months.find(t->tm_mon + 1) == fields.months.end()) {
        t->tm_mon++;
        t->tm_mday = 1;
        t->tm_hour = 0;
        t->tm_min = 0;
        current = timegm(t);
        continue;
      }

      bool dom_match = fields.days_of_month.find(t->tm_mday) != fields.days_of_month.end();
      bool dow_match = fields.days_of_week.find(t->tm_wday) != fields.days_of_week.end();

      bool date_match = false;
      if (fields.dom_restricted && fields.dow_restricted) {
        date_match = dom_match || dow_match;
      } else {
        date_match = dom_match && dow_match;
      }

      if (!date_match) {
        t->tm_mday++;
        t->tm_hour = 0;
        t->tm_min = 0;
        current = timegm(t);
        continue;
      }

      if (fields.hours.find(t->tm_hour) == fields.hours.end()) {
        t->tm_hour++;
        t->tm_min = 0;
        current = timegm(t);
        continue;
      }

      if (fields.minutes.find(t->tm_min) == fields.minutes.end()) {
        t->tm_min++;
        current = timegm(t);
        continue;
      }

      return current;
    }

    return 0; // Not found
  }

private:
  static std::tm* gmtime_portable(const std::time_t* timep, std::tm* result) {
#ifdef _WIN32
    gmtime_s(result, timep);
    return result;
#else
    return gmtime_r(timep, result);
#endif
  }

private:
  static inline std::map<std::string, int> month_names = {
      {"JAN", 1}, {"FEB", 2},  {"MAR", 3},  {"APR", 4},  {"MAY", 5},  {"JUN", 6},
      {"JUL", 7}, {"AUG", 8},  {"SEP", 9},  {"OCT", 10}, {"NOV", 11}, {"DEC", 12},
      {"JANUARY", 1}, {"FEBRUARY", 2},  {"MARCH", 3},  {"APRIL", 4},  {"MAY", 5},  {"JUNE", 6},
      {"JULY", 7}, {"AUGUST", 8},  {"SEPTEMBER", 9},  {"OCTOBER", 10}, {"NOVEMBER", 11}, {"DECEMBER", 12}
  };

  static inline std::map<std::string, int> day_names = {
      {"SUN", 0}, {"MON", 1}, {"TUE", 2}, {"WED", 3}, {"THU", 4}, {"FRI", 5}, {"SAT", 6},
      {"SUNDAY", 0}, {"MONDAY", 1}, {"TUESDAY", 2}, {"WEDNESDAY", 3}, {"THURSDAY", 4}, {"FRIDAY", 5}, {"SATURDAY", 6}
  };

  static std::set<int> parse_field(const std::string &field, int min_val,
                                   int max_val,
                                   const std::map<std::string, int> &names = {}) {
    std::set<int> result;
    std::stringstream ss(field);
    std::string item;
    while (std::getline(ss, item, ',')) {
      if (item == "*") {
        for (int i = min_val; i <= max_val; ++i)
          result.insert(i);
      } else {
        size_t slash = item.find('/');
        int step = 1;
        std::string range_part = item;
        if (slash != std::string::npos) {
          step = std::stoi(item.substr(slash + 1));
          range_part = item.substr(0, slash);
        }

        int start, end;
        if (range_part == "*") {
          start = min_val;
          end = max_val;
        } else {
          size_t dash = range_part.find('-');
          if (dash != std::string::npos) {
            start = parse_value(range_part.substr(0, dash), names);
            end = parse_value(range_part.substr(dash + 1), names);
          } else {
            start = end = parse_value(range_part, names);
          }
        }

        for (int i = start; i <= end; i += step) {
          result.insert(i);
        }
      }
    }
    return result;
  }

  static int parse_value(const std::string &val,
                         const std::map<std::string, int> &names) {
    if (val.empty()) return 0;
    if (std::isdigit(val[0]) || (val.size() > 1 && val[0] == '-')) {
        return std::stoi(val);
    }
    std::string upper_val = val;
    std::transform(upper_val.begin(), upper_val.end(), upper_val.begin(),
                   ::toupper);
    if (names.count(upper_val)) {
      return names.at(upper_val);
    }
    throw std::runtime_error("Invalid value in cron expression: " + val);
  }
};

} // namespace metascript

#endif // METASCRIPT_CRON_PARSER_HH
