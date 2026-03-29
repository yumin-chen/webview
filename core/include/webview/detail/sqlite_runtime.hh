/*
 * MIT License
 *
 * Copyright (c) 2017 Serge Zaitsev
 * Copyright (c) 2022 Steffen André Langnes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WEBVIEW_DETAIL_SQLITE_RUNTIME_HH
#define WEBVIEW_DETAIL_SQLITE_RUNTIME_HH

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include "json.hh"

namespace webview {
namespace detail {

class sqlite_runtime {
public:
        void exec(const std::string& db_id, const std::string& sql) {
            auto db = get_db(db_id);
            if (!db) throw std::runtime_error("Database not found");
            char* errmsg = nullptr;
            if (sqlite3_exec(db->get_native(), sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
                std::string err = errmsg;
                sqlite3_free(errmsg);
                throw std::runtime_error(err);
            }
        }
    class statement {
    public:
        statement(sqlite3* db, const std::string& sql) : m_db(db) {
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &m_stmt, nullptr) != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }

            void reset() {
                sqlite3_reset(m_stmt);
            }

            std::string step(bool safeIntegers) {
                int res = sqlite3_step(m_stmt);
                if (res == SQLITE_ROW) {
                    std::string out = "{";
                    int count = sqlite3_column_count(m_stmt);
                    for (int i = 0; i < count; ++i) {
                        if (i > 0) out += ",";
                        out += json_escape(sqlite3_column_name(m_stmt, i)) + ":";
                        out += column_to_json(i, safeIntegers);
                    }
                    out += "}";
                    return out;
                } else if (res == SQLITE_DONE) {
                    return "done";
                } else {
                    throw std::runtime_error(sqlite3_errmsg(m_db));
                }
            }

        ~statement() {
            if (m_stmt) {
                sqlite3_finalize(m_stmt);
            }
        }

        void bind(const std::string& params_json, bool strict) {
            sqlite3_reset(m_stmt);
            sqlite3_clear_bindings(m_stmt);

            if (params_json.empty() || params_json == "[]" || params_json == "{}") {
                return;
            }

            // Simple JSON detection: [ is array, { is object
            if (params_json[0] == '[') {
                int i = 0;
                while (true) {
                    std::string val = json_parse(params_json, "", i++);
                    if (val.empty() && i > 1) {
                        // Check if we actually have more elements by looking for commas
                        // This is a very crude way but json_parse is limited.
                        // For a better implementation, we'd need a real JSON parser.
                        break;
                    }
                    bind_value(i, val);
                }
            } else if (params_json[0] == '{') {
                int count = sqlite3_bind_parameter_count(m_stmt);
                for (int i = 1; i <= count; ++i) {
                    const char* name = sqlite3_bind_parameter_name(m_stmt, i);
                    if (name) {
                        std::string key = name;
                        std::string val = json_parse(params_json, key, 0);
                        if (val.empty() && strict) {
                            // Try without prefix if strict is true
                            if (key[0] == '$' || key[0] == ':' || key[0] == '@') {
                                val = json_parse(params_json, key.substr(1), 0);
                            }
                        }
                        if (!val.empty()) {
                            bind_value(i, val);
                        } else if (strict) {
                            throw std::runtime_error("Missing parameter: " + key);
                        }
                    }
                }
            }
        }

        void bind_value(int index, const std::string& val) {
            if (val == "null") {
                sqlite3_bind_null(m_stmt, index);
            } else if (val == "true") {
                sqlite3_bind_int(m_stmt, index, 1);
            } else if (val == "false") {
                sqlite3_bind_int(m_stmt, index, 0);
            } else if (val.size() > 1 && val[0] == '"') {
                std::string s = val.substr(1, val.size() - 2);
                if (s.size() >= 1 && s.back() == 'n' && s.find_first_not_of("-0123456789", 0, s.size() - 1) == std::string::npos) {
                    // It's a BigInt string from JS: "123n"
                    sqlite3_bind_int64(m_stmt, index, std::stoll(s.substr(0, s.size() - 1)));
                } else {
                    sqlite3_bind_text(m_stmt, index, s.c_str(), -1, SQLITE_TRANSIENT);
                }
            } else {
                try {
                    if (val.find('.') != std::string::npos || val.find('e') != std::string::npos) {
                        sqlite3_bind_double(m_stmt, index, std::stod(val));
                    } else {
                        sqlite3_bind_int64(m_stmt, index, std::stoll(val));
                    }
                } catch (...) {
                    sqlite3_bind_text(m_stmt, index, val.c_str(), -1, SQLITE_TRANSIENT);
                }
            }
        }

        std::string execute(const std::string& mode, bool safeIntegers) {
            if (mode == "run") {
                int res = sqlite3_step(m_stmt);
                if (res != SQLITE_DONE && res != SQLITE_ROW) {
                    throw std::runtime_error(sqlite3_errmsg(m_db));
                }
                std::string out = "{";
                out += "\"lastInsertRowid\":" + std::to_string(sqlite3_last_insert_rowid(m_db)) + ",";
                out += "\"changes\":" + std::to_string(sqlite3_changes(m_db));
                out += "}";
                return out;
            }

            std::string out;
            if (mode == "all" || mode == "values" || mode == "iterate") {
                out = "[";
            }

            bool first = true;
            int res;
            while ((res = sqlite3_step(m_stmt)) == SQLITE_ROW) {
                if (!first) out += ",";
                first = false;

                if (mode == "get" || mode == "all" || mode == "iterate") {
                    out += "{";
                    int count = sqlite3_column_count(m_stmt);
                    for (int i = 0; i < count; ++i) {
                        if (i > 0) out += ",";
                        out += json_escape(sqlite3_column_name(m_stmt, i)) + ":";
                        out += column_to_json(i, safeIntegers);
                    }
                    out += "}";
                    if (mode == "get") break;
                } else if (mode == "values") {
                    out += "[";
                    int count = sqlite3_column_count(m_stmt);
                    for (int i = 0; i < count; ++i) {
                        if (i > 0) out += ",";
                        out += column_to_json(i, safeIntegers);
                    }
                    out += "]";
                }
            }

            if (res != SQLITE_DONE && res != SQLITE_ROW) {
                throw std::runtime_error(sqlite3_errmsg(m_db));
            }

            if (mode == "get" && first) return "null";
            if (mode == "all" || mode == "values" || mode == "iterate") {
                out += "]";
            }
            return out;
        }

        std::string column_to_json(int i, bool safeIntegers) {
            int type = sqlite3_column_type(m_stmt, i);
            switch (type) {
                case SQLITE_INTEGER: {
                    sqlite3_int64 v = sqlite3_column_int64(m_stmt, i);
                    if (safeIntegers && (v > 9007199254740991LL || v < -9007199254740991LL)) {
                        return "\"" + std::to_string(v) + "n\""; // BigInt suffix for JS helper
                    }
                    return std::to_string(v);
                }
                case SQLITE_FLOAT:
                    return std::to_string(sqlite3_column_double(m_stmt, i));
                case SQLITE_TEXT:
                    return json_escape((const char*)sqlite3_column_text(m_stmt, i));
                case SQLITE_BLOB: {
                    const void* blob = sqlite3_column_blob(m_stmt, i);
                    int size = sqlite3_column_bytes(m_stmt, i);
                    // Return as base64 for JS to convert to Uint8Array
                    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                    std::string b = "\"base64:";
                    const unsigned char* p = (const unsigned char*)blob;
                    int j = 0;
                    for (; j + 2 < size; j += 3) {
                        b += base64_chars[p[j] >> 2];
                        b += base64_chars[((p[j] & 0x03) << 4) | (p[j+1] >> 4)];
                        b += base64_chars[((p[j+1] & 0x0f) << 2) | (p[j+2] >> 6)];
                        b += base64_chars[p[j+2] & 0x3f];
                    }
                    if (j < size) {
                        b += base64_chars[p[j] >> 2];
                        if (j + 1 < size) {
                            b += base64_chars[((p[j] & 0x03) << 4) | (p[j+1] >> 4)];
                            b += base64_chars[(p[j+1] & 0x0f) << 2];
                            b += '=';
                        } else {
                            b += base64_chars[(p[j] & 0x03) << 4];
                            b += "==";
                        }
                    }
                    b += "\"";
                    return b;
                }
                case SQLITE_NULL:
                default:
                    return "null";
            }
        }

        std::string get_expanded_sql() {
            char* expanded = sqlite3_expanded_sql(m_stmt);
            if (expanded) {
                std::string s = expanded;
                sqlite3_free(expanded);
                return s;
            }
            return "";
        }

        std::vector<std::string> get_column_names() {
            std::vector<std::string> names;
            int count = sqlite3_column_count(m_stmt);
            for (int i = 0; i < count; ++i) {
                names.push_back(sqlite3_column_name(m_stmt, i));
            }
            return names;
        }

        int get_params_count() {
            return sqlite3_bind_parameter_count(m_stmt);
        }

    private:
        sqlite3* m_db;
        sqlite3_stmt* m_stmt = nullptr;
    };

    class database {
    public:
        database(const std::string& filename, bool readonly, bool create, bool safeIntegers, bool strict)
            : m_safeIntegers(safeIntegers), m_strict(strict) {
            int flags = 0;
            if (readonly) flags |= SQLITE_OPEN_READONLY;
            else {
                flags |= SQLITE_OPEN_READWRITE;
                if (create) flags |= SQLITE_OPEN_CREATE;
            }

            if (sqlite3_open_v2(filename.empty() ? ":memory:" : filename.c_str(), &m_db, flags, nullptr) != SQLITE_OK) {
                std::string err = sqlite3_errmsg(m_db);
                sqlite3_close(m_db);
                m_db = nullptr;
                throw std::runtime_error(err);
            }
        }

        ~database() {
            close();
        }

        void close() {
            if (m_db) {
                m_stmt_cache.clear();
                sqlite3_close_v2(m_db);
                m_db = nullptr;
            }
        }

        std::shared_ptr<statement> prepare(const std::string& sql, bool use_cache) {
            if (use_cache) {
                auto it = m_stmt_cache.find(sql);
                if (it != m_stmt_cache.end()) {
                    return it->second;
                }
            }

            auto stmt = std::make_shared<statement>(m_db, sql);
            if (use_cache) {
                m_stmt_cache[sql] = stmt;
            }
            return stmt;
        }

        std::string serialize() {
            sqlite3_int64 size = 0;
            unsigned char* data = sqlite3_serialize(m_db, "main", &size, 0);
            if (!data) return "[]";
            std::string out = "[";
            for (sqlite3_int64 i = 0; i < size; ++i) {
                if (i > 0) out += ",";
                out += std::to_string(data[i]);
            }
            out += "]";
            sqlite3_free(data);
            return out;
        }

        static std::shared_ptr<database> deserialize(const std::vector<unsigned char>& data) {
            auto db = std::make_shared<database>(":memory:", false, true, false, false);
            unsigned char* copy = (unsigned char*)sqlite3_malloc64(data.size());
            std::copy(data.begin(), data.end(), copy);
            // Fallback for older sqlite3 versions where SQLITE_DESERIALIZE_READWRITE might not be available or
            // the function itself might be missing (it was added in 3.23.0)
#ifdef SQLITE_ENABLE_DESERIALIZE
            sqlite3_deserialize(db->m_db, "main", copy, data.size(), data.size(),
                SQLITE_DESERIALIZE_FREEONCLOSE | 1 /* SQLITE_DESERIALIZE_READWRITE */);
