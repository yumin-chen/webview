#ifndef ALLOY_PROCESS_MANAGER_HPP
#define ALLOY_PROCESS_MANAGER_HPP

#include "subprocess.hpp"
#include <mutex>
#include <map>
#include <string>
#include <memory>

namespace alloy {

class process_manager {
public:
    static process_manager& instance();

    std::string register_proc(std::unique_ptr<subprocess> proc);
    subprocess* get_proc(const std::string& id);
    void unregister_proc(const std::string& id);

    std::map<std::string, subprocess*> get_all_procs();

private:
    process_manager() = default;
    std::mutex m_mutex;
    std::map<std::string, std::unique_ptr<subprocess>> m_procs;
    int m_next_id = 1;
};

} // namespace alloy

#endif // ALLOY_PROCESS_MANAGER_HPP
