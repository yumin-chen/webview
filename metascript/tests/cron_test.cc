#include "../cron_parser.hh"
#include <cassert>
#include <iostream>

void test_nicknames() {
    auto fields = metascript::cron_parser::parse_expression("@daily");
    assert(fields.minutes.count(0));
    assert(fields.hours.count(0));
    assert(fields.days_of_month.size() == 31);
    assert(fields.months.size() == 12);
    assert(fields.days_of_week.size() == 7);
    std::cout << "test_nicknames passed" << std::endl;
}

void test_names() {
    auto fields = metascript::cron_parser::parse_expression("0 9 * * MON-FRI");
    assert(fields.days_of_week.size() == 5);
    assert(fields.days_of_week.count(1));
    assert(fields.days_of_week.count(5));
    assert(!fields.days_of_week.count(0));
    std::cout << "test_names passed" << std::endl;
}

void test_next_occurrence() {
    // 2025-01-20 09:00:00 UTC is a Monday
    std::tm t = {};
    t.tm_year = 125;
    t.tm_mon = 0;
    t.tm_mday = 20;
    t.tm_hour = 9;
    t.tm_min = 0;
    std::time_t start = timegm(&t);

    // Next weekday at 9:30 AM
    std::time_t next = metascript::cron_parser::parse("30 9 * * MON-FRI", start);
    std::tm nt_buf;
#ifdef _WIN32
    gmtime_s(&nt_buf, &next);
#else
    gmtime_r(&next, &nt_buf);
#endif
    std::tm *nt = &nt_buf;
    assert(nt->tm_hour == 9);
    assert(nt->tm_min == 30);
    assert(nt->tm_mday == 20);

    // If we are at 9:31 AM, it should be the next day
    next = metascript::cron_parser::parse("30 9 * * MON-FRI", start + 31 * 60);
#ifdef _WIN32
    gmtime_s(&nt_buf, &next);
#else
    gmtime_r(&next, &nt_buf);
#endif
    assert(nt->tm_hour == 9);
    assert(nt->tm_min == 30);
    assert(nt->tm_mday == 21);

    std::cout << "test_next_occurrence passed" << std::endl;
}

int main() {
    test_nicknames();
    test_names();
    test_next_occurrence();
    std::cout << "All cron parser tests passed!" << std::endl;
    return 0;
}
