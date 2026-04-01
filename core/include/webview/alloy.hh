#ifndef WEBVIEW_ALLOY_HH
#define WEBVIEW_ALLOY_HH

#include "detail/alloy_process.hh"
#include "detail/alloy_sqlite.hh"
#include "detail/alloy_js.hh"
#include "detail/json.hh"
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

namespace webview {

// Forward declaration of browser_engine is tricky because it's a typedef.
// We'll use the base class pointer if possible, or just include webview.h where needed.
class engine_base;

namespace detail {

class AlloyRuntime {
public:
    AlloyRuntime(void* w) : m_webview(w) {
        setup_bindings();
    }

    void setup_bindings();

    void on_process_exit(const std::string& id, int code, AlloyProcess::ResourceUsage usage);

    std::string base64_encode(const std::vector<char>& data);

private:
    void alloy_bind_sqlite_params(std::shared_ptr<AlloySQLite::Statement> stmt, const std::string& params_json);
    std::string alloy_row_to_json(std::shared_ptr<AlloySQLite::Statement> stmt, bool values_only);

    void* m_webview;
    std::map<std::string, std::shared_ptr<AlloyProcess>> m_processes;
    std::map<std::string, std::shared_ptr<AlloySQLite>> m_databases;
    std::map<std::string, std::shared_ptr<AlloySQLite::Statement>> m_statements;
    std::map<std::string, void*> m_signals;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_ALLOY_HH
