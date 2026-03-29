#ifndef ALLOY_CRON_PARSER_HPP
#define ALLOY_CRON_PARSER_HPP

#include <string>
#include <vector>
#include <chrono>
#include <set>

namespace alloy {

class cron_parser {
public:
    struct cron_expression {
        std::set<int> minutes;
        std::set<int> hours;
        std::set<int> days_of_month;
        std::set<int> months;
        std::set<int> days_of_week;
        bool dom_restricted = false;
        bool dow_restricted = false;
        std::string original_expression;
    };

    static cron_expression parse(const std::string& expression);
    static std::chrono::system_clock::time_point next(const cron_expression& expr, std::chrono::system_clock::time_point relative_to);

private:
    static std::set<int> parse_field(const std::string& field, int min_val, int max_val, const std::vector<std::string>& names = {});
    static std::string normalize_expression(const std::string& expression);
};

} // namespace alloy

#endif // ALLOY_CRON_PARSER_HPP
