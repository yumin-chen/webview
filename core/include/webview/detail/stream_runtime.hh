#ifndef WEBVIEW_DETAIL_STREAM_RUNTIME_HH
#define WEBVIEW_DETAIL_STREAM_RUNTIME_HH

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace webview {
namespace detail {

class array_buffer_sink {
public:
    struct options {
        bool as_uint8_array = false;
        size_t high_water_mark = 0;
        bool stream = false;
    };

    void start(const options& opts) {
        m_options = opts;
        m_buffer.clear();
        if (m_options.high_water_mark > 0) {
            m_buffer.reserve(m_options.high_water_mark);
        }
    }

    size_t write(const uint8_t* data, size_t len) {
        m_buffer.insert(m_buffer.end(), data, data + len);
        return len;
    }

    std::vector<uint8_t> flush() {
        std::vector<uint8_t> result = std::move(m_buffer);
        m_buffer.clear();
        if (m_options.high_water_mark > 0) {
            m_buffer.reserve(m_options.high_water_mark);
        }
        return result;
    }

    std::vector<uint8_t> end() {
        return flush();
    }

    const options& get_options() const { return m_options; }

private:
    options m_options;
    std::vector<uint8_t> m_buffer;
};

} // namespace detail
} // namespace webview

#endif
