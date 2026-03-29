#include "../cron_parser.hpp"
#include <iostream>
#include <cassert>
#include <ctime>
#include <iomanip>

using namespace alloy;

std::string format_time(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
    gmtime_r(&t, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::chrono::system_clock::time_point make_time(int year, int mon, int day, int hour, int min) {
    std::tm tm_buf = {};
    tm_buf.tm_year = year - 1900;
    tm_buf.tm_mon = mon - 1;
    tm_buf.tm_mday = day;
    tm_buf.tm_hour = hour;
    tm_buf.tm_min = min;
    tm_buf.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(timegm(&tm_buf));
}

void test_parse_basic() {
    auto expr = cron_parser::parse("*/15 * * * *");
    assert(expr.minutes.count(0));
    assert(expr.minutes.count(15));
    assert(expr.minutes.count(30));
    assert(expr.minutes.count(45));
    assert(expr.hours.size() == 24);
    std::cout << "test_parse_basic passed" << std::endl;
}

void test_next_boundary() {
    auto expr = cron_parser::parse("*/15 * * * *");
    auto start = make_time(2025, 1, 1, 12, 0);
    auto next = cron_parser::next(expr, start);
    assert(format_time(next) == "2025-01-01T12:15:00Z");
    std::cout << "test_next_boundary passed" << std::endl;
}

void test_next_weekday() {
    // 30 9 * * MON-FRI
    // 2025-01-15 is Wednesday.
    auto expr = cron_parser::parse("30 9 * * MON-FRI");
    auto start = make_time(2025, 1, 15, 10, 0);
    auto next = cron_parser::next(expr, start);
    assert(format_time(next) == "2025-01-16T09:30:00Z"); // Thursday
    std::cout << "test_next_weekday passed" << std::endl;
}

void test_nicknames() {
    auto expr = cron_parser::parse("@daily");
    auto start = make_time(2025, 1, 15, 10, 0);
    auto next = cron_parser::next(expr, start);
    assert(format_time(next) == "2025-01-16T00:00:00Z");
    std::cout << "test_nicknames passed" << std::endl;
}

void test_dom_dow_interaction() {
    // Fires on the 15th of every month OR every Friday
    // 2025-01-15 is Wednesday. Next 15th is 2025-01-15.
    // Next Friday is 2025-01-17.
    auto expr = cron_parser::parse("0 0 15 * FRI");
    auto start = make_time(2025, 1, 15, 10, 0);
    auto next = cron_parser::next(expr, start);
    assert(format_time(next) == "2025-01-17T00:00:00Z");

    auto start2 = make_time(2025, 1, 14, 10, 0);
    auto next2 = cron_parser::next(expr, start2);
    assert(format_time(next2) == "2025-01-15T00:00:00Z");
    std::cout << "test_dom_dow_interaction passed" << std::endl;
}

int main() {
    test_parse_basic();
    test_next_boundary();
    test_next_weekday();
    test_nicknames();
    test_dom_dow_interaction();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
