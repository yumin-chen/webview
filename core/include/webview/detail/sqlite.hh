#ifndef WEBVIEW_DETAIL_SQLITE_HH
#define WEBVIEW_DETAIL_SQLITE_HH

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <stdexcept>
#include "json.hh"

namespace webview {
namespace detail {

class sqlite_stmt {
public:
    sqlite_stmt(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &m_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
    }
    ~sqlite_stmt() {
        if (m_stmt) sqlite3_finalize(m_stmt);
    }

    std::string step(bool safe_integers) {
        int rc = sqlite3_step(m_stmt);
        if (rc == SQLITE_ROW) {
            std::string result = "{";
            int count = sqlite3_column_count(m_stmt);
            for (int i = 0; i < count; ++i) {
                if (i > 0) result += ",";
                result += "\"" + std::string(sqlite3_column_name(m_stmt, i)) + "\":";
                int type = sqlite3_column_type(m_stmt, i);
                if (type == SQLITE_TEXT) {
                    result += json_escape((const char*)sqlite3_column_text(m_stmt, i));
                } else if (type == SQLITE_INTEGER) {
                    long long val = sqlite3_column_int64(m_stmt, i);
                    if (safe_integers) result += "\"" + std::to_string(val) + "n\"";
                    else result += std::to_string(val);
                } else if (type == SQLITE_FLOAT) {
                    result += std::to_string(sqlite3_column_double(m_stmt, i));
                } else if (type == SQLITE_BLOB) {
                    const unsigned char* blob = (const unsigned char*)sqlite3_column_blob(m_stmt, i);
                    int bytes = sqlite3_column_bytes(m_stmt, i);
                    result += "{\"__blob\":\""; // Simple base64 placeholder
                    for (int j = 0; j < bytes; ++j) {
                        char buf[3]; sprintf(buf, "%02x", blob[j]); result += buf;
                    }
                    result += "\"}";
                } else {
                    result += "null";
                }
            }
            result += "}";
            return result;
        }
        return "";
    }

    void reset() { sqlite3_reset(m_stmt); }
    void bind_text(int index, const std::string& val) { sqlite3_bind_text(m_stmt, index, val.c_str(), -1, SQLITE_TRANSIENT); }
    void bind_int64(int index, int64_t val) { sqlite3_bind_int64(m_stmt, index, val); }
    void bind_double(int index, double val) { sqlite3_bind_double(m_stmt, index, val); }
    void bind_null(int index) { sqlite3_bind_null(m_stmt, index); }

private:
    sqlite3_stmt* m_stmt = nullptr;
};

class sqlite_db {
public:
    sqlite_db(const std::string& filename, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) {
        if (sqlite3_open_v2(filename.c_str(), &m_db, flags, nullptr) != SQLITE_OK) {
            std::string err = sqlite3_errmsg(m_db); sqlite3_close(m_db); m_db = nullptr;
            throw std::runtime_error(err);
        }
    }
    ~sqlite_db() { if (m_db) sqlite3_close(m_db); }
    sqlite3* get_native() { return m_db; }
private:
    sqlite3* m_db = nullptr;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_SQLITE_HH
