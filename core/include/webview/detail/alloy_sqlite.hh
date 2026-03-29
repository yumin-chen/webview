#ifndef WEBVIEW_DETAIL_ALLOY_SQLITE_HH
#define WEBVIEW_DETAIL_ALLOY_SQLITE_HH

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

namespace webview {
namespace detail {

class AlloySQLite {
public:
    class Statement {
    public:
        Statement(sqlite3_stmt* stmt) : m_stmt(stmt) {}
        ~Statement() { sqlite3_finalize(m_stmt); }

        sqlite3_stmt* get() const { return m_stmt; }

        void bind(int index, int64_t val) { sqlite3_bind_int64(m_stmt, index, val); }
        void bind(int index, double val) { sqlite3_bind_double(m_stmt, index, val); }
        void bind(int index, const std::string& val) { sqlite3_bind_text(m_stmt, index, val.c_str(), -1, SQLITE_TRANSIENT); }
        void bind_null(int index) { sqlite3_bind_null(m_stmt, index); }
        void reset() { sqlite3_reset(m_stmt); sqlite3_clear_bindings(m_stmt); }

    private:
        sqlite3_stmt* m_stmt;
    };

    AlloySQLite(const std::string& filename, int flags) {
        if (sqlite3_open_v2(filename.c_str(), &m_db, flags, nullptr) != SQLITE_OK) {
            std::string err = sqlite3_errmsg(m_db);
            sqlite3_close(m_db);
            throw std::runtime_error("Failed to open database: " + err);
        }
    }

    ~AlloySQLite() {
        sqlite3_close(m_db);
    }

    std::shared_ptr<Statement> prepare(const std::string& sql) {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }
        return std::make_shared<Statement>(stmt);
    }

    sqlite3* get_db() const { return m_db; }

private:
    sqlite3* m_db;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOY_SQLITE_HH