#endif
            return db;
        }

        int file_control(int cmd, void* value) {
            return sqlite3_file_control(m_db, "main", cmd, value);
        }

        sqlite3* get_native() { return m_db; }

        void load_extension(const std::string& path) {
            sqlite3_enable_load_extension(m_db, 1);
            char* errmsg = nullptr;
            if (sqlite3_load_extension(m_db, path.c_str(), nullptr, &errmsg) != SQLITE_OK) {
                std::string err = errmsg;
                sqlite3_free(errmsg);
                throw std::runtime_error(err);
            }
        }

        bool is_safe_integers() const { return m_safeIntegers; }
        bool is_strict() const { return m_strict; }

    private:
        sqlite3* m_db = nullptr;
        bool m_safeIntegers;
        bool m_strict;
        std::unordered_map<std::string, std::shared_ptr<statement>> m_stmt_cache;
    };

    sqlite_runtime() = default;
    ~sqlite_runtime() {
        m_databases.clear();
    }

    std::string open(const std::string& filename, bool readonly, bool create, bool safeIntegers, bool strict) {
        auto db = std::make_shared<database>(filename, readonly, create, safeIntegers, strict);
        std::string id = std::to_string(m_next_db_id++);
        m_databases[id] = db;
        return id;
    }

    void close(const std::string& id) {
        m_databases.erase(id);
    }

    std::string open_from_data(const std::vector<unsigned char>& data) {
        auto db = database::deserialize(data);
        std::string id = std::to_string(m_next_db_id++);
        m_databases[id] = db;
        return id;
    }

    std::shared_ptr<database> get_db(const std::string& id) {
        auto it = m_databases.find(id);
        if (it == m_databases.end()) return nullptr;
        return it->second;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<database>> m_databases;
    int m_next_db_id = 1;
};

} // namespace detail
} // namespace webview

#endif // WEBVIEW_DETAIL_SQLITE_RUNTIME_HH
