#ifndef ALLOY_CRON_MANAGER_HPP
#define ALLOY_CRON_MANAGER_HPP

#include <string>

namespace alloy {

class cron_manager {
public:
    static void register_job(const std::string& path, const std::string& schedule, const std::string& title);
    static void remove_job(const std::string& title);
};

} // namespace alloy

#endif // ALLOY_CRON_MANAGER_HPP
