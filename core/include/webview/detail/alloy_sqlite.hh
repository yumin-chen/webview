#ifndef WEBVIEW_DETAIL_ALLOY_SQLITE_HH
#define WEBVIEW_DETAIL_ALLOY_SQLITE_HH

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include "json.hh"

namespace webview {
namespace detail {

class AlloySQLite {
public:
    struct Options {
        bool readonly = false;
        bool create = true;
        bool readwrite = true;
        bool safeIntegers = false;
        bool strict = false;
    };

    class Statement {
    public:
        Statement(sqlite3* db, const std::string& sql, bool safeIntegers = false)
            : m_db(db), m_safe_integers(safeIntegers) {
            if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &m_stmt, nullptr) != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(m_db));
            }
        }

        ~Statement() {
            sqlite3_finalize(m_stmt);
        }

        sqlite3_stmt* get() { return m_stmt; }

        void reset() {
            sqlite3_reset(m_stmt);
            sqlite3_clear_bindings(m_stmt);
        }

        void bind_positional(int idx, const std::string& json_val) {
            if (json_val == "null" || json_val.empty()) {
                sqlite3_bind_null(m_stmt, idx);
            } else if (json_val == "true") {
                sqlite3_bind_int(m_stmt, idx, 1);
            } else if (json_val == "false") {
                sqlite3_bind_int(m_stmt, idx, 0);
            } else if (json_val.size() >= 2 && json_val.front() == '"' && json_val.back() == '"') {
                std::string s = json_val.substr(1, json_val.size() - 2);
                sqlite3_bind_text(m_stmt, idx, s.c_str(), -1, SQLITE_TRANSIENT);
            } else {
                try {
                    if (json_val.find('.') != std::string::npos || json_val.find('e') != std::string::npos || json_val.find('E') != std::string::npos) {
                        sqlite3_bind_double(m_stmt, idx, std::stod(json_val));
                    } else {
                        sqlite3_bind_int64(m_stmt, idx, std::stoll(json_val));
                    }
                } catch (...) {
                    sqlite3_bind_text(m_stmt, idx, json_val.c_str(), -1, SQLITE_TRANSIENT);
                }
            }
        }

        void bind_named(const std::string& name, const std::string& json_val) {
            int idx = sqlite3_bind_parameter_index(m_stmt, name.c_str());
            if (idx > 0) {
                bind_positional(idx, json_val);
            }
        }

        std::string to_sql() {
            char* sql = sqlite3_expanded_sql(m_stmt);
            std::string res = sql ? sql : "";
            if (sql) sqlite3_free(sql);
            return res;
        }

        bool is_safe_integers() const { return m_safe_integers; }

        std::vector<std::string> column_names() {
            std::vector<std::string> names;
            int count = sqlite3_column_count(m_stmt);
            for (int i = 0; i < count; ++i) {
                names.push_back(sqlite3_column_name(m_stmt, i));
            }
            return names;
        }

        int params_count() {
            return sqlite3_bind_parameter_count(m_stmt);
        }

    private:
        sqlite3* m_db;
        sqlite3_stmt* m_stmt;
        bool m_safe_integers;
    };

    AlloySQLite(const std::string& filename, const Options& opts) : m_opts(opts) {
        int flags = 0;
        if (opts.readonly) {
            flags |= SQLITE_OPEN_READONLY;
        } else {
            flags |= SQLITE_OPEN_READWRITE;
            if (opts.create) flags |= SQLITE_OPEN_CREATE;
        }

        if (sqlite3_open_v2(filename.c_str(), &m_db, flags, nullptr) != SQLITE_OK) {
            std::string err = sqlite3_errmsg(m_db);
            sqlite3_close(m_db);
            throw std::runtime_error("Failed to open database: " + err);
        }
    }

    ~AlloySQLite() {
        m_stmt_cache.clear();
        sqlite3_close(m_db);
    }

    std::shared_ptr<Statement> prepare(const std::string& sql) {
        return std::make_shared<Statement>(m_db, sql, m_opts.safeIntegers);
    }

    std::shared_ptr<Statement> query(const std::string& sql) {
        auto it = m_stmt_cache.find(sql);
        if (it != m_stmt_cache.end()) {
            it->second->reset();
            return it->second;
        }
        auto stmt = prepare(sql);
        m_stmt_cache[sql] = stmt;
        return stmt;
    }

    void exec(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error(msg);
        }
    }

    std::vector<uint8_t> serialize() {
        sqlite3_int64 size = 0;
        unsigned char* data = sqlite3_serialize(m_db, "main", &size, 0);
        if (!data) return {};
        std::vector<uint8_t> res(data, data + size);
        sqlite3_free(data);
        return res;
    }

    int file_control(int op, void* arg) {
        return sqlite3_file_control(m_db, "main", op, arg);
    }

    sqlite3* get_db() { return m_db; }
    const Options& get_options() const { return m_opts; }

private:
    sqlite3* m_db;
    Options m_opts;
    std::unordered_map<std::string, std::shared_ptr<Statement>> m_stmt_cache;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_ALLOY_SQLITE_HH
