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
                    result += "{\"__blob\":\"";
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
    void bind_blob(int index, const void* data, int n) { sqlite3_bind_blob(m_stmt, index, data, n, SQLITE_TRANSIENT); }
    void bind_null(int index) { sqlite3_bind_null(m_stmt, index); }

    std::string to_string() {
        char* expanded = sqlite3_expanded_sql(m_stmt);
        std::string res = expanded ? expanded : "";
        sqlite3_free(expanded);
        return res;
    }

    int column_count() { return sqlite3_column_count(m_stmt); }
    std::string column_name(int i) { return sqlite3_column_name(m_stmt, i); }

private:
    sqlite3_stmt* m_stmt = nullptr;
};

class sqlite_db {
public:
    sqlite_db(const std::string& filename, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) {
        if (sqlite3_open_v2(filename.c_str(), &m_db, flags, nullptr) != SQLITE_OK) {
            std::string err = sqlite3_errmsg(m_db);
            sqlite3_close(m_db);
            m_db = nullptr;
            throw std::runtime_error(err);
        }
    }
    ~sqlite_db() {
        if (m_db) sqlite3_close(m_db);
    }

    sqlite3* get_native() { return m_db; }
    int64_t last_insert_rowid() { return sqlite3_last_insert_rowid(m_db); }
    int changes() { return sqlite3_changes(m_db); }

    void exec(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error(msg);
        }
    }

    std::vector<unsigned char> serialize() {
        sqlite3_int64 sz = 0;
        unsigned char* data = sqlite3_serialize(m_db, "main", &sz, 0);
        std::vector<unsigned char> res(data, data + (size_t)sz);
        sqlite3_free(data);
        return res;
    }

    void deserialize(const std::vector<unsigned char>& data) {
        unsigned char* buf = (unsigned char*)sqlite3_malloc64(data.size());
        memcpy(buf, data.data(), data.size());
        sqlite3_deserialize(m_db, "main", buf, data.size(), data.size(), SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_READWRITE);
    }

    void file_control(int op, void* arg) {
        sqlite3_file_control(m_db, "main", op, arg);
    }

    void load_extension(const std::string& path) {
        sqlite3_enable_load_extension(m_db, 1);
        char* err = nullptr;
        if (sqlite3_load_extension(m_db, path.c_str(), nullptr, &err) != SQLITE_OK) {
            std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error(msg);
        }
    }

private:
    sqlite3* m_db = nullptr;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_SQLITE_HH
