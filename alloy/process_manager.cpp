#include "process_manager.hpp"

namespace alloy {

process_manager& process_manager::instance() {
    static process_manager manager;
    return manager;
}

std::string process_manager::register_proc(std::shared_ptr<subprocess> proc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string id = std::to_string(m_next_id++);
    m_procs[id] = proc;
    return id;
}

std::shared_ptr<subprocess> process_manager::get_proc(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_procs.find(id);
    if (it != m_procs.end()) return it->second;
    return nullptr;
}

void process_manager::unregister_proc(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_procs.erase(id);
}

std::map<std::string, std::shared_ptr<subprocess>> process_manager::get_all_procs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_procs;
}

} // namespace alloy
