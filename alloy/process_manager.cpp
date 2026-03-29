#include "process_manager.hpp"

namespace alloy {

process_manager& process_manager::instance() {
    static process_manager manager;
    return manager;
}

std::string process_manager::register_proc(std::unique_ptr<subprocess> proc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string id = std::to_string(m_next_id++);
    m_procs[id] = std::move(proc);
    return id;
}

subprocess* process_manager::get_proc(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_procs.find(id);
    if (it != m_procs.end()) return it->second.get();
    return nullptr;
}

void process_manager::unregister_proc(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_procs.erase(id);
}

std::map<std::string, subprocess*> process_manager::get_all_procs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, subprocess*> result;
    for (auto& pair : m_procs) {
        result[pair.first] = pair.second.get();
    }
    return result;
}

} // namespace alloy
